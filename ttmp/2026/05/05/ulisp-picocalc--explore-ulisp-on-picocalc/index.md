---
Title: Explore uLisp on PicoCalc
Ticket: ulisp-picocalc
Status: active
Topics:
    - embedded
    - lisp
    - rp2040
    - picocalc
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Explore, compile, and test uLisp on the Clockwork Pi PicoCalc. Successfully compiled a 460KB UF2 image using arduino-cli with RP2040 core 5.6.0."
LastUpdated: 2026-05-05T18:58:11.808190854-04:00
WhatFor: ""
WhenToUse: ""
---

# Explore uLisp on PicoCalc

## Overview

Exploring uLisp (a Lisp interpreter for microcontrollers) on the Clockwork Pi PicoCalc handheld computer. The PicoCalc is based on an RP2040 Raspberry Pi Pico with a 320x320 IPS display, QWERTY keyboard, and SD card.

**Current status**: Successfully compiled uLisp from `technoblogy/ulisp-picocalc` source. UF2 image ready for flashing. Next step: flash to hardware and test.

### Key Results
- Compiled uLisp v4.7+ for PicoCalc → 460KB UF2 image at `build/ulisp-picocalc-sketch.ino.uf2`
- Toolchain: arduino-cli 1.4.1 + RP2040 core 5.6.0 + TFT_eSPI 2.5.34 + arduino_picocalc_kbd
- 7 reference documents downloaded to `sources/`

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- embedded
- lisp
- rp2040
- picocalc

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
