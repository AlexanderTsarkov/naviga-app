#!/usr/bin/env python3
"""
hw317 bench script — deterministic evidence capture for issue #317.
Single-process, opens both ports once, runs all scenarios, writes logs.

Ports:
  NODE_A = /dev/cu.wchusbserial5B3D0112491  (serial 5B3D011249)
  NODE_B = /dev/cu.wchusbserial5B3D0164361  (serial 5B3D016436)
Baud: 115200
"""

import serial
import threading
import time
import re
import sys
import os

PORT_A = "/dev/cu.wchusbserial5B3D0112491"
PORT_B = "/dev/cu.wchusbserial5B3D0164361"
BAUD   = 115200

ARTIFACTS = os.path.dirname(os.path.abspath(__file__))
LOG_A = os.path.join(ARTIFACTS, "nodeA_s02E_rerun.log")
LOG_B = os.path.join(ARTIFACTS, "nodeB_s02E_rerun.log")

# ── NodeSerial ────────────────────────────────────────────────────────────────

class NodeSerial:
    def __init__(self, port, label, logfile):
        self.port    = port
        self.label   = label
        self.logfile = logfile
        self._lines  = []   # all stamped lines ever received
        self._lock   = threading.Lock()
        self._stop   = threading.Event()
        self._ser    = None
        self._fh     = None

    def open(self):
        self._ser = serial.Serial(self.port, BAUD, timeout=0.1)
        self._fh  = open(self.logfile, "w", buffering=1)
        self._thread = threading.Thread(target=self._reader, daemon=True)
        self._thread.start()
        _log(f"{self.label}: opened {self.port}")

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
        time.sleep(0.2)

    def snapshot_idx(self):
        """Return current line count (use as start index for future collect/wait)."""
        with self._lock:
            return len(self._lines)

    def collect_from(self, start_idx, pattern, duration):
        """Collect lines matching pattern that arrive after start_idx, for duration seconds."""
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

    def wait_from(self, start_idx, pattern, timeout=45):
        """Wait for first new line matching pattern after start_idx."""
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

    def recent(self, n=15):
        with self._lock:
            return list(self._lines[-n:])

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

def check(cond, label, detail=""):
    status = "PASS" if cond else "FAIL"
    print(f"  [{status}] {label}" + (f"  ({detail})" if detail else ""))
    if not cond:
        print("  *** STOP CONDITION ***")
        sys.exit(1)

def seq(line):
    m = re.search(r'\bseq=(\d+)', line)
    return int(m.group(1)) if m else None

def core_seq(line):
    m = re.search(r'\bcore_seq=(\d+)', line)
    return int(m.group(1)) if m else None

def collect_both(nodeA, nodeB, pat_A, pat_B, duration, idxA=None, idxB=None):
    """Collect from both nodes concurrently."""
    if idxA is None: idxA = nodeA.snapshot_idx()
    if idxB is None: idxB = nodeB.snapshot_idx()
    res = {}
    def _a(): res['A'] = nodeA.collect_from(idxA, pat_A, duration)
    def _b(): res['B'] = nodeB.collect_from(idxB, pat_B, duration)
    ta = threading.Thread(target=_a); tb = threading.Thread(target=_b)
    ta.start(); tb.start(); ta.join(); tb.join()
    return res['A'], res['B']

# ── main ──────────────────────────────────────────────────────────────────────

def main():
    nodeA = NodeSerial(PORT_A, "NodeA", LOG_A)
    nodeB = NodeSerial(PORT_B, "NodeB", LOG_B)

    try:
        nodeA.open()
        nodeB.open()
    except serial.SerialException as e:
        _log(f"FATAL: {e}")
        sys.exit(1)

    # ── A) Baseline bring-up ─────────────────────────────────────────────────
    section("A) Baseline bring-up")
    _log("Waiting 5s for nodes to settle after open...")
    time.sleep(5)

    nodeA.send("debug on")
    nodeB.send("debug on")
    time.sleep(0.5)
    nodeA.send("status")
    nodeB.send("status")
    time.sleep(1)

    _log("NodeA recent:")
    for l in nodeA.recent(12): print(f"  A| {l}")
    _log("NodeB recent:")
    for l in nodeB.recent(12): print(f"  B| {l}")

    # ── B) Core_Pos vs Alive ─────────────────────────────────────────────────
    section("B) Core_Pos vs Alive (P0 must periodic)")

    # profile interval requires reboot — skip; use gnss override + move to trigger Core_Pos.
    # Step 1: activate override at a known position (clears real GNSS), then move to trigger displacement.
    nodeA.send("gnss fix 374200000 -1220800000")
    nodeB.send("gnss fix 374300000 -1220900000")
    time.sleep(1)
    # Step 2: snapshot before move, then move to trigger SELF_POS displacement → Core_Pos TX.
    idxA = nodeA.snapshot_idx()
    idxB = nodeB.snapshot_idx()
    nodeA.send("gnss move 200000 0")
    nodeB.send("gnss move 200000 0")
    time.sleep(1)

    _log("Collecting Core_Pos TX(A) and RX(B) concurrently for 60s...")
    core_tx_A, core_rx_B = collect_both(
        nodeA, nodeB,
        r'pkt tx.*type=CORE', r'pkt rx.*type=CORE',
        duration=60, idxA=idxA, idxB=idxB
    )
    _log(f"NodeA CORE TX: {len(core_tx_A)}")
    for l in core_tx_A[:5]: print(f"  A| {l}")
    _log(f"NodeB CORE RX: {len(core_rx_B)}")
    for l in core_rx_B[:5]: print(f"  B| {l}")

    check(len(core_tx_A) >= 1, "NodeA transmits Core_Pos when FIX active",
          f"{len(core_tx_A)} frames")
    check(len(core_rx_B) >= 1, "NodeB receives Core_Pos from NodeA",
          f"{len(core_rx_B)} frames")

    # Switch NodeA to NOFIX
    _log("Switching NodeA to NOFIX...")
    idxA2 = nodeA.snapshot_idx()
    idxB2 = nodeB.snapshot_idx()
    nodeA.send("gnss nofix")
    time.sleep(1)

    _log("Collecting Alive TX(A) and Core TX(A) after NOFIX for 100s...")
    res = {}
    def _alive():      res['alive']      = nodeA.collect_from(idxA2, r'pkt tx.*type=ALIVE', 100)
    def _core_after(): res['core_after'] = nodeA.collect_from(idxA2, r'pkt tx.*type=CORE',  100)
    t1 = threading.Thread(target=_alive); t2 = threading.Thread(target=_core_after)
    t1.start(); t2.start(); t1.join(); t2.join()
    alive_tx_A    = res['alive']
    core_tx_after = res['core_after']

    _log(f"NodeA ALIVE TX after NOFIX: {len(alive_tx_A)}")
    for l in alive_tx_A[:5]: print(f"  A| {l}")
    _log(f"NodeA CORE TX after NOFIX (should be 0): {len(core_tx_after)}")

    check(len(alive_tx_A) >= 1, "NodeA transmits Alive when NOFIX + max_silence hit",
          f"{len(alive_tx_A)} frames")
    check(len(core_tx_after) == 0, "NodeA stops Core_Pos after NOFIX",
          f"unexpected CORE count={len(core_tx_after)}")

    # ── C) Core_Tail dual-seq + Variant 2 ───────────────────────────────────
    section("C) Core_Tail dual-seq + Variant 2")

    nodeA.send("gnss fix 374200000 -1220800000")
    time.sleep(1)
    idxA3 = nodeA.snapshot_idx()
    idxB3 = nodeB.snapshot_idx()
    # Force displacement trigger
    nodeA.send("gnss move 200000 0")
    time.sleep(1)

    _log("Waiting for Core_Pos TX on NodeA (up to 30s)...")
    core_line = nodeA.wait_from(idxA3, r'pkt tx.*type=CORE', timeout=30)
    check(core_line is not None, "NodeA emits Core_Pos after FIX+move", str(core_line))
    core_s = seq(core_line)
    print(f"  A| {core_line}  → seq={core_s}")

    _log("Waiting for Core_Tail (TAIL1) TX on NodeA...")
    tail_line = nodeA.wait_from(idxA3, r'pkt tx.*type=TAIL1', timeout=15)
    check(tail_line is not None, "NodeA emits Core_Tail (TAIL1) after Core_Pos", str(tail_line))
    tail_s      = seq(tail_line)
    tail_core_s = core_seq(tail_line)
    print(f"  A| {tail_line}  → seq={tail_s} core_seq={tail_core_s}")

    check(tail_s is not None and core_s is not None and tail_s == core_s + 1,
          f"Core_Tail seq16 = Core_Pos seq16 + 1",
          f"core={core_s} tail={tail_s}")
    check(tail_core_s == core_s,
          f"Core_Tail ref_core_seq16 == Core_Pos seq16",
          f"ref={tail_core_s} expected={core_s}")

    # Variant 2: collect TAIL1 RX on NodeB; confirm at most 1 per ref_core_seq16
    _log("Collecting TAIL1 RX on NodeB for 20s (Variant 2 check)...")
    tail_rx_B = nodeB.collect_from(idxB3, r'pkt rx.*type=TAIL1', duration=20)
    _log(f"NodeB TAIL1 RX: {len(tail_rx_B)}")
    for l in tail_rx_B: print(f"  B| {l}")
    dup = [l for l in tail_rx_B if core_seq(l) == core_s]
    check(len(dup) <= 1,
          "NodeB does not apply second tail for same ref_core_seq16 (Variant 2)",
          f"tails with ref={core_s}: {len(dup)}")

    # ── D) Tail2 split: 0x04 vs 0x05 ────────────────────────────────────────
    section("D) Tail2 split: Operational (0x04) vs Informative (0x05)")

    idxA4 = nodeA.snapshot_idx()
    idxB4 = nodeB.snapshot_idx()
    _log("Collecting TAIL2 + INFO TX(A) and RX(B) concurrently for 120s...")
    res4 = {}
    def _d1(): res4['tail2_tx'] = nodeA.collect_from(idxA4, r'pkt tx.*type=TAIL2', 120)
    def _d2(): res4['info_tx']  = nodeA.collect_from(idxA4, r'pkt tx.*type=INFO',  120)
    def _d3(): res4['tail2_rx'] = nodeB.collect_from(idxB4, r'pkt rx.*type=TAIL2', 120)
    def _d4(): res4['info_rx']  = nodeB.collect_from(idxB4, r'pkt rx.*type=INFO',  120)
    threads = [threading.Thread(target=f) for f in [_d1, _d2, _d3, _d4]]
    for t in threads: t.start()
    for t in threads: t.join()

    tail2_tx_A = res4['tail2_tx']
    info_tx_A  = res4['info_tx']
    tail2_rx_B = res4['tail2_rx']
    info_rx_B  = res4['info_rx']

    _log(f"NodeA TAIL2 TX: {len(tail2_tx_A)}, INFO TX: {len(info_tx_A)}")
    for l in tail2_tx_A[:3]: print(f"  A| {l}")
    for l in info_tx_A[:3]:  print(f"  A| {l}")
    _log(f"NodeB TAIL2 RX: {len(tail2_rx_B)}, INFO RX: {len(info_rx_B)}")
    for l in tail2_rx_B[:3]: print(f"  B| {l}")
    for l in info_rx_B[:3]:  print(f"  B| {l}")

    check(len(tail2_tx_A) >= 1, "NodeA transmits Operational (0x04/TAIL2)",
          f"{len(tail2_tx_A)}")
    check(len(info_tx_A) >= 1,  "NodeA transmits Informative (0x05/INFO)",
          f"{len(info_tx_A)}")
    check(len(tail2_rx_B) >= 1, "NodeB receives Operational (0x04/TAIL2)",
          f"{len(tail2_rx_B)}")
    check(len(info_rx_B) >= 1,  "NodeB receives Informative (0x05/INFO)",
          f"{len(info_rx_B)}")

    # Confirm no seq16 collision between 0x04 and 0x05 TX
    seqs_t2 = {seq(l) for l in tail2_tx_A if seq(l) is not None}
    seqs_in = {seq(l) for l in info_tx_A  if seq(l) is not None}
    overlap = seqs_t2 & seqs_in
    check(len(overlap) == 0,
          "0x04 and 0x05 TX use distinct seq16 values",
          f"overlap={overlap}")

    # ── E) TX queue ordering sanity ──────────────────────────────────────────
    section("E) TX queue ordering sanity (priority + fairness)")

    idxA5 = nodeA.snapshot_idx()
    _log("Collecting all TX types on NodeA for 60s...")
    all_tx_A = nodeA.collect_from(idxA5, r'pkt tx.*type=', duration=60)
    _log(f"NodeA all TX: {len(all_tx_A)} lines")
    for l in all_tx_A[:25]: print(f"  A| {l}")

    p0 = [l for l in all_tx_A if re.search(r'type=(CORE|ALIVE)', l)]
    p2 = [l for l in all_tx_A if re.search(r'type=(TAIL1|TAIL2|INFO)', l)]
    check(len(p0) >= 1, "P0 packets (CORE/ALIVE) present", f"count={len(p0)}")
    check(len(p2) >= 1, "P2 packets (TAIL1/TAIL2/INFO) present", f"count={len(p2)}")

    # TAIL1 (BE_HIGH) must immediately follow its paired CORE (no TAIL2/INFO between them).
    # Find each CORE and verify the very next TX is TAIL1.
    core_idx = [i for i, l in enumerate(all_tx_A) if re.search(r'type=CORE', l)]
    if core_idx:
        violations = []
        for ci in core_idx:
            if ci + 1 < len(all_tx_A):
                next_line = all_tx_A[ci + 1]
                if not re.search(r'type=TAIL1', next_line):
                    violations.append((ci, next_line.strip()))
        check(len(violations) == 0,
              "Core_Tail (TAIL1/BE_HIGH) immediately follows CORE (no TAIL2/INFO between)",
              f"violations={violations}")
    else:
        _log("  [SKIP] No CORE in window for TAIL1 ordering check")

    # ── Done ─────────────────────────────────────────────────────────────────
    section("ALL SCENARIOS COMPLETE — no stop conditions hit")
    _log(f"Artifacts: {LOG_A}, {LOG_B}")

    nodeA.close()
    nodeB.close()

if __name__ == "__main__":
    main()
