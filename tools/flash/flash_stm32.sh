#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage:"
    echo "  $0 <firmware.elf>"
    exit 1
fi

ELF_FILE="$1"

if [[ ! -f "$ELF_FILE" ]]; then
    echo "Error: firmware file not found:"
    echo "  $ELF_FILE"
    exit 1
fi

echo "Flashing:"
echo "  $ELF_FILE"

openocd \
    -f interface/stlink.cfg \
    -f target/stm32f1x.cfg \
    -c "program ${ELF_FILE} verify reset exit"

echo "Flash completed successfully."