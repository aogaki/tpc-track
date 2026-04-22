#!/usr/bin/env python3
"""Pixel-by-pixel PNG diff between two directories.

Usage:
    python3 scripts/compare_png.py <dir_a> <dir_b> [--pattern "*_u.png"]

Prints per-file status:
    OK                                  — identical pixels
    DIFF (N/T px = P%, max_delta=D)     — differs; N of T pixels differ,
                                          max per-channel int delta = D
    MISSING in B                        — file present in A but not B
    SHAPE MISMATCH                      — image dimensions differ

Exit code: 0 if all compared files are OK; 1 otherwise.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def diff_pair(path_a: Path, path_b: Path):
    a = np.array(Image.open(path_a).convert("RGBA"))
    b = np.array(Image.open(path_b).convert("RGBA"))
    if a.shape != b.shape:
        return None
    total = a.shape[0] * a.shape[1]
    d = np.abs(a.astype(int) - b.astype(int))
    differing = int(np.any(d > 0, axis=-1).sum())
    max_delta = int(d.max()) if d.size else 0
    return total, differing, max_delta


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dir_a", type=Path)
    parser.add_argument("dir_b", type=Path)
    parser.add_argument("--pattern", default="*.png")
    args = parser.parse_args()

    files = sorted(f.name for f in args.dir_a.glob(args.pattern))
    if not files:
        print(f"No files matching {args.pattern} in {args.dir_a}")
        return 1

    any_diff = False
    for name in files:
        pa, pb = args.dir_a / name, args.dir_b / name
        if not pb.exists():
            print(f"{name}: MISSING in B")
            any_diff = True
            continue
        result = diff_pair(pa, pb)
        if result is None:
            print(f"{name}: SHAPE MISMATCH")
            any_diff = True
            continue
        total, differing, max_delta = result
        if differing == 0:
            print(f"{name}: OK")
        else:
            pct = 100.0 * differing / total
            print(
                f"{name}: DIFF ({differing}/{total} px = {pct:.2f}%, "
                f"max_delta={max_delta})"
            )
            any_diff = True

    return 1 if any_diff else 0


if __name__ == "__main__":
    sys.exit(main())
