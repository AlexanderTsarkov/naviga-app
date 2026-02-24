#!/bin/sh
# Optional: helper to capture 240s from two serial devices.
# Usage: ./capture_script.sh /dev/ttyUSB0 /dev/ttyUSB1
# Or run manually: e.g. cat /dev/ttyUSB0 | tee device_A_serial.log (then stop after 240s).

A="${1:-/dev/ttyUSB0}"
B="${2:-/dev/ttyUSB1}"
DURATION=240

echo "Capture $DURATION s from A=$A and B=$B. Run in two terminals:"
echo "  timeout ${DURATION} cat $A | tee device_A_serial.log"
echo "  timeout ${DURATION} cat $B | tee device_B_serial.log"
echo "Before capture: on each device: wait ~28s, then 'debug on', wait 2s, flush, start capture."
