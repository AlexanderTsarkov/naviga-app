#!/usr/bin/env python3
"""
Dual serial logger for two ESP32 boards.

Writes plain-text logs with timestamps and board tags.
Designed for quick "capture ~N seconds then exit" sessions.

Example:
  python3 tools/dual_serial_log.py \
    --port-a /dev/cu.wchusbserialAAAA \
    --port-b /dev/cu.wchusbserialBBBB \
    --out-a boardA.log \
    --out-b boardB.log \
    --seconds 90
"""

from __future__ import annotations

import argparse
import threading
import time
from dataclasses import dataclass

import serial  # pyserial


@dataclass
class PortSpec:
    tag: str
    port: str
    baud: int
    out_path: str


def _now_ms(start: float) -> int:
    return int((time.time() - start) * 1000)


def _reader(
    stop_evt: threading.Event,
    spec: PortSpec,
    start: float,
    reset_after_ms: int,
) -> None:
    ser = serial.Serial(spec.port, spec.baud, timeout=0.2)
    ser.reset_input_buffer()

    # Open file in binary mode, write UTF-8 ourselves to avoid encoding issues.
    with open(spec.out_path, "wb") as f:
        header = f"---\nport: {spec.port}\nbaud: {spec.baud}\ntag: {spec.tag}\nstart_unix: {time.time()}\n---\n"
        f.write(header.encode("utf-8", errors="replace"))
        f.flush()

        reset_done = False

        while not stop_evt.is_set():
            # Optional: reset after logger started, so boot is captured in logs.
            if not reset_done and reset_after_ms >= 0 and _now_ms(start) >= reset_after_ms:
                try:
                    ser.rts = True
                    time.sleep(0.15)
                    ser.rts = False
                    time.sleep(0.15)
                    msg = f"[{_now_ms(start):>8}ms] {spec.tag} RESET_RTS_PULSE\n"
                    f.write(msg.encode("utf-8", errors="replace"))
                    f.flush()
                except Exception as e:
                    msg = f"[{_now_ms(start):>8}ms] {spec.tag} RESET_ERROR: {e}\n"
                    f.write(msg.encode("utf-8", errors="replace"))
                    f.flush()
                reset_done = True

            try:
                line = ser.readline()  # bytes until '\n' or timeout
            except Exception as e:
                msg = f"[{_now_ms(start):>8}ms] {spec.tag} READ_ERROR: {e}\n"
                f.write(msg.encode("utf-8", errors="replace"))
                f.flush()
                time.sleep(0.2)
                continue

            if not line:
                continue

            # Decode for human log; preserve non-utf8 by replacement.
            # Replace embedded NUL to keep logs readable and avoid "binary" detection.
            text = line.decode("utf-8", errors="replace").replace("\x00", "\\x00").rstrip("\r\n")
            msg = f"[{_now_ms(start):>8}ms] {spec.tag} {text}\n"
            f.write(msg.encode("utf-8", errors="replace"))
            f.flush()

    try:
        ser.close()
    except Exception:
        pass


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port-a", required=True)
    ap.add_argument("--port-b", required=True)
    ap.add_argument("--out-a", required=True)
    ap.add_argument("--out-b", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--seconds", type=int, default=90)
    ap.add_argument(
        "--reset-rts",
        action="store_true",
        help="Pulse RTS on both ports once at start (ESP32 hard reset).",
    )
    ap.add_argument(
        "--reset-after-ms",
        type=int,
        default=-1,
        help="If set (e.g. 1000), pulses RTS from inside reader after N ms so boot is captured.",
    )
    args = ap.parse_args()

    stop_evt = threading.Event()
    start = time.time()

    specs = [
        PortSpec(tag="A", port=args.port_a, baud=args.baud, out_path=args.out_a),
        PortSpec(tag="B", port=args.port_b, baud=args.baud, out_path=args.out_b),
    ]

    threads = [
        threading.Thread(
            target=_reader, args=(stop_evt, specs[0], start, args.reset_after_ms), daemon=True
        ),
        threading.Thread(
            target=_reader, args=(stop_evt, specs[1], start, args.reset_after_ms), daemon=True
        ),
    ]

    if args.reset_rts:
        # esptool uses RTS to hard-reset; do the same before starting readers.
        for s in specs:
            try:
                ser = serial.Serial(s.port, s.baud, timeout=0.2)
                ser.rts = True
                time.sleep(0.15)
                ser.rts = False
                time.sleep(0.15)
                ser.close()
            except Exception:
                pass

    for t in threads:
        t.start()

    try:
        time.sleep(max(1, args.seconds))
    except KeyboardInterrupt:
        pass
    finally:
        stop_evt.set()

    for t in threads:
        t.join(timeout=2.0)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

