# tpc-track マニュアル

TPC (Time Projection Chamber) で取得した 1 つの `.root` ファイルから 1 event ごとの 3D 点群を作り、Plotly でインタラクティブに回す一連のパイプラインの使い方。

新しいマシンに持って行って最初からセットアップする想定で書いてある。

## 目次

1. [このプロジェクトで何ができるか](#このプロジェクトで何ができるか)
2. [ディレクトリ構成](#ディレクトリ構成)
3. [システム要件](#システム要件)
4. [Ubuntu でのセットアップ](#ubuntu-でのセットアップ)
5. [ビルド](#ビルド)
6. [日常的な使い方](#日常的な使い方)
7. [テスト](#テスト)
8. [パラメータの意味と調整](#パラメータの意味と調整)
9. [トラブルシューティング](#トラブルシューティング)
10. [既知の制限・注意](#既知の制限注意)

## このプロジェクトで何ができるか

- `.root` (miniTPC GET 電子回路出力) → 3D 点群 CSV を 1 コマンドで生成 (`./build/run_mini`)
- ROOT マクロも cling もいらない。普通の native 実行ファイル
- Plotly ノートブックで 1 event を**ぐりぐり回して見る**
- ↑/↓ キーでイベント切替、ボクセル閾値でゴースト除去、BFGS で直線フィット重ね描き
- C++ 側: vendored miniTPC reader (`loadData` / `convertUVW_mini` / `cleanUVW`) + GET クラスディクショナリ + 我々の `tpctrack_core`
- Python 側: CSV 読込 + ボクセル集計 + 直線フィット + Plotly 描画

設計概要は [`docs/tpcanalysis_overview.md`](tpcanalysis_overview.md) 参照。

## ディレクトリ構成

```
tpc-track/
├── include/
│   ├── tpctrack/       # 我々の ROOT-free ヘッダ
│   └── upstream/       # vendored miniTPC + GET 辞書ヘッダ
├── src/
│   ├── tpctrack/       # 我々の実装
│   └── upstream/       # vendored loadData / convertUVW / cleanUVW + GET 辞書 .cpp
├── tools/
│   └── run_mini_main.cpp   # 実行ファイル本体
├── tests/              # doctest
├── python/             # Plotly viewer + 直線フィット
├── utils/              # 実行時の設定: geometry + ストリップ正規化 CSV
├── docs/
├── TODO/
├── CMakeLists.txt
└── raw_run_files/      # データ (git ignored)
```

## システム要件

| 項目 | 最低 | 推奨 |
| --- | --- | --- |
| OS | Ubuntu 20.04 LTS | Ubuntu 22.04 / 24.04 |
| C++ | C++17 (gcc 7+ / clang 6+) | gcc 11+ |
| CMake | 3.16 | 3.22+ |
| ROOT | 6.32 | 6.36+ |
| Python | 3.9 | 3.11+ |
| ディスク | 3 GB | 10 GB+ |
| メモリ | 4 GB | 8 GB+ |

### Ubuntu 18.04 について

glibc 2.27 が古すぎてプリビルド ROOT と非互換。Docker/Singularity で 22.04 を持ち込むか、ROOT 6.24 をソースビルドする。

## Ubuntu でのセットアップ

### 1. 基本パッケージ

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    python3 python3-venv python3-pip \
    libxpm-dev libxft-dev libxext-dev \
    libgl1-mesa-dev libglu1-mesa-dev
```

### 2. CMake が 3.16 未満なら kitware PPA で上げる

```bash
sudo apt install -y software-properties-common
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt update && sudo apt install -y cmake
```

### 3. ROOT

公式プリビルドが一番楽:

```bash
cd /opt
sudo wget https://root.cern/download/root_v6.36.10.Linux-ubuntu22.04-x86_64-gcc11.4.tar.gz
sudo tar -xzf root_v6.36.10.Linux-*.tar.gz
echo 'source /opt/root/bin/thisroot.sh' >> ~/.bashrc
source ~/.bashrc
root --version   # 6.32+
```

conda / snap 経由でも可。

### 4. Python 3.9+

古ければ `deadsnakes` PPA を追加。

## ビルド

**クローン直後、1 回だけ 2 コマンド:**

```bash
cd tpc-track
cmake -B build -S .
cmake --build build -j
```

これで以下が全部揃う:

- `build/libtpctrack_core.a` — 我々のライブラリ
- `build/libtpc_upstream.{so,dylib}` + `libtpc_upstream_rdict.pcm` — vendored + GET 辞書
- `build/run_mini` — 実行ファイル
- `build/test_geometry`, `test_hit_extraction`, `test_uvw_xyz` — C++ テスト

Python venv も 1 回だけ:

```bash
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install plotly pandas ipywidgets jupyterlab scipy numpy pytest kaleido
```

## 日常的な使い方

### Step A. `.root` → 点群 CSV

**プロジェクトルートが cwd で実行** (`utils/` 参照のため):

```bash
# 自動出力 (<入力>_points.csv が入力の隣に)
./build/run_mini raw_run_files/CoBo_2026-04-06_0001.root

# 出力パス明示
./build/run_mini raw_run_files/CoBo_2026-04-06_0001.root /tmp/hoge.csv
```

CSV カラム: `event_id, x_mm, y_mm, z_mm, charge`。

パイプラインのデフォルト値は `tools/run_mini_main.cpp` の `constexpr` ブロック: `kNorm=true, kClean=true, kHitThreshold=30.0, kMaxChargeRatio=2.0`。変えたければここを編集して再ビルド。

### Step B. Jupyter で 3D 可視化

```bash
.venv/bin/jupyter lab python/viewer3d.ipynb
```

3 つ目のセルでスライダ群が出る:

| スライダ | 意味 |
| --- | --- |
| `event_id` | ↑/↓ で ±1、数字直接入力も可 |
| `voxel [mm]` | ボクセル 1 辺 (0 で filter off) |
| `min_count` | ボクセル最小ヒット数 |
| `max pts` | 描画最大点数 |
| `n lines` | 直線フィット本数 (0 で off) |
| `fit thresh` | inlier 判定距離 [mm] |

最初は `voxel=0, min_count=1` で素のまま → ノイズ多いなら `voxel=1, min_count=10` で絞る。

## テスト

```bash
# C++ (doctest)
cd build && ctest --output-on-failure

# Python (pytest)
.venv/bin/python -m pytest python/tests -q
```

## パラメータの意味と調整

### `tools/run_mini_main.cpp` の `constexpr`

```cpp
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;    // ADC above cleaned baseline
constexpr double kMaxChargeRatio = 2.0;   // 3-plane charge agreement factor
```

- `kHitThreshold` を下げる → ヒット数増 (ノイズ拾う)、上げる → 弱トラック落とす
- `kMaxChargeRatio=2.0` は「3 面の電荷が 2 倍以内で整合」

### Viewer のボクセル閾値

event 0 (raw=35k) でのスキャン:

| `voxel=1 mm`, `min_count=N` | 残る割合 |
| --- | --- |
| 5 | ~97 % |
| 10 | ~83 % |
| 20 | ~38 % |
| 50 | <1 % |

20 前後から弱いトラックも削れ始める。

## トラブルシューティング

### `./build/run_mini` が `convertUVW_mini::openSpecFile failed` で落ちる

cwd が違う。`utils/new_geometry_mini_eTPC.dat` を cwd から相対参照するので**プロジェクトルートで実行**する必要あり。

### `cmake -B build` で ROOT が見つからない

`thisroot.sh` がソースされていない。

```bash
source /opt/root/bin/thisroot.sh
```

で ROOTSYS が設定された後に再度 cmake。

### pytest で `ModuleNotFoundError: line_fit`

プロジェクトルートから実行する:

```bash
.venv/bin/python -m pytest python/tests -q
```

### Plotly が重い / 固まる

`max pts` を 5000 以下に、あるいは voxel / min_count で点数を削る。

## 既知の制限・注意

- **3D 点群は cartesian product 由来のゴーストを含む**: ボクセル閾値がまず第一の対策。物理的に精緻な ghost rejection は未実装。
- **Z 軸の符号は trigger_delay 基準**: `z_mm = (time_bin - 256) * drift_velocity * bin_width`。正負はビーム方向の定義次第なので、物理解析前に突き合わせて確認。
- **miniTPC only**: ELITPC サポートは vendored と一緒に消した。
