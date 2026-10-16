#!/bin/sh
# Build each board and merge its bootloader, partition table and firmware into one image to
# flash at 0x0: ./merge.sh [env...]   (default: tdeck wio_l2, images in dist/)
cd "$(dirname "$0")"
PIO=${PIO:-$(command -v pio || echo ~/.platformio/penv/bin/pio)}
PY=$(dirname "$PIO")/python
ESPTOOL=~/.platformio/packages/tool-esptoolpy/esptool.py
mkdir -p dist
for env in ${@:-tdeck wio_l2}; do
    if ! "$PIO" run -e "$env" > ".pio/build-$env.log" 2>&1; then
        tail -20 ".pio/build-$env.log"
        echo "BUILD FAILED: $env"
        exit 1
    fi
    b=.pio/build/$env
    "$PY" "$ESPTOOL" --chip esp32s3 merge_bin -o "dist/duke3d-$env.bin" \
        0x0 "$b/bootloader.bin" 0x8000 "$b/partitions.bin" 0x10000 "$b/firmware.bin" \
        | grep -iE "wrote|error"
done
