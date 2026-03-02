#!/usr/bin/env python3
"""
hw339 bench — E22 preset regression: Default (2.4k) vs Fast (4.8k).

Goals:
  1. Verify preset applied (boot log shows readback matches target).
  2. Measure TX inter-packet gap as airtime proxy for same packet types.
  3. Produce summary table: preset | rb_airRate | pkt_type | mean_gap_ms | stddev | n.

Measurement method:
  Firmware logs "pkt tx t_ms=<uptime_ms> type=<TYPE> seq=<N>" after radio_.send() returns.
  radio_.send() on E22 UART is blocking (library waits for AUX HIGH = TX done).
  Therefore: gap[n] = t_ms[n] - t_ms[n-1] ≈ airtime(pkt[n-1]) + processing_overhead.
  Processing overhead is constant across presets → delta(gap) reflects airtime difference.

Preset switching:
  No runtime "radio preset" shell command exists. Preset is set at compile time in
  radio_factory.cpp (RadioPresetId::Default or ::Fast). To run both presets:
  1. Run with --preset default (current firmware).
  2. Change radio_factory.cpp to RadioPresetId::Fast, rebuild, reflash, run with --preset fast.
  OR: run with --preset both — script will pause and prompt for reflash between runs.

Port config:
  NODE_A = /dev/cu.wchusbserial5B3D0112491
  NODE_B = /dev/cu.wchusbserial5B3D0164361

Usage:
  python3 run_bench_339.py --preset default|fast|both [--port-a PORT] [--port-b PORT] [--duration N]
"""

import argparse
import re
import sys
import os
import time
import threading
import statistics

PORT_A_DEFAULT = "/dev/cu.wchusbserial5B3D0112491"
PORT_B_DEFAULT = "/dev/cu.wchusbserial5B3D0164361"
BAUD = 115200

ARTIFACTS = os.path.dirname(os.path.abspath(__file__))

# Known wire sizes (bytes) by packet type (header 2B + payload).
WIRE_SIZES = {
    "CORE":  17,
    "ALIVE": 12,
    "TAIL1": 13,
    "TAIL2": 13,
    "INFO":  13,
}

try:
    import serial
except ImportError:
    print("[FATAL] pyserial not installed. Run: pip install pyserial")
    sys.exit(1)


# ── NodeSerial ────────────────────────────────────────────────────────────────

class NodeSerial:
    def __init__(self, port, label, logfile):
        self.port = port
        self.label = label
        self.logfile = logfile
        self._lines = []
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._ser = None
        self._fh = None

    def open(self):
        self._ser = serial.Serial(self.port, BAUD, timeout=0.1)
        self._fh = open(self.logfile, "w", buffering=1)
        self._thread = threading.Thread(target=self._reader, daemon=True)
        self._thread.start()
        _log(f"{self.label}: opened {self.port} → {self.logfile}")

    def _reader(self):
        while not self._stop.is_set():
            try:
                raw = self._ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").rstrip()
                stamped = f"[{_ts()}] {line}"
                self._fh.write(stamped + "\n")
                with self._lock:
                    self._lines.append(stamped)
            except Exception as e:
                if not self._stop.is_set():
                    _log(f"{self.label} reader error: {e}")

    def send(self, cmd):
        self._ser.write((cmd + "\r\n").encode())
        _log(f"{self.label} >> {cmd}")
        time.sleep(0.3)

    def snapshot_idx(self):
        with self._lock:
            return len(self._lines)

    def collect_from(self, start_idx, pattern, duration):
        deadline = time.time() + duration
        results = []
        idx = start_idx
        while time.time() < deadline:
            with self._lock:
                new = self._lines[idx:]
            for line in new:
                if re.search(pattern, line):
                    results.append(line)
            idx += len(new)
            time.sleep(0.15)
        return results

    def wait_from(self, start_idx, pattern, timeout=30):
        deadline = time.time() + timeout
        idx = start_idx
        while time.time() < deadline:
            with self._lock:
                new = self._lines[idx:]
            for line in new:
                if re.search(pattern, line):
                    return line
            idx += len(new)
            time.sleep(0.1)
        return None

    def recent(self, n=30):
        with self._lock:
            return list(self._lines[-n:])

    def all_lines(self):
        with self._lock:
            return list(self._lines)

    def close(self):
        self._stop.set()
        if self._ser:
            self._ser.close()
        if self._fh:
            self._fh.close()


# ── helpers ───────────────────────────────────────────────────────────────────

def _ts():
    return time.strftime("%H:%M:%S")


def _log(msg):
    print(f"[{_ts()}] {msg}")


def section(title):
    print(f"\n{'='*60}\n  {title}\n{'='*60}")


def check(cond, label, detail="", fatal=True):
    status = "PASS" if cond else "FAIL"
    print(f"  [{status}] {label}" + (f"  ({detail})" if detail else ""))
    if not cond and fatal:
        print("  *** STOP CONDITION ***")
        sys.exit(1)
    return cond


def extract_t_ms(line):
    m = re.search(r't_ms=(\d+)', line)
    return int(m.group(1)) if m else None


def extract_pkt_type(line):
    m = re.search(r'type=(\w+)', line)
    return m.group(1) if m else None


def compute_gaps(tx_lines):
    """Given 'pkt tx t_ms=...' lines, return list of (type, gap_ms) pairs.
    gap[n] = t_ms[n] - t_ms[n-1], attributed to type of pkt[n-1]."""
    points = []
    for line in tx_lines:
        t = extract_t_ms(line)
        typ = extract_pkt_type(line)
        if t is not None and typ is not None:
            points.append((t, typ))
    gaps = []
    for i in range(1, len(points)):
        gap = points[i][0] - points[i-1][0]
        if 0 < gap < 10000:  # sanity: ignore gaps > 10s (cadence pauses)
            gaps.append((points[i-1][1], gap))
    return gaps


def stats_by_type(gaps):
    by_type = {}
    for typ, gap in gaps:
        by_type.setdefault(typ, []).append(gap)
    result = {}
    for typ, vals in by_type.items():
        n = len(vals)
        mean = sum(vals) / n
        stddev = statistics.stdev(vals) if n > 1 else 0.0
        result[typ] = {"mean": mean, "stddev": stddev, "n": n}
    return result


def parse_boot_preset(lines):
    """
    Scan lines for E22/E220 boot result.
    Returns (rb_air_rate: int|None, boot_msg: str, status: str).
    Status: 'ok' | 'repaired' | 'repair_failed' | 'not_found'.
    """
    for line in lines:
        # "E22 boot: config ok"
        if re.search(r'(E22|E220) boot: config ok', line):
            return None, "config ok", "ok"
        # "E22 boot: repaired (air_rate clamped: req=X norm=Y)"
        m = re.search(r'(E22|E220) boot: repaired \((.+)\)', line)
        if m:
            detail = m.group(2)
            cm = re.search(r'norm=(\d+)', detail)
            rb = int(cm.group(1)) if cm else None
            return rb, f"repaired: {detail}", "repaired"
        # "E22 boot: repaired (<fields>)" — field list repair (no clamp)
        m2 = re.search(r'(E22|E220) boot: repaired$', line)
        if m2:
            return None, "repaired (fields)", "repaired"
        # "E22 boot: repair failed (...)"
        m3 = re.search(r'(E22|E220) boot: repair failed \((.+)\)', line)
        if m3:
            return None, f"repair_failed: {m3.group(2)}", "repair_failed"
    return None, "not found in log", "not_found"


# ── Single preset run ─────────────────────────────────────────────────────────

def run_preset(nodeA, nodeB, preset_name, collect_duration):
    section(f"Preset: {preset_name}")

    # Step 1: Reboot to capture fresh boot log with preset info
    _log("  Rebooting NodeA to capture boot log...")
    idx_reboot = nodeA.snapshot_idx()
    nodeA.send("reboot")
    # Wait for boot sequence (E22 boot + GNSS init takes ~5s)
    boot_line = nodeA.wait_from(idx_reboot, r'(E22|E220) boot:', timeout=15)
    if boot_line:
        print(f"  A| {boot_line}")
    else:
        _log("  [WARN] No E22/E220 boot line seen in 15s")

    # Collect all boot lines
    time.sleep(3)
    boot_lines = nodeA.collect_from(idx_reboot, r'.*', duration=2)
    rb_air_rate, boot_msg, boot_status = parse_boot_preset(boot_lines)
    _log(f"  Boot preset: status={boot_status} rb_airRate={rb_air_rate} msg={boot_msg}")

    # Step 2: Enable instrumentation
    idx0 = nodeA.snapshot_idx()
    nodeA.send("debug on")
    resp = nodeA.wait_from(idx0, r'instrumentation', timeout=5)
    if resp:
        print(f"  A| {resp}")
    else:
        _log("  [WARN] No instrumentation ack — continuing anyway")

    if nodeB:
        nodeB.send("debug on")

    # Step 3: Trigger GNSS fix + movement to generate CORE packets
    nodeA.send("gnss fix 374200000 -1220800000")
    if nodeB:
        nodeB.send("gnss fix 374300000 -1220900000")
    time.sleep(1)
    nodeA.send("gnss move 200000 0")
    time.sleep(0.5)

    # Step 4: Collect pkt tx lines; periodically trigger movement to keep CORE flowing
    idx_tx = nodeA.snapshot_idx()
    _log(f"  Collecting TX lines for {collect_duration}s (with periodic gnss move)...")
    tx_lines = []
    move_interval = 30  # trigger gnss move every 30s
    last_move = time.time()
    deadline = time.time() + collect_duration
    lat_offset = 200000
    while time.time() < deadline:
        # Collect what arrived
        with nodeA._lock:
            new = nodeA._lines[idx_tx:]
        for line in new:
            if re.search(r'pkt tx t_ms=', line):
                tx_lines.append(line)
        idx_tx += len(new)
        # Periodic move to keep CORE packets flowing
        if time.time() - last_move >= move_interval:
            lat_offset += 200000
            nodeA.send(f"gnss move {lat_offset} 0")
            last_move = time.time()
        time.sleep(0.5)

    _log(f"  Collected {len(tx_lines)} pkt tx lines")
    for l in tx_lines[:8]:
        print(f"    A| {l}")
    if len(tx_lines) > 8:
        print(f"    ... ({len(tx_lines)-8} more)")

    # Step 5: Compute gaps
    gaps = compute_gaps(tx_lines)
    st = stats_by_type(gaps)

    # Step 6: RX sanity (NodeB)
    rx_lines = []
    if nodeB:
        idx_rx = nodeB.snapshot_idx()
        rx_lines = nodeB.collect_from(idx_rx, r'pkt rx', duration=10)
        _log(f"  NodeB RX: {len(rx_lines)} lines")
        for l in rx_lines[:3]:
            print(f"    B| {l}")

    return boot_status, rb_air_rate, boot_msg, st, tx_lines, rx_lines


# ── Summary ───────────────────────────────────────────────────────────────────

def print_summary_table(results):
    section("SUMMARY TABLE")
    print(f"  {'preset':<10} {'rb_airRate':>10} {'pkt_type':<8} {'wire_B':>6} "
          f"{'mean_gap_ms':>12} {'stddev':>8} {'n':>5}")
    print(f"  {'-'*10} {'-'*10} {'-'*8} {'-'*6} {'-'*12} {'-'*8} {'-'*5}")

    for preset_name, boot_status, rb_air_rate, boot_msg, st in results:
        rb_str = str(rb_air_rate) if rb_air_rate is not None else f"ok({boot_status})"
        for typ in ["CORE", "ALIVE", "TAIL1", "TAIL2", "INFO"]:
            if typ not in st:
                continue
            d = st[typ]
            wire = WIRE_SIZES.get(typ, "?")
            print(f"  {preset_name:<10} {rb_str:>10} {typ:<8} {wire:>6} "
                  f"{d['mean']:>12.1f} {d['stddev']:>8.1f} {d['n']:>5}")

    if len(results) >= 2:
        section("COMPARISON: Default vs Fast")
        default_st = next((st for name, _, _, _, st in results if "default" in name.lower()), None)
        fast_st    = next((st for name, _, _, _, st in results if "fast"    in name.lower()), None)
        if default_st and fast_st:
            print(f"  {'pkt_type':<8} {'default_ms':>12} {'fast_ms':>10} {'ratio':>8}  verdict")
            print(f"  {'-'*8} {'-'*12} {'-'*10} {'-'*8}  -------")
            for typ in ["CORE", "ALIVE", "TAIL1", "TAIL2", "INFO"]:
                if typ in default_st and typ in fast_st:
                    d_mean = default_st[typ]["mean"]
                    f_mean = fast_st[typ]["mean"]
                    ratio = d_mean / f_mean if f_mean > 0 else float("nan")
                    if ratio > 1.5:
                        verdict = f"Fast is ~{ratio:.1f}x faster (expected ~2x)"
                    elif ratio > 1.1:
                        verdict = f"Fast is {ratio:.1f}x faster (partial)"
                    else:
                        verdict = f"No clear improvement (ratio={ratio:.2f})"
                    print(f"  {typ:<8} {d_mean:>12.1f} {f_mean:>10.1f} {ratio:>8.2f}  {verdict}")


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preset", choices=["default", "fast", "both"], default="default")
    parser.add_argument("--port-a", default=PORT_A_DEFAULT)
    parser.add_argument("--port-b", default=PORT_B_DEFAULT)
    parser.add_argument("--duration", type=int, default=180,
                        help="Collection window per preset in seconds (default 180)")
    args = parser.parse_args()

    presets_to_run = []
    if args.preset in ("default", "both"):
        presets_to_run.append("Default")
    if args.preset in ("fast", "both"):
        presets_to_run.append("Fast")

    results = []

    for i, preset_name in enumerate(presets_to_run):
        log_a = os.path.join(ARTIFACTS, f"nodeA_{preset_name.lower()}.log")
        log_b = os.path.join(ARTIFACTS, f"nodeB_{preset_name.lower()}.log")

        # If running both and this is Fast, prompt for reflash
        if preset_name == "Fast" and "Default" in presets_to_run:
            section("ACTION REQUIRED: Reflash with Fast preset")
            print("  1. Edit firmware/src/platform/radio_factory.cpp:")
            print("     Change: get_radio_preset(RadioPresetId::Default)")
            print("     To:     get_radio_preset(RadioPresetId::Fast)")
            print("  2. Run: pio run -e devkit_e22_oled_gnss --target upload \\")
            print(f"           --upload-port {args.port_a}")
            print("  3. Wait for boot, then press ENTER.")
            input("  Press ENTER when NodeA is running with Fast preset... ")

        nodeA = NodeSerial(args.port_a, "NodeA", log_a)
        nodeB = None
        try:
            nodeB = NodeSerial(args.port_b, "NodeB", log_b)
            nodeB.open()
        except Exception as e:
            _log(f"NodeB not available ({e}); TX-only mode.")
            nodeB = None

        nodeA.open()
        _log(f"Settling 8s before {preset_name} run...")
        time.sleep(8)

        boot_status, rb_air_rate, boot_msg, st, tx_lines, rx_lines = run_preset(
            nodeA, nodeB, preset_name, args.duration)

        results.append((preset_name, boot_status, rb_air_rate, boot_msg, st))

        # Per-run checks
        check(boot_status in ("ok", "repaired"),
              f"{preset_name}: preset boot ok",
              f"status={boot_status} msg={boot_msg}",
              fatal=False)

        total_gaps = sum(d["n"] for d in st.values())
        check(total_gaps >= 10,
              f"{preset_name}: ≥10 TX gap samples",
              f"total={total_gaps}",
              fatal=False)

        if rx_lines:
            check(len(rx_lines) >= 1,
                  f"{preset_name}: NodeB received ≥1 packet",
                  f"rx={len(rx_lines)}",
                  fatal=False)

        nodeA.close()
        if nodeB:
            nodeB.close()

    # Summary
    print_summary_table(results)

    section("CONCLUSION")
    all_ok = True
    for preset_name, boot_status, rb_air_rate, boot_msg, st in results:
        total = sum(d["n"] for d in st.values())
        ok = boot_status in ("ok", "repaired") and total >= 10
        print(f"  {'[OK]' if ok else '[WARN]'} {preset_name}: boot={boot_status} "
              f"rb_airRate={rb_air_rate} gaps={total}")
        if not ok:
            all_ok = False

    if len(results) >= 2:
        default_st = next((st for name, _, _, _, st in results if "default" in name.lower()), None)
        fast_st    = next((st for name, _, _, _, st in results if "fast"    in name.lower()), None)
        if default_st and fast_st:
            improvements = []
            for typ in WIRE_SIZES:
                if typ in default_st and typ in fast_st:
                    ratio = default_st[typ]["mean"] / fast_st[typ]["mean"]
                    if ratio > 1.2:
                        improvements.append(f"{typ}:{ratio:.2f}x")
            if improvements:
                print(f"  [OK] Fast shows improvement: {', '.join(improvements)}")
            else:
                print("  [WARN] No clear improvement Default→Fast. Check boot logs.")
                all_ok = False

    status_str = "ALL CHECKS PASSED" if all_ok else "SOME CHECKS FAILED — review above"
    print(f"\n  {status_str}")
    _log(f"Artifacts: {ARTIFACTS}/")


if __name__ == "__main__":
    main()
