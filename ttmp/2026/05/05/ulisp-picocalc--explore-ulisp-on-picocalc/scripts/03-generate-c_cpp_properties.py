#!/usr/bin/env python3
"""Generate .idea/c_cpp_properties.json for CLion IntelliSense from arduino-cli build output.

Parses compile_commands.json produced by arduino-cli to extract include paths
and preprocessor defines, then adds all pico-sdk header directories that are
normally pulled in via -iprefix/@files (which CLion doesn't understand).

Usage:
    python3 03-generate-c_cpp_properties.py [--build-dir BUILD_DIR] [--output OUTPUT]

Defaults:
    --build-dir  ../../../../../build          (relative to this script)
    --output     ../../../../../.idea/c_cpp_properties.json
"""
import json
import os
import sys
import glob

def _extract_forward_declarations(preprocessed_cpp, output_header, ino_source=None):
    """Extract ONLY the forward declarations that are actually needed.

    Arduino generates prototypes for ALL functions (462 in our case), but most are
    redundant because the function is defined before it's called. We scan the .ino
    to find which functions are forward-referenced, then extract only those prototypes
    from the Arduino-preprocessed .cpp.
    """
    import re

    if not os.path.exists(preprocessed_cpp):
        print(f"WARNING: {preprocessed_cpp} not found, skipping forward declarations", file=sys.stderr)
        return

    # Step 1: Find functions that are actually forward-referenced in the .ino
    # (called before their definition)
    forward_needed = set()
    if ino_source and os.path.exists(ino_source):
        with open(ino_source) as f:
            ino_lines = f.readlines()

        func_defs = {}
        for i, line in enumerate(ino_lines, 1):
            m = re.match(
                r'^(?:inline\s+)?(?:[\w:*&<>\s]+?)\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:\{|;)',
                line
            )
            if m and not line.strip().startswith('//') and not line.strip().startswith('#'):
                name = m.group(1)
                # Only count actual definitions (ending with {), not manual prototypes (ending with ;)
                if '{' in line:
                    func_defs[name] = i

        keywords = {
            'if', 'while', 'for', 'switch', 'return', 'sizeof', 'typeof',
            'case', 'else', 'true', 'false', 'NULL', 'setjmp', 'longjmp',
            'defined', 'offsetof', 'alignof', 'decltype', 'static_assert',
        }
        for i, line in enumerate(ino_lines, 1):
            if line.strip().startswith('//') or line.strip().startswith('#'):
                continue
            for m in re.finditer(r'\b(\w+)\s*\(', line):
                name = m.group(1)
                if name in keywords:
                    continue
                if name in func_defs and func_defs[name] > i:
                    forward_needed.add(name)

    # Step 2: Extract prototypes from the Arduino preprocessed .cpp
    with open(preprocessed_cpp) as f:
        cpp_lines = f.readlines()

    # Find the forward-declaration block: starts at first #line 288, ends at second #line 288
    # (where the function bodies begin)
    prototypes = {}
    block_start = None
    block_end = None
    for i, line in enumerate(cpp_lines):
        s = line.strip()
        if s.startswith('#line 288 '):
            if block_start is None:
                block_start = i
            else:
                block_end = i
                break

    if block_start is not None and block_end is not None:
        for line in cpp_lines[block_start:block_end]:
            s = line.strip()
            if s.startswith('#line') or s == '' or s.startswith('//'):
                continue
            if s.endswith(';') and '(' in s:
                m = re.search(r'\b(\w+)\s*\(', s)
                if m:
                    name = m.group(1)
                    # If we know which ones are needed, only include those
                    if forward_needed and name not in forward_needed:
                        continue
                    prototypes[name] = s

    # If no ino_source was provided (fallback), include all prototypes
    if not forward_needed:
        pass  # already included all above

    unique_fwd = [prototypes[name] for name in sorted(prototypes)]

    with open(output_header, 'w') as f:
        f.write("// Auto-generated forward declarations (minimal set)\n")
        f.write("// Only functions that are called before their definition are included.\n")
        f.write("// Regenerate with: python3 scripts/03-generate-c_cpp_properties.py\n")
        f.write("// DO NOT EDIT - this file is overwritten on each build\n")
        f.write("#ifndef _ULISP_FWD_DECLS_H_\n")
        f.write("#define _ULISP_FWD_DECLS_H_\n\n")
        for proto in unique_fwd:
            f.write(proto + '\n')
        f.write("\n#endif // _ULISP_FWD_DECLS_H_\n")

    print(f"  Forward declarations: {len(unique_fwd)} extracted to {output_header}")


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.normpath(os.path.join(script_dir, "..", "..", "..", "..", "..", ".."))

    import argparse
    parser = argparse.ArgumentParser(description="Generate c_cpp_properties.json for CLion")
    parser.add_argument("--build-dir", default=None,
                        help="Path to arduino-cli build directory with compile_commands.json")
    parser.add_argument("--output", default=None,
                        help="Output path for c_cpp_properties.json")
    parser.add_argument("--project-dir", default=project_dir,
                        help="Project root directory")
    args = parser.parse_args()

    if args.build_dir is None:
        args.build_dir = os.path.join(args.project_dir, "build")
    if args.output is None:
        args.output = os.path.join(args.project_dir, ".idea", "c_cpp_properties.json")
    cc_json = os.path.join(args.build_dir, "compile_commands.json")
    if not os.path.exists(cc_json):
        print(f"ERROR: {cc_json} not found. Run arduino-cli compile first.", file=sys.stderr)
        sys.exit(1)

    # Parse compile_commands.json
    with open(cc_json) as f:
        cmds = json.load(f)

    includes = []
    defines = []
    seen_inc = set()
    seen_def = set()
    iprefix = None
    gcc_path = None

    def add_inc(p):
        p = os.path.normpath(p)
        if p not in seen_inc:
            includes.append(p)
            seen_inc.add(p)

    def add_def(d):
        if d not in seen_def:
            defines.append(d)
            seen_def.add(d)

    for entry in cmds:
        args_list = entry.get("arguments", [])
        if not gcc_path and args_list:
            gcc_path = args_list[0]
        i = 0
        while i < len(args_list):
            a = args_list[i]
            if a.startswith("-I") and a != "-I":
                add_inc(a[2:])
            elif a == "-I" and i + 1 < len(args_list):
                add_inc(args_list[i + 1])
                i += 1
            elif a.startswith("-D"):
                val = a[2:]
                if '"' not in val:
                    add_def(val)
            elif a.startswith("-iprefix"):
                iprefix = a[len("-iprefix"):]
            elif a.startswith("@"):
                fpath = a[1:]
                if os.path.exists(fpath):
                    with open(fpath) as f:
                        for line in f:
                            line = line.strip()
                            if line.startswith("-I"):
                                add_inc(line[2:])
                            elif line.startswith("-D") and '"' not in line:
                                add_def(line[2:])
            i += 1

    # Discover pico-sdk include directories pulled in via -iprefix
    # These contain headers like hardware/pwm.h, pico/time.h, etc.
    if iprefix and os.path.isdir(iprefix):
        pico_sdk = os.path.join(iprefix, "pico-sdk")
        if os.path.isdir(pico_sdk):
            for dirpath, dirnames, filenames in os.walk(pico_sdk):
                if dirpath.endswith("/include") and "/test/" not in dirpath:
                    add_inc(dirpath)
            # Also add the lwip includes
            lwip_inc = os.path.join(pico_sdk, "lib", "lwip", "src", "include")
            if os.path.isdir(lwip_inc):
                add_inc(lwip_inc)

    # Also add the generated config includes under the build dir's include/
    build_include = os.path.join(iprefix or "", "include")
    if os.path.isdir(build_include):
        for root, dirs, files in os.walk(build_include):
            add_inc(root)

    # Add the ArduinoCore-API headers (nested under cores/rp2040/api/../../../ArduinoCore-API/api)
    core_api_dirs = set()
    for inc in list(includes):
        parent = os.path.dirname(inc)
        api_dir = os.path.join(parent, "ArduinoCore-API", "api")
        if os.path.isdir(api_dir):
            core_api_dirs.add(api_dir)
    for d in core_api_dirs:
        add_inc(d)

    # Add sketch source directories (the actual .ino files, not just the preprocessed .cpp)
    sketch_dirs = set()
    for entry in cmds:
        f = entry.get("file", "")
        d = os.path.dirname(f)
        if d:
            sketch_dirs.add(d)
    for d in sketch_dirs:
        add_inc(d)

    # Add project-local source directories that contain .ino/.h files
    for subdir in ["ulisp-picocalc-sketch", "ulisp-picocalc"]:
        full = os.path.join(project_dir, subdir)
        if os.path.isdir(full):
            add_inc(full)

    # Generate forward declarations header from the preprocessed .cpp
    # This is needed because Arduino auto-generates forward declarations for .ino files
    # but CLion doesn't know about them.
    fwd_header = os.path.join(project_dir, "_ulisp_fwd_decls.h")
    ino_source = None
    # Try to find the .ino source for forward-reference analysis
    for subdir in ["ulisp-picocalc-sketch", "ulisp-picocalc"]:
        candidate = os.path.join(project_dir, subdir)
        if os.path.isdir(candidate):
            inos = [f for f in os.listdir(candidate) if f.endswith('.ino')]
            if inos:
                ino_source = os.path.join(candidate, inos[0])
                break
    _extract_forward_declarations(
        os.path.join(args.build_dir, "sketch", "ulisp-picocalc-sketch.ino.cpp"),
        fwd_header,
        ino_source=ino_source,
    )

    # Build the c_cpp_properties.json
    config = {
        "configurations": [
            {
                "name": "ulisp-picocalc",
                "includePath": includes,
                "defines": defines,
                "forcedInclude": [fwd_header] if os.path.exists(fwd_header) else [],
                "cStandard": "c17",
                "cppStandard": "c++17",
                "compilerPath": gcc_path or "",
                "compilerArgs": [
                    "-march=armv6-m",
                    "-mcpu=cortex-m0plus",
                    "-mthumb",
                    "-fno-exceptions",
                    "-fno-rtti",
                ],
            }
        ],
        "version": 4,
    }

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, "w") as f:
        json.dump(config, f, indent=4)

    print(f"Generated {args.output}")
    print(f"  {len(includes)} include paths")
    print(f"  {len(defines)} defines")
    if gcc_path:
        print(f"  compiler: {gcc_path}")


if __name__ == "__main__":
    main()
