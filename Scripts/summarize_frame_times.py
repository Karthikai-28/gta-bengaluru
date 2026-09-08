#!/usr/bin/env python3
"""Summarize a UE CSV profiler capture after a warm-up exclusion. Times must be milliseconds."""
import argparse
import csv
import math
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv")
    parser.add_argument("--column", default="FrameTime", help="Exact frame-time column emitted by the selected UE build")
    parser.add_argument("--warmup-seconds", type=float, default=30)
    args = parser.parse_args()
    samples = []
    elapsed_ms = 0.0
    with open(args.csv, newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or args.column not in reader.fieldnames:
            parser.error(f"Column {args.column!r} missing; available: {reader.fieldnames}")
        for row in reader:
            try:
                value = float(row[args.column])
            except (ValueError, TypeError, KeyError):
                continue  # UE CSV metadata/footer is not frame data.
            if not math.isfinite(value) or value <= 0:
                continue
            elapsed_ms += value
            if elapsed_ms > args.warmup_seconds * 1000:
                samples.append(value)
    if not samples:
        parser.error("No frames remain after warm-up")
    ordered = sorted(samples)
    p95 = ordered[math.ceil(len(ordered) * 0.95) - 1]
    duration = sum(samples) / 1000
    print(f"Measured {duration:.1f}s, {len(samples)} frames; mean {sum(samples)/len(samples):.2f}ms; p95 {p95:.2f}ms")
    print("Timing gate:", "PASS" if p95 <= 33.3 else "FAIL", "(p95 <= 33.3ms)")
    print("Duration gate:", "PASS" if duration >= 600 else "FAIL", "(600 measured seconds required)")
    return 0 if p95 <= 33.3 and duration >= 600 else 1


if __name__ == "__main__":
    sys.exit(main())
