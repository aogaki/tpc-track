# tpc-track

Pipeline for turning one miniTPC `.root` file (GET electronics output) into a
3D point cloud and viewing a single event interactively in Plotly.

This README is the **setup-and-use manual in English**. The Japanese version
lives at [`docs/manual.md`](docs/manual.md) and is the primary source; keep
both in sync when making changes that affect the workflow.

---

## Contents

1. [What the project does](#what-the-project-does)
2. [Layout](#layout)
3. [System requirements](#system-requirements)
4. [Ubuntu setup](#ubuntu-setup)
5. [Build](#build)
6. [Daily workflow](#daily-workflow)
7. [Tests](#tests)
8. [Parameters and tuning](#parameters-and-tuning)
9. [Troubleshooting](#troubleshooting)
10. [Known limitations](#known-limitations)

---

## What the project does

- `.root` (miniTPC, GET electronics) → 3D point-cloud CSV via a single
  native executable (`./build/run_mini`). No ROOT macros, no environment
  variables, no interactive cling.
- Plotly notebook (`python/viewer3d.ipynb`) spins one event around in 3D
  with event navigation, voxel-coincidence filter for ghost suppression,
  and an iterative BFGS line fitter.
- C++ side reuses the vendored miniTPC reader classes (`loadData`,
  `convertUVW_mini`, `cleanUVW`) plus the GET class dictionary. Hit
  extraction, UVW → XYZ geometry and CSV output are in our own
  `tpctrack_core` library.
- Python side handles CSV loading, voxel aggregation, BFGS line fitting,
  and Plotly rendering.

For architecture notes see [`docs/tpcanalysis_overview.md`](docs/tpcanalysis_overview.md).

---

## Layout

```
tpc-track/
├── include/
│   ├── tpctrack/       # our ROOT-free library headers
│   └── upstream/       # vendored miniTPC classes + GET dictionary headers
├── src/
│   ├── tpctrack/       # our implementations
│   └── upstream/       # vendored loadData / convertUVW / cleanUVW + GET dict sources
├── tools/
│   └── run_mini_main.cpp   # the standalone executable
├── tests/              # doctest suites
├── python/             # Plotly viewer + line fit
├── utils/              # runtime config: geometry + strip normalization CSV
├── docs/               # Japanese manual + architecture overview
├── TODO/               # project plan & milestones
├── CMakeLists.txt
├── README.md           # you are here
└── raw_run_files/      # data (git-ignored)
```

---

## System requirements

| Item | Minimum | Recommended |
| --- | --- | --- |
| OS | Ubuntu 20.04 LTS | Ubuntu 22.04 / 24.04 |
| C++ compiler | C++17 (gcc 7+ / clang 6+) | gcc 11+ |
| CMake | 3.16 | 3.22+ |
| ROOT | 6.32 | 6.36+ |
| Python | 3.9 | 3.11+ |
| Disk | 3 GB | 10 GB+ |
| RAM | 4 GB | 8 GB+ (Pandas reads a 13 M-point CSV) |

### About Ubuntu 18.04

18.04 is **not straightforward**: glibc 2.27 is too old for prebuilt ROOT
binaries, and CMake / Python need PPAs. Use a 22.04 Docker/Singularity
image instead, or build ROOT 6.24 from source.

---

## Ubuntu setup

### 1. Base packages

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    python3 python3-venv python3-pip \
    libxpm-dev libxft-dev libxext-dev \
    libgl1-mesa-dev libglu1-mesa-dev
```

### 2. CMake 3.16+ (bump via kitware PPA if apt is too old)

```bash
sudo apt install -y software-properties-common
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt update && sudo apt install -y cmake
```

### 3. ROOT

Install the ROOT prebuilt tarball for your Ubuntu release (recommended),
or use `conda install -c conda-forge root=6.36`. Make sure `root
--version` reports 6.32 or newer and `thisroot.sh` is sourced.

```bash
cd /opt
sudo wget https://root.cern/download/root_v6.36.10.Linux-ubuntu22.04-x86_64-gcc11.4.tar.gz
sudo tar -xzf root_v6.36.10.Linux-*.tar.gz
echo 'source /opt/root/bin/thisroot.sh' >> ~/.bashrc
source ~/.bashrc
```

### 4. Python 3.9+ (via `deadsnakes` PPA if needed)

---

## Build

**One command from a fresh clone:**

```bash
cd tpc-track
cmake -B build -S .
cmake --build build -j
```

This produces:

- `build/libtpctrack_core.a` — our library
- `build/libtpc_upstream.dylib` / `.so` — vendored code + GET dictionary
- `build/libtpc_upstream_rdict.pcm` — ROOT auto-loads this at runtime
- `build/run_mini` — the executable
- `build/test_geometry`, `build/test_hit_extraction`, `build/test_uvw_xyz`

Create the Python environment once:

```bash
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install plotly pandas ipywidgets jupyterlab scipy numpy pytest kaleido
```

---

## Daily workflow

### Step A. `.root` → point-cloud CSV

Run from the project root so `utils/` resolves:

```bash
# Output auto-named <input>_points.csv next to the input
./build/run_mini raw_run_files/CoBo_2026-04-06_0001.root

# Explicit output path
./build/run_mini raw_run_files/CoBo_2026-04-06_0001.root /tmp/custom.csv
```

Columns: `event_id, x_mm, y_mm, z_mm, charge`. Internal defaults in
`tools/run_mini_main.cpp`: `norm=true`, `clean=true`, hit threshold 30 ADC,
3-plane charge-ratio filter 2.0. Edit the `constexpr` block and rebuild
if you need to change them.

### Step B. Interactive 3D viewer

```bash
.venv/bin/jupyter lab python/viewer3d.ipynb
```

Run the cells top to bottom; the third cell exposes widgets:

| Widget | Role |
| --- | --- |
| `event_id` | ↑/↓ to step ±1, or type a number directly |
| `voxel [mm]` | Voxel edge length; 0 disables the coincidence filter |
| `min_count` | Minimum combos per voxel to keep |
| `max pts` | Safety cap on rendered markers (top-charge kept) |
| `n lines` | Fitted lines to overlay (0 = no fit) |
| `fit thresh` | Inlier distance [mm] for line fitting |

Start with `voxel=0, min_count=1`; tighten later (`voxel=1, min_count=10`).

---

## Tests

```bash
# C++ (doctest)
cd build && ctest --output-on-failure

# Python (pytest)
.venv/bin/python -m pytest python/tests -q
```

---

## Parameters and tuning

### `tools/run_mini_main.cpp` `constexpr` block

```cpp
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;
constexpr double kMaxChargeRatio = 2.0;
```

- `kHitThreshold`: lower → more hits (noise); higher → weaker tracks lost.
- `kMaxChargeRatio = 2.0`: require 3-plane charges within 2×.

### Voxel filter in the viewer

Scan on event 0 of `CoBo_2026-04-06_0001` (raw = 35 k):

| `voxel=1 mm`, `min_count=N` | points kept |
| --- | --- |
| 5 | ~97 % |
| 10 | ~83 % |
| 20 | ~38 % |
| 50 | <1 % |

---

## Troubleshooting

### `./build/run_mini` reports "convertUVW_mini::openSpecFile failed"

You launched from the wrong directory. `utils/new_geometry_mini_eTPC.dat`
is looked up relative to cwd; run from the project root.

### ROOT `find_package` fails during `cmake -B build`

`thisroot.sh` isn't sourced. Source it and re-run `cmake`.

### `import line_fit` fails under pytest

Run pytest from the project root:

```bash
.venv/bin/python -m pytest python/tests -q
```

### Plotly sluggish / freezes

Reduce `max pts` (below 5 000) or enable the voxel filter.

---

## Known limitations

- **3D points include cartesian-product ghosts.** The voxel filter is the
  first defence; smarter physics-based rejection is future work.
- **Z convention:** `z_mm = (time_bin - 256) * drift_velocity * bin_width`.
  Sign depends on the beam direction; cross-check before physics use.
- **miniTPC only.** ELITPC support was removed along with its vendored
  files to keep the tree lean.
