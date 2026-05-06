#!/usr/bin/env python3
"""Extract Arduino-generated forward declarations from the preprocessed .cpp file.

The Arduino builder inserts ~925 lines of function prototypes (forward declarations)
before the first function body. These are what make the single-file .ino approach
work without manual forward declarations.

This script extracts those declarations into a standalone header that can be used
as a forced include in CLion's c_cpp_properties.json.
"""
import re
import sys
import os


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.normpath(os.path.join(script_dir, "..", "..", "..", "..", "..", ".."))

    preprocessed_cpp = os.path.join(project_dir, "build", "sketch", "ulisp-picocalc-sketch.ino.cpp")
    output_header = os.path.join(project_dir, "_ulisp_fwd_decls.h")

    if not os.path.exists(preprocessed_cpp):
        print(f"ERROR: {preprocessed_cpp} not found. Run arduino-cli compile first.", file=sys.stderr)
        sys.exit(1)

    with open(preprocessed_cpp) as f:
        content = f.read()
        lines = content.split('\n')

    # Strategy: find the block of forward declarations inserted by Arduino.
    # Pattern: lines of "#line N" followed by "prototype;" repeated many times,
    # then the first function body starts with "type name(...) {".
    #
    # The forward declarations start at the first "#line N" that is followed by
    # a prototype ending in ";", and end when we hit a line ending in "{"
    # (function body start).

    fwd_lines = []
    in_fwd_block = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        # Skip the #include <Arduino.h> at the top
        if stripped == '#include <Arduino.h>':
            continue

        # The original .ino content starts at the first #line 1 directive
        # After that comes the original source (includes, defines, globals)
        # Then Arduino inserts a big block of forward declarations
        # Then the function bodies begin

        # Detect start of fwd block: a #line followed by a prototype ending in ;
        if not in_fwd_block:
            if stripped.startswith('#line') and i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                if next_line.endswith(';') and '(' in next_line and not next_line.startswith('#'):
                    in_fwd_block = True
                    fwd_lines.append(next_line)
                    continue
            continue

        # We're in the forward declaration block
        if stripped.startswith('#line'):
            # Skip #line directives
            continue
        elif stripped == '' or stripped.startswith('//'):
            # Skip blanks and comments
            continue
        elif stripped.endswith(';') and '(' in stripped:
            # Forward declaration
            fwd_lines.append(stripped)
        elif stripped.endswith('{'):
            # Hit a function body — we're done
            break
        else:
            # Some other line (maybe a typedef or using) — include it
            fwd_lines.append(stripped)

    # Deduplicate while preserving order
    seen = set()
    unique_fwd = []
    for proto in fwd_lines:
        if proto not in seen:
            seen.add(proto)
            unique_fwd.append(proto)

    # Write the header
    with open(output_header, 'w') as f:
        f.write("// Auto-generated forward declarations from Arduino .ino preprocessing\n")
        f.write("// Regenerate with: python3 scripts/04-extract-forward-declarations.py\n")
        f.write("// DO NOT EDIT - this file is overwritten on each build\n")
        f.write("#ifndef _ULISP_FWD_DECLS_H_\n")
        f.write("#define _ULISP_FWD_DECLS_H_\n\n")
        for proto in unique_fwd:
            f.write(proto + '\n')
        f.write("\n#endif // _ULISP_FWD_DECLS_H_\n")

    print(f"Extracted {len(unique_fwd)} forward declarations to {output_header}")

    # Verify key functions are present
    key_funcs = ['pserial', 'pln', 'indent', 'printstring', 'setup', 'loop']
    for fn in key_funcs:
        found = any(fn + '(' in p for p in unique_fwd)
        print(f"  {fn}: {'FOUND' if found else 'MISSING'}")


if __name__ == "__main__":
    main()
