#!/usr/bin/env bash
# Flush writes and unmount the PicoCalc SD card volume on the Mac.
set -euo pipefail

SD_HOST="${SD_HOST:-manuel@192.168.0.57}"
SD_VOL="${SD_VOL:-/Volumes/NO NAME}"
ssh "$SD_HOST" "sync; diskutil unmount '$SD_VOL'"
