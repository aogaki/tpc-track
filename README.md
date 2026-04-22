# tpc-track

Pipeline for turning one miniTPC `.root` file (GET electronics output) into a
3D point cloud and viewing a single event interactively in Plotly.

This README is the **setup-and-use manual in English**. The Japanese version
lives at [`docs/manual.md`](docs/manual.md) and is the primary source; keep
both in sync when making changes that affect the workflow.

---

## Contents

1. [What the project does](#what-the-project-does)
2. [System requirements](#system-requirements)
3. [Ubuntu setup](#ubuntu-setup)
4. [Getting the project](#getting-the-project)
5. [One-time build](#one-time-build)
6. [Daily workflow](#daily-workflow)
7. [Verification (M4 reference check)](#verification-m4-reference-check)
8. [Tests](#tests)
9. [Parameters and tuning](#parameters-and-tuning)
10. [Troubleshooting](#troubleshooting)
11. [Known limitations](#known-limitations)

---

## What the project does

- `.root` (miniTPC, GET electronics) → 3D point-cloud CSV.
- Plotly notebook spins one event around in 3D.
- Event navigation with arrow keys, voxel-coincidence filter to suppress
  ghosts, iterative BFGS line fitter to draw detected tracks on top.
- C++ side reuses the upstream `tpcanalysis-main` classes (`loadData`,
  `convertUVW_mini`, `cleanUVW`); our own code takes care of hit
  extraction, UVW → XYZ conversion, and CSV output.
- Python side handles CSV loading, voxel aggregation, BFGS line fitting,
  and Plotly rendering.

For deeper architecture notes see [`docs/tpcanalysis_overview.md`](docs/tpcanalysis_overview.md).

---

## System requirements

| Item | Minimum | Recommended |
| --- | --- | --- |
| OS | Ubuntu 20.04 LTS | Ubuntu 22.04 / 24.04 |
| C++ compiler | C++17 (gcc 7+ / clang 6+) | gcc 11+ |
| CMake | 3.14 (needs FetchContent) | 3.22+ |
| ROOT | 6.32 | 6.36+ |
| Python | 3.9 | 3.11+ |
| Disk | 3 GB (venv + ROOT + one raw file) | 10 GB+ |
| RAM | 4 GB | 8 GB+ (to hold the 13 M-point CSV in pandas) |

### About Ubuntu 18.04

18.04 is **not straightforward**:

- gcc 7.5 technically supports C++17, but ROOT 6.32+ has no prebuilt
  binaries for 18.04 (glibc 2.27 is too old).
- CMake 3.10 lacks `FetchContent`. Bump to 3.22 via the kitware PPA below.
- Python 3.6 is too old; install 3.11 via the `deadsnakes` PPA.

If you must target 18.04, the practical options are:

- **Older ROOT (6.24-ish)**: then revert `tpcanalysis-main/dict/CMakeLists.txt`
  `c++17` back to `c++14` (the upstream default); our `tpctrack_core` still
  needs C++17 which gcc 7.5 can provide.
- **Build ROOT from source** (4–8 hours, not recommended).
- **Docker / Singularity with a 22.04 image**: the cleanest option —
  [`tpcanalysis-main/howto.txt`](tpcanalysis-main/howto.txt) already does this
  for the upstream graw→root step.

The rest of this README targets 20.04 / 22.04 / 24.04.

---

## Ubuntu setup

### 1. Base packages

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    python3 python3-venv python3-pip \
    libxpm-dev libxft-dev libxext-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    imagemagick                       # used by the PNG diff script
```

The X/GL libraries are ROOT graphics dependencies. `run_mini.cpp` runs
headless, but `verify_plot.cpp` writes PNGs via `TCanvas::Print` and needs
them.

### 2. Up-to-date CMake (only if apt ships <3.14)

```bash
sudo apt install -y software-properties-common
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt update && sudo apt install -y cmake
cmake --version   # expect 3.22+
```

### 3. Install ROOT

**(A) Official prebuilt tarball — recommended**

```bash
# Pick the tarball matching your distro at https://root.cern/install/all_releases/
# Example: ROOT 6.36.10 for Ubuntu 22.04
cd /opt
sudo wget https://root.cern/download/root_v6.36.10.Linux-ubuntu22.04-x86_64-gcc11.4.tar.gz
sudo tar -xzf root_v6.36.10.Linux-*.tar.gz
echo 'source /opt/root/bin/thisroot.sh' >> ~/.bashrc
source ~/.bashrc
root --version
```

**(B) conda (self-contained env)**

```bash
conda create -n tpctrack -c conda-forge root=6.36 python=3.11
conda activate tpctrack
```

**(C) snap (easiest, but versions lag)**

```bash
sudo snap install root-framework
```

Verify with `root --version`; it must report 6.32 or newer.

### 4. Python 3.9+

```bash
python3 --version
```

If too old:

```bash
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install -y python3.11 python3.11-venv
# Then read "python3" below as "python3.11".
```

---

## Getting the project

```bash
cd ~/work                       # wherever you keep projects
git clone <tpc-track URL> tpc-track
cd tpc-track
```

Layout:

```
tpc-track/
├── tpcanalysis-main/           # upstream copy (vendored)
│   ├── dict/                   # GET dictionary library (build once)
│   ├── runmacro_mini.cpp       # original upstream macro (don't edit)
│   ├── run_mini.cpp            # OUR production macro
│   └── verify_plot.cpp         # OUR M4 verification macro
├── include/tpctrack/           # our headers
├── src/                        # our implementation
├── tests/                      # C++ tests (doctest)
├── python/
│   ├── viewer3d.py             # Plotly rendering
│   ├── line_fit.py             # 3D line fitter (BFGS)
│   ├── viewer3d.ipynb          # interactive notebook
│   └── tests/                  # Python tests (pytest)
├── scripts/
│   └── compare_png.py          # pixel-level PNG diff
├── docs/                       # Japanese manual, verification notes, overview
└── TODO/                       # roadmap and per-milestone specs
```

---

## One-time build

### 1. GET dictionary library (C++)

Required so ROOT macros can see `GDataFrame` etc. Produces
`libMyLib.so` (Linux) / `libMyLib.dylib` (macOS).

```bash
cd tpcanalysis-main/dict
cmake -B build -S .
cmake --build build -j
ls build/libMyLib.so   # or .dylib on macOS
```

The CMakeLists bakes the absolute build path into the library's
`install_name`, so no `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` is needed
later.

### 2. `tpctrack_core` + C++ tests

```bash
cd ../..                                  # back to project root
cmake -B build -S .
cmake --build build -j
cd build && ctest --output-on-failure     # expect 3 targets / 21 cases green
cd ..
```

### 3. Python venv

```bash
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install plotly pandas ipywidgets jupyterlab scipy numpy pytest kaleido
.venv/bin/python -m pytest python/tests -q   # expect 5 cases green
```

`.venv/` is git-ignored.

---

## Daily workflow

### Step A. `.root` → point-cloud CSV

**Single-argument API. File path is the only input:**

```bash
cd tpcanalysis-main

# Interpreter mode (fast startup, ~20 s for 142 events)
ROOT_INCLUDE_PATH=../include root -b -q \
    'run_mini.cpp("../raw_run_files/CoBo_2026-04-06_0001.root")'
```

For repeated runs:

```bash
# ACLiC mode — ~15 s cold compile, ~12 s with cached .so
ROOT_INCLUDE_PATH=../include root -b -q \
    -e 'gSystem->AddLinkedLibs("'$PWD'/dict/build/libMyLib.so");' \
    'run_mini.cpp+("../raw_run_files/CoBo_2026-04-06_0001.root")'
```

Output: `<input>_points.csv` next to the input `.root`, with columns
`event_id, x_mm, y_mm, z_mm, charge`.

Internal pipeline defaults (hard-coded in `run_mini.cpp`):

- `norm = true`, `clean = true`
- hit threshold 30 ADC, 3-plane charge-ratio filter 2.0

Adjust the `constexpr` values at the top of `run_mini.cpp` if you really
need to change them.

### Step B. Interactive 3D viewer

```bash
cd <project root>
.venv/bin/jupyter lab python/viewer3d.ipynb
```

Run the cells top to bottom. The third cell exposes the widgets:

| Widget | Role |
| --- | --- |
| `event_id` | Click the box, then ↑/↓ to step ±1 or type a number directly. |
| `voxel [mm]` | Voxel edge length (0 disables the filter). |
| `min_count` | Drop voxels that received fewer than this many combos. |
| `max pts` | Safety cap on rendered markers (highest-charge retained). |
| `n lines` | Fitted lines to overlay (0 = no fit). |
| `fit thresh` | Inlier distance [mm] for line fitting. |

**Sensible starting values**: `voxel=0, min_count=1` (raw view). Tighten
later — try `voxel=1, min_count=10` for gentle ghost rejection.

**When to restart the kernel**: only when you edit `viewer3d.py` or
`line_fit.py`. Changing slider values is live.

---

## Verification (M4 reference check)

To confirm the new pipeline matches the upstream `runmacro_mini -norm0
-clean0` output pixel-for-pixel, follow the procedure in
[`docs/verify.md`](docs/verify.md). Short version:

1. Generate reference PNGs with `runmacro_mini`.
2. Generate comparison PNGs with `verify_plot.cpp`.
3. Diff with `scripts/compare_png.py`.

Same ROOT version on both sides → 0 pixel diff. Different ROOT versions
typically diverge by 10–15 % on anti-aliasing / palette noise only — not
a pipeline issue.

---

## Tests

```bash
# C++ (doctest via CMake)
cd build && ctest --output-on-failure

# Python (pytest)
.venv/bin/python -m pytest python/tests -q
```

CI or sanity checks: just those two commands must pass.

---

## Parameters and tuning

### `run_mini.cpp` `constexpr`s

```cpp
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;   // ADC above cleaned baseline
constexpr double kMaxChargeRatio = 2.0;  // 3-plane charge agreement
```

- Lower `kHitThreshold` → more hits (including noise); higher → weaker
  tracks get lost. 30 ADC on cleaned signal is comfortably above noise.
- `kMaxChargeRatio = 2.0` means "all three planes agree within 2×".
  Raise to let more combinations through, lower to be stricter.

### Voxel thresholds in `viewer3d`

Scan on event 0 of `CoBo_2026-04-06_0001` (raw = 35 k):

| `voxel=1 mm`, `min_count=N` | points kept |
| --- | --- |
| 5 | ~97 % |
| 10 | ~83 % |
| 20 | ~38 % |
| 50 | <1 % |

Around 20 the filter starts eating into real track tails. The default
(filter off, `voxel=0`) is designed for "look first, tighten later".

### Line fitting

- `n_lines`: start at 2–3; increase if tracks are missing.
- `fit thresh`: 3 mm by default; slightly wider than pad pitch (1 mm)
  and z bin width (~0.3 mm).

---

## Troubleshooting

### `Library not loaded: @rpath/libMyLib.so`

Rebuild the dict library — the fixed `install_name` gets re-baked:

```bash
cd tpcanalysis-main/dict && rm -rf build && cmake -B build -S . && cmake --build build
```

### ACLiC (`+`) fails with `GET::GDataFrame::Class()` not found

You forgot `-e 'gSystem->AddLinkedLibs("/abs/path/to/libMyLib.so");'`,
or the dict library isn't built yet.

### `compare_png.py` reports 10–15 % diff

Different ROOT versions used for reference vs. new generation. Rerun
`runmacro_mini` under the current ROOT and re-diff — expect 0 px.
Details in [`docs/verify.md`](docs/verify.md).

### Plotly is sluggish / freezes

Reduce `max pts` (below 5 000) or enable the voxel filter
(`voxel>0, min_count>1`). Browsers start stuttering above ~50 000 points
per event.

### `ModuleNotFoundError: line_fit` in pytest

Run pytest from the **project root** so the `sys.path.insert` at the top
of the test file resolves correctly:

```bash
.venv/bin/python -m pytest python/tests -q
```

### CMake warns about `cmake_minimum_required`

doctest (pulled by `FetchContent`) asks for a pre-3.5 policy, CMake 4
nags. We set `CMAKE_POLICY_VERSION_MINIMUM=3.5` already — warning only,
no functional issue.

### ROOT won't run on Ubuntu 18.04

glibc 2.27 is too old for modern ROOT binaries. Use Docker/Singularity
with a 22.04 image, pin ROOT to 6.24, or upgrade the OS.

---

## Known limitations

- **Hit count differs between interpreter and ACLiC** (~946 k vs.
  ~1.39 M): cling FP vs. -O2 native FP. Treat ACLiC as canonical for
  numerical runs.
- **3D points include cartesian-product ghosts.** The voxel filter is
  the first line of defence. More physics-aware rejection is future
  work.
- **`hitsToPoints` currently trusts any combination passing the charge
  filter.** Fine for visualisation; be cautious before feeding points
  downstream into precision fits.
- **Z sign convention depends on `TRIGGER DELAY`.** `z_mm = (time_bin -
  256) * drift_velocity * bin_width`. Cross-check with your beam
  direction before doing physics.
- **ELITPC is not supported.** Upstream files remain in the tree, but
  the production pipeline targets miniTPC only.
- **Changing `run_mini.cpp` `constexpr`s changes all downstream
  numbers.** Record configuration in git so results are reproducible.
