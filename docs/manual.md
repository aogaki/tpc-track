# tpc-track マニュアル

TPC (Time Projection Chamber) で取得した 1 つの `.root` ファイルから 1 event ごとの 3D 点群を作り、Plotly でインタラクティブに回す一連のパイプラインの使い方。

セッションでのクイック参照ではなく、**新しいマシンに持って行って最初からセットアップする** 想定で書いてある。

---

## 目次

1. [このプロジェクトで何ができるか](#このプロジェクトで何ができるか)
2. [システム要件](#システム要件)
3. [Ubuntu でのセットアップ](#ubuntu-でのセットアップ)
4. [プロジェクトの取得](#プロジェクトの取得)
5. [1 回だけの初期ビルド](#1-回だけの初期ビルド)
6. [日常的な使い方](#日常的な使い方)
7. [検証 (M4 リファレンス照合)](#検証-m4-リファレンス照合)
8. [テスト](#テスト)
9. [パラメータの意味と調整](#パラメータの意味と調整)
10. [トラブルシューティング](#トラブルシューティング)
11. [既知の制限・注意](#既知の制限注意)

---

## このプロジェクトで何ができるか

- `.root` (miniTPC の GET 電子回路出力) → 3D 点群 CSV
- Plotly ノートブックで 1 event を **ぐりぐり回して見る**
- イベント間を ↑/↓ キーで切替、ボクセル閾値でゴースト除去、直線フィットで軌跡抽出
- C++ 側で upstream (`tpcanalysis-main`) の `loadData` / `convertUVW_mini` / `cleanUVW` を再利用し、ヒット抽出・UVW→XYZ 変換・CSV 出力を我々のコードで担う
- Python 側で CSV 読み込み・ボクセル集計・BFGS 直線フィット・Plotly 可視化

詳細なアーキテクチャは [`tpcanalysis_overview.md`](tpcanalysis_overview.md) を参照。

---

## システム要件

| 項目 | 最低 | 推奨 |
| --- | --- | --- |
| OS | Ubuntu 20.04 LTS | Ubuntu 22.04 / 24.04 |
| C++ コンパイラ | C++17 対応 (gcc 7+ / clang 6+) | gcc 11+ |
| CMake | 3.14 (FetchContent が要る) | 3.22+ |
| ROOT | 6.32 | 6.36+ |
| Python | 3.9 | 3.11+ |
| ディスク | 3 GB (venv + ROOT + 生データ) | 10 GB+ |
| メモリ | 4 GB | 8 GB+ (13M 点 CSV を pandas に載せるため) |

### Ubuntu 18.04 について

18.04 は以下が古すぎて**そのままでは動かない**:

- gcc 7.5 は C++17 に一応対応しているが ROOT 6.32 以降はビルド済みバイナリが提供されていない (glibc 要件)
- CMake 3.10 → FetchContent 非対応。後述の PPA で 3.22 に上げる必要あり
- Python 3.6 → deadsnakes PPA で 3.11 を別途入れる必要あり

18.04 でやる場合の選択肢:

- **ROOT は古めのバージョン (6.24 前後)**: その場合 `tpcanalysis-main/dict/CMakeLists.txt` の `c++17` を `c++14` に戻せば (upstream 元々その設定) 通る。我々の `tpctrack_core` は C++17 必須のまま動く。
- **もしくは ROOT をソースからビルド** (4-8 時間コース) — 推奨しない
- **Docker/Singularity で 22.04 環境を持ち込む**: 実運用はこれが楽。[`tpcanalysis-main/howto.txt`](../tpcanalysis-main/howto.txt) でも singularity を使っている。

以下の手順は 20.04 / 22.04 / 24.04 向け。

---

## Ubuntu でのセットアップ

### 1. 基本パッケージ

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    python3 python3-venv python3-pip \
    libxpm-dev libxft-dev libxext-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    imagemagick                       # PNG 比較用
```

`libxpm-dev` 〜 `libgl1-mesa-dev` は ROOT のグラフィックス依存。`run_mini.cpp` はヘッドレスでも動くが `verify_plot.cpp` は `TCanvas::Print` で PNG 出力するので必要。

### 2. CMake が古い場合 (Ubuntu 18.04 / 20.04 で 3.14 未満のとき)

```bash
sudo apt install -y software-properties-common
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt update && sudo apt install -y cmake
cmake --version   # 3.22+ が出ればOK
```

### 3. ROOT のインストール

**(A) 公式プリビルド (推奨)**

ROOT 公式の Ubuntu 向けバイナリを使うのが一番速い:

```bash
# https://root.cern/install/all_releases/ から該当 Ubuntu 版を探す
# 例: Ubuntu 22.04 用 ROOT 6.36.10
cd /opt
sudo wget https://root.cern/download/root_v6.36.10.Linux-ubuntu22.04-x86_64-gcc11.4.tar.gz
sudo tar -xzf root_v6.36.10.Linux-*.tar.gz
echo 'source /opt/root/bin/thisroot.sh' >> ~/.bashrc
source ~/.bashrc
root --version
```

**(B) conda (隔離環境が欲しいとき)**

```bash
conda create -n tpctrack -c conda-forge root=6.36 python=3.11
conda activate tpctrack
```

**(C) snap (手軽だがバージョン選択に難)**

```bash
sudo snap install root-framework
```

どれにしろ `root --version` で 6.32 以上が出ることを確認。

### 4. Python (3.9 以上) の確認

```bash
python3 --version  # 3.9+ が出れば OK
```

古ければ `deadsnakes` PPA を追加:

```bash
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install -y python3.11 python3.11-venv
# 以降の `python3` は `python3.11` に読み替え
```

---

## プロジェクトの取得

```bash
cd ~/work                    # 好きな場所
git clone <tpc-track の URL> tpc-track
cd tpc-track
```

ディレクトリ構成:

```
tpc-track/
├── tpcanalysis-main/          # upstream コピー (vendored)
│   ├── dict/                  # GET dict ライブラリ (要ビルド)
│   ├── runmacro_mini.cpp      # 既存マクロ (変更しない)
│   ├── run_mini.cpp           # 本番マクロ (新規作成)
│   └── verify_plot.cpp        # 検証マクロ (新規作成)
├── include/tpctrack/          # 我々のヘッダ
├── src/                       # 我々の実装
├── tests/                     # C++ テスト (doctest)
├── python/
│   ├── viewer3d.py            # Plotly 可視化
│   ├── line_fit.py            # 3D 直線フィット
│   ├── viewer3d.ipynb         # インタラクティブノート
│   └── tests/                 # Python テスト (pytest)
├── scripts/
│   └── compare_png.py         # UVW PNG 差分比較
├── docs/                      # このマニュアル含む
└── TODO/                      # 計画とマイルストーン
```

---

## 1 回だけの初期ビルド

### 1. GET ディクショナリライブラリ (C++)

ROOT マクロが `GDataFrame` 等を扱うために必要な `libMyLib.dylib` (Linux では `libMyLib.so`) を作る。

```bash
cd tpcanalysis-main/dict
cmake -B build -S .
cmake --build build -j
ls build/libMyLib.so   # Linux の場合 (macOS は .dylib)
```

install_name は CMakeLists 側で絶対パスが焼かれるので、以降 DYLD_LIBRARY_PATH / LD_LIBRARY_PATH を触る必要はない。

### 2. tpctrack_core + テスト

```bash
cd ../..                                  # プロジェクトルートに戻る
cmake -B build -S .
cmake --build build -j
cd build && ctest --output-on-failure     # 3 target / 21 ケース green を確認
cd ..
```

### 3. Python venv

```bash
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install plotly pandas ipywidgets jupyterlab scipy numpy pytest kaleido
.venv/bin/python -m pytest python/tests -q   # 5 ケース green を確認
```

`.venv/` は `.gitignore` 済み。

---

## 日常的な使い方

### Step A. `.root` → 点群 CSV を作る

**miniTPC の 1 ファイルを渡すだけ** (ファイル名 1 引数が本番マクロの唯一の API):

```bash
cd tpcanalysis-main

# インタプリタモード (起動早い、~20秒で 142 event)
ROOT_INCLUDE_PATH=../include root -b -q \
    'run_mini.cpp("../raw_run_files/CoBo_2026-04-06_0001.root")'
```

初回のみ:

```bash
# ACLiC モード (.so キャッシュで 2 回目以降 ~12 秒、初回は ~15 秒)
ROOT_INCLUDE_PATH=../include root -b -q \
    -e 'gSystem->AddLinkedLibs("'$PWD'/dict/build/libMyLib.so");' \
    'run_mini.cpp+("../raw_run_files/CoBo_2026-04-06_0001.root")'
```

出力: 入力 `.root` と同じディレクトリに `<ファイル名>_points.csv` が作られる。
カラム: `event_id, x_mm, y_mm, z_mm, charge`。

内部パラメータ (固定):

- `norm = true`, `clean = true` (M5 で固定)
- ヒット閾値 30 ADC, ghost 除去 (3 面 charge 比) 2.0

これらを変えたくなったら `run_mini.cpp` 先頭の `constexpr` を編集。

### Step B. Jupyter で 3D 可視化

```bash
cd <プロジェクトルート>
.venv/bin/jupyter lab python/viewer3d.ipynb
```

ブラウザが開いたら上からセルを実行。3 つ目のセルで 4 段のスライダと描画枠が出る。

スライダ:

| スライダ | 意味 |
| --- | --- |
| `event_id` | 見たいイベント番号。クリックしてから ↑/↓ で ±1、直接タイプも OK |
| `voxel [mm]` | ボクセル 1 辺 (0 で filter off) |
| `min_count` | ボクセル内の最小ヒット数。これ未満は drop |
| `max pts` | 描画する最大点数 (charge 上位で clip) |
| `n lines` | 直線フィット本数 (0 で off, 1-6 で重ね描き) |
| `fit thresh` | inlier 判定距離 [mm] |

**おすすめ初期値**: `voxel=0`, `min_count=1` (= 素のまま見る)。絞りたくなったら `voxel=1, min_count=10`。

**カーネル再起動の必要なとき**:

- `viewer3d.py` や `line_fit.py` を編集したあと → カーネル再起動 → 全セル再実行
- スライダの値を変えただけなら再起動不要

---

## 検証 (M4 リファレンス照合)

新パイプラインが upstream `runmacro_mini` と整合していることを確認する手順は [`verify.md`](verify.md) に。ざっくり:

1. `runmacro_mini` を `-norm0 -clean0` で走らせて基準 PNG を生成
2. `verify_plot.cpp` を同じ `.root` で走らせて比較用 PNG を生成
3. `scripts/compare_png.py` でピクセル差分

---

## テスト

```bash
# C++ (doctest via CMake)
cd build && ctest --output-on-failure

# Python (pytest)
.venv/bin/python -m pytest python/tests -q
```

CI 環境ならこの 2 コマンドだけで全テストが通ることを確認。

---

## パラメータの意味と調整

### `run_mini.cpp` 内の `constexpr`

```cpp
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;   // ADC above cleaned baseline
constexpr double kMaxChargeRatio = 2.0;  // 3-plane charge agreement factor
```

- `kHitThreshold`: 下げるとヒット数増 (ノイズ拾う)、上げると弱トラックを落とす。30 ADC は baseline 除去後で十分高い。
- `kMaxChargeRatio`: 2.0 は「3 面の電荷が互いに 2 倍以内」。上げると多くの ghost が通る、下げると実 hit も落ちる。

### viewer3d のボクセル閾値

`voxel=1 mm` で走査してみると:

| min_count | 残る割合 (event 0 raw=35k) |
| --- | --- |
| 5 | 97% |
| 10 | 83% |
| 20 | 38% |
| 50 | <1% |

20 あたりから真のトラックの弱い部分も削れ始める。デフォルト off (`voxel=0`) は**最初は素で見てから絞る**方針。

### 直線フィット

- `n_lines`: 最初は 2-3 で試し、足りなければ上げる
- `fit thresh`: デフォルト 3 mm。pad pitch (~1 mm) と drift bin (~0.3 mm) より少し広く

---

## トラブルシューティング

### ROOT マクロが落ちる: `Library not loaded: @rpath/libMyLib.so`

dict を再ビルドすれば install_name が CMake 側で焼き直されるはず:

```bash
cd tpcanalysis-main/dict && rm -rf build && cmake -B build -S . && cmake --build build
```

### ACLiC (`+`) が `GET::GDataFrame::Class()` not found で落ちる

`-e 'gSystem->AddLinkedLibs("/abs/path/to/libMyLib.so");'` を忘れている (README 参照)。もしくは dict が未ビルド。

### `compare_png.py` が 10-15% の差分を出す

ROOT のバージョン差によるアンチエイリアス / フォント / カラーパレット違い。**同じ ROOT で `runmacro_mini` を走らせ直せば 0 diff になる**。詳細は [`verify.md`](verify.md)。

### Plotly が重い / 固まる

`max pts` を下げる (5000 以下) か、`voxel/min_count` で点数を削る。**1 event あたり 5 万点を超えるとブラウザ側で遅延する**。

### pytest で `ModuleNotFoundError: line_fit`

`python/tests/` から見た import path の問題。`python/tests/test_line_fit.py` 冒頭で `sys.path.insert(0, ...)` を使っているので、**プロジェクトルートから** `.venv/bin/python -m pytest python/tests` として実行すること。

### C++ ビルドで `cmake_minimum_required` warning

doctest (FetchContent で取る) が古い min_required を要求している。`CMAKE_POLICY_VERSION_MINIMUM=3.5` を CMake 側で設定済みなので warning のみで実害はない。

### Ubuntu 18.04 で ROOT が動かない

18.04 の glibc 2.27 は新しい ROOT の binary と互換性なし。Docker/Singularity で 22.04 環境を持ち込むか、ROOT 6.24 以下を使うか、OS を upgrade するかの 3 択。

---

## 既知の制限・注意

- **インタプリタと ACLiC で hit 数が違う** (946k vs 1.39M): cling FP 演算と -O2 ネイティブ FP の差。どちらを基準とするかは [`verify.md`](verify.md) を参照。production は ACLiC 側を canonical として扱う。
- **3D 点群は cartesian product 由来のゴーストを含む**: ボクセル閾値で減らすのが基本。精緻な ghost rejection は未実装。
- **`hitsToPoints` は charge ratio filter 以外の品質判断なし**: 真のトラックと大きくずれた combination も formally 許される。可視化では問題なくても、下流でフィットするときは注意。
- **Z 軸の符号は trigger_delay 基準**: `z_mm = (time_bin - 256) * drift_velocity * bin_width`。正負は drift 方向の定義次第なので、物理解析に使う前にビーム方向と突き合わせて確認。
- **ELITPC は未対応**: upstream コードは残っているが本番パイプラインは miniTPC 専用。
- **`run_mini.cpp` の `constexpr` を変えたらテストは通るが数字は変わる**: 設定変更は `git log` で残すこと。
