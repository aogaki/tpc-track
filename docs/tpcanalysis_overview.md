# upstream コード解析ドキュメント (歴史的)

**注意**: 本書は元の `tpcanalysis-main/` ディレクトリが project root に vendored されていた時期に書かれたもの。その後、必要なファイルだけを `include/upstream/` と `src/upstream/` に移動し、`tpcanalysis-main/` は削除した。**パスの記述は古いが、パイプラインの設計思想・物理パラメータ・アルゴリズムの解説は現行コードにもそのまま当てはまる**。新レイアウトでの読み替え:

| 旧パス | 新パス |
| --- | --- |
| `tpcanalysis-main/src/*.cpp` | `src/upstream/*.cpp` |
| `tpcanalysis-main/include/*.hpp` | `include/upstream/*.hpp` |
| `tpcanalysis-main/dict/include/*.h` | `include/upstream/*.h` |
| `tpcanalysis-main/dict/src/*.cpp` | `src/upstream/*.cpp` |
| `tpcanalysis-main/utils/*.dat` | `utils/*.dat` |
| `tpcanalysis-main/runmacro_mini.cpp` | 削除 (なくなった) |
| `tpcanalysis-main/run_mini.cpp` (マクロ) | `tools/run_mini_main.cpp` (standalone 実行ファイル) |
| `tpcanalysis-main/dict/CMakeLists.txt` | トップの `CMakeLists.txt` に統合 (ROOT_GENERATE_DICTIONARY) |

---

## 0. プロジェクトの位置付け

- **親プロジェクトの目的**: TPC (Time Projection Chamber) で取得した UVW 3 面 × 時間軸 のデータを 3 次元 (x, y, z) に再構成し、可視化する。
- **`tpcanalysis-main` の役割**: Pawel Majewski らが公開している上流解析コード。`.graw` → `.root`（GET electronics 経由）→ UVW 形式の 2D 画像 PNG、までを担う「前処理＋2D 可視化」段階。
- **対象検出器**: 2 種類サポートしている。
  - **miniTPC**: 単一 AsAd、U=72, V=92, W=92 ストリップ。U/V/W 角度 `(90°, -30°, 30°)`。
  - **ELITPC**: 4 枚の AsAd ボード（同じ event を 4 ファイルに跨って記録）、U=132, V=225, W=226 ストリップ。U/V/W 角度 `(0°, -120°, -60°)`。

---

## 1. ディレクトリ構成

```
tpcanalysis-main/
├── CMakeLists.txt          # executable 用 (現状 main.cpp が空なのでほぼ不使用)
├── install.sh              # build/ で cmake && make && make install するだけ
├── main.cpp                # スケルトンのみ。ROOT macro 経路が実運用
├── runmacro_mini.cpp       # miniTPC 用 ROOT マクロ
├── runmacro_elitpc.cpp     # ELITPC 用 ROOT マクロ
├── howto.txt               # graw→root への変換コマンドとマクロの呼び方
├── README.md               # 引数説明 + 実行例
├── include/                # ヘッダ
│   ├── loadData.hpp
│   ├── convertUVW_mini.hpp / convertUVW_elitpc.hpp
│   ├── cleanUVW.hpp
│   ├── miscFunc_elitpc.hpp
│   ├── rawData.hpp / dataUVW.hpp / dataFPN.hpp / generalDataStorage.hpp
│   └── ErrorCodesMap.hpp
├── src/                    # 対応する cpp
├── dict/                   # CEA の CoBoFrameViewer 由来 ROOT ディクショナリ
│   ├── include/ (GDataFrame.h, GDataChannel.h, GDataSample.h, GFrameHeader.h)
│   ├── src/     (上記 .cpp)
│   └── LinkDefGET.h
├── utils/                  # ジオメトリと正規化
│   ├── new_geometry_mini_eTPC.dat          # miniTPC 用
│   ├── geometry_ELITPC.dat                 # ELITPC 用（既定）
│   ├── geometry_ELITPC_80mbar_50MHz.dat    # 50 MHz 版
│   ├── ch_norm_ratios_mini.csv             # plane,strip,ratio
│   └── ch_norm_ratios_elitpc.csv
├── scripts/
│   ├── createNormCSV_mini.cpp      # ノイズエントリから正規化 CSV を生成
│   ├── createNormCSV_elitpc.cpp
│   └── entries_*.txt               # 上記で使うエントリ番号
├── python_comp/
│   ├── src/openCSV.py, openImage.py
│   ├── src/processUVWLines.py      # RANSAC / Hough の 2D 直線フィット
│   ├── src/processXYZLines.py      # RANSAC / BFGS の 3D 直線フィット
│   ├── llmClassification.ipynb     # LLaVA (localhost:1234) で画像分類
│   ├── printFit.ipynb              # skeletonize + RANSAC
│   ├── radonTransformImage.ipynb   # Radon 変換 + KMeans で主軸検出
│   └── obsolete/
├── junk/                    # 旧 XYZ 変換、Hough notebook、GUI など旧資産
└── Present.root, SepTest.root, second.root, third.root  # 動作検証用 .root
```

並列に `raw_run_files/` があり、`CoBo_2026-04-06_0001.root` 〜 `CoBo_2026-04-21_0000.root` の実データが置かれている。

---

## 2. 実行フローとエントリポイント

### 2.1 データ取得から画像化まで

1. **生データ取得**: CoBo + AsAd + AGET → `.graw` ファイル。
2. **`.graw` → `.root`**: `cobo-frame-graw2root`（`howto.txt` 参照。singularity コンテナ内で実行）。結果は `tree` という TTree に `GDataFrame` ブランチとして格納される。
3. **ROOT マクロ実行**:
   ```
   $ root
   [] .L runmacro_mini.cpp
   [] runmacro_mini({"-convert","-norm0","-clean0","-zip0",
                     "../Desktop/TPC_Share/CoBo_2026-04-21_0000.root"})
   ```
   - 第 1 引数: `-view` (対話的 TH2D 表示) または `-convert` (PNG 連続出力)。
   - 第 2〜4 引数: `-norm0/1`, `-clean0/1`, `-zip0/1` で正規化・クリーニング・zip の有無を切り替え。
   - 残りの可変引数: `.root` ファイルのパス。ELITPC では 4 つの AsAd ファイルを束ねて渡す。
4. **出力**:
   - `-view`: TCanvas に U/V/W の TH2D（と charge TH1D）を描画、`d/a/i/e` キーで entry 操作。
   - `-convert`: `images_<default|mild|aggressive|none>/<root ファイル名>/<entry>_{u,v,w}.png` を生成。`-zip1` なら zip して中間フォルダを削除。

### 2.2 呼び出しグラフ（ELITPC の convert モード）

```
runmacro_elitpc
 └─ mass_create_clean_images_elitpc
     └─ drawUVWimages_elitpc
         ├─ miscFunc_elitpc::getAllEntries      (ファイル毎)
         │   └─ loadData::openFile/readData/decodeData
         │   └─ convertUVW_elitpc::openSpecFile + makeConversion
         │   └─ (先頭 10 bin と末尾 12 bin を 0 埋め, <10 は 10 にクリップ)
         ├─ miscFunc_elitpc::groupEntriesByEvent  (event_id で 4 ファイル分を集約)
         ├─ miscFunc_elitpc::mergeSplitStrips     (同一 plane,strip の信号を足し合わせ)
         └─ (opt_clean の場合) cleanUVW::substractBl<U|V|W>
         └─ TH2D 作成 → COL 描画 → TCanvas::Print で PNG
```

miniTPC は AsAd が 1 枚なので `loadData` と `convertUVW_mini` を直接ループし、`drawUVWimage_mini` が entry 毎に画像を出す。

---

## 3. C++ 側モジュール詳解

### 3.1 `loadData` (ROOT → rawData)

- **役割**: `TFile` を `READ` で開き `TTree tree` から `GDataFrame` ブランチを取得、1 entry ずつ `rawData` ベクトルへ decode する。
- **主な API**
  - `openFile()`: 0=成功, -1=開けない
  - `readData()`: TTree を読み branch addr を `m_data_branch`(`GDataFrame*`) にバインド。
  - `decodeData(entryNr, remove_fpn)`: `GetEntry(entryNr)` → `GetChannels()` を `TIter` で回し、各 `GDataChannel` の 512 個の `GDataSample::fValue` を `rawData.signal_val` に push。`chip_nr = fAgetIdx`, `ch_nr = fChanIdx`, `asad_id = fHeader.fAsadIdx`, `event_id = fHeader.fEventIdx`。
  - `returnRawData()`, `returnNEntries()`
- **FPN 除去 (`removeFPN`)**: ハードコードされた FPN チャンネル集合 `{11, 22, 45, 56}` を同じ `chip_nr` 内で平均し、その chip の全チャンネル信号から減算。負値や 10 未満は 10 に切り上げ。`loadData` 側で `remove_fpn=true` とすれば適用される。miniTPC では runmacro からは `true` で呼ばれている。

### 3.2 `convertUVW_mini` / `convertUVW_elitpc` (座標変換)

- **役割**: rawData の電子系座標 `(AGET chip, channel)` を検出器座標 `(plane, strip, strip_section)` に変換。
- **ジオメトリファイル**: `utils/new_geometry_mini_eTPC.dat` / `utils/geometry_ELITPC.dat`。ヘッダに
  - `ANGLES: <U> <V> <W>` [deg]
  - `DIAMOND SIZE` [mm]（pad 1 辺）
  - `REFERENCE POINT: x y` [mm]
  - `DRIFT VELOCITY: 0.724` [cm/μs]
  - `SAMPLING RATE: 25.0` [MHz]
  - `TRIGGER DELAY: 10.24` [μs]
  - `DRIFT CAGE ACCEPTANCE: -100 100`（mini）／`-98 98`（ELITPC）
  などの物理パラメータが入り、続く行で strip 毎の接続情報を記述する。  
  列: `plane / strip_section / strip_nr / COBO_id / ASAD_id / AGET_id / AGET_ch / pad_pitch_offset / strip_pitch_offset / length_in_pads`（コメント行は `#`）。
- **`m_channel_reorder_map`**: ジオメトリファイルは FPN 用の 11/22/45/56 を除いた 0〜63 の論理 channel で記述されているため、実機の 0〜67 にマップし直す辞書 (class 内に inline static で定義)。
- **処理**
  - `openSpecFile()`: 正規化 CSV (`buildNormalizationMap`) とジオメトリファイルを読み、`fPositionMap[{chip, ch}] = {GEM, strip}` (mini) ないしは `fPositionMap[{ASAD, chip, ch}] = {GEM, strip, section}` (elitpc) を構築。
  - `makeConversion(opt_norm, opt_verbose)`: rawData をループして `fPositionMap.at(...)` で該当 strip に引き当てる。map にない key は try/catch で無視（= FPN チャンネル）。最後に `opt_norm==true` なら `normalizeChannels()`。
  - `normalizeChannels()`: `m_ch_ratio_map[{plane, strip}]` を分母に `signal_val` を `std::transform` で除算。感度差を揃える前処理。比率 0 の strip は既存データでは実質無効（除算前に `0,0,0` など縁）。
- **CSV 書き出し** (`convertUVW_mini::convertToCSV`): 行毎に `plane,strip,entry,signal0,signal1,...` を書く utility。Python 側の解析で使える。ELITPC 版にはない。

### 3.3 `cleanUVW` (ベースライン減算 + 平滑)

- **役割**: UVW 波形からベースラインを推定して引き、オプションで Savitzky-Golay 平滑を掛ける。template 引数で U/V/W を切替。
- **構造体** `miniPlaneInfoU/V/W`: `plane_nr`, `plane_size`, `plane_hist_name` を静的に持つ型タグ。miniTPC 基準の `plane_size` (72/92/92) が ELITPC でもそのまま使われている点に注意。
- **Savitzky-Golay**: window `8`, poly order `2`。係数 `m_coefficients` はコンストラクタで一度計算される。
- **`substractBl<pI>(smooth_opt=false)` の手順**
  1. 対象 plane の各 strip 波形について、`smooth_opt` ならば `savitzkyGolayFilter` を適用。
  2. 両端のアーティファクトを抑えるため先頭 10 bin と末尾 12 bin をゼロクリア。
  3. `calculateChargeHist<pI>()` で「その plane の全 strip を bin 毎に加算した 512 長ヒスト」`m_charge_val[plane]` を構築。
  4. `calculateBaseline<pI>()` で `m_charge_val` の 6 番目以降の非ゼロ最小値をベースラインとして採用（カスタム比較で 0 を除外）。
  5. `m_charge_val` から baseline を引いて負値 → 0。
  6. baseline を `plane_size` で割り、各 strip の `signal_val` から引く。下限は 10（空 bin を作らない）。
- **`getChargeHist<pI>(charge_hist)`**: 上で得られた電荷ヒストを `TH1D` にロードして返す。
- **`setUVWData()`**: 別途ベースライン後などの UVW データを再投入する抜け穴。

### 3.4 `miscFunc_elitpc` (ELITPC 特有の束ね処理)

- **`getAllEntries(fileName, verbose, opt_norm)`**
  - 与えられた 1 ファイルの全 entry を loadData+convertUVW_elitpc で UVW 化。
  - 最終段で「先頭 10 bin & 末尾 12 bin を 0 埋め」「全値の下限を 10」を適用。メモリ節約のため `root_raw_data` を swap で破棄。
- **`groupEntriesByEvent(entries_vec)`**
  - 4 つの AsAd に跨って記録された UVW データを `event_id` をキーにした `std::multimap` で束ね、同一イベントの全 strip を `generalDataStorage::uvw_data` に集約した新しいベクトルを返す。
- **`mergeSplitStrips(data_vec)`**
  - ELITPC では長い strip が物理的に 2 分割されて別 channel で読まれている。`(plane, strip_nr)` が同じものを探し、`signal_val` を bin 毎に**足し合わせて** 1 本に統合（平均にする選択肢もコメントアウトで残る）。`strip_section` は 0 に戻される。

### 3.5 `rawData` / `dataUVW` / `generalDataStorage`

```
rawData        : ch_nr, chip_nr, entry_nr, asad_id, event_id, signal_val[512]
dataUVW        : plane_val(0/1/2), strip_section, strip_nr, entry_nr,
                 event_id, baseline_val, signal_val
generalDataStorage : root_raw_data[], uvw_data[], event_id
```

- `enum class GEM { U, V, W, size }` を `dataUVW.hpp` で定義。plane_val と一致させる。
- `dataFPN` は FPN 専用の保管用だが現行コードでは未使用。
- `ErrorCodesMap`: `{0: 成功, -1: File not open, -2: Tree not found, -3: Invalid size, -4: Unassigned data}` をマッピング。

### 3.6 `dict/` (CEA CoBoFrameViewer 由来)

- `GFrameHeader` … `fEventIdx`, `fAsadIdx`, `fCoboIdx`, `fHitPatterns[4]`, `fMult[4]` などを保持。
- `GDataChannel` … `fAgetIdx`, `fChanIdx`, `fSamples (TRefArray)`。
- `GDataSample` … `fBuckIdx` (0〜511), `fValue` (12 bit ADC)。
- `GDataFrame` … 上記の `TClonesArray` を束ねる。
- LinkDef が用意されており `rootcling` でディクショナリ化可能（現状は `cpp` を直接 `#include` してマクロから使う）。

### 3.7 正規化 CSV 生成スクリプト

- `scripts/createNormCSV_mini.cpp`: 手動で選んだ「ノイズのみ」な 20 エントリを `entry_nrs` 配列で指定。各 plane 毎に全 strip の平均値 → 全 strip の平均 (`total_mean`) を算出し、`ratio = strip 平均 / total_mean` を計算して `utils/ch_norm_ratios_mini.csv` に書き出す。
- ELITPC 版も考え方は同じ。`scripts/entries_*.txt` にエントリの候補が列挙されている。

---

## 4. ランマクロの内部動作詳細

### 4.1 `runmacro_mini.cpp`

- `viewUVWdata_mini`: `TCanvas` を 4×2 に分割し、U/V/W の生 TH2D + charge TH1D と、`cleanUVW` 適用後の TH2D + charge TH1D を並べて比較表示。キー入力 `e/d/a/i` で終了/次/前/番号指定。
- `drawUVWimage_mini`: バッチモード (`gROOT->SetBatch(kTRUE)`) で entry 1 つを 3 枚の PNG (`<entry>_u.png` etc) に書き出す。y 軸 bins は (72, 92, 92)。描画時の最大値は `SetMaximum(1000)`、最小値は `-1`。
- `mass_create_clean_images_mini`: 最大 `nr_entries`(デフォルト 10000) まで `drawUVWimage_mini` を連続呼び。zip オプション付きなら `zip -j -r -m` で圧縮して元フォルダを削除。
- 出力フォルダ名は `opt_clean` と `opt_norm` の組合せから `images_aggressive / images_mild / images_default / images_none` を選ぶ。

### 4.2 `runmacro_elitpc.cpp`

- 4 ファイル想定で `std::initializer_list<TString>` を受け取り、`selected_files` を `std::vector<TString>` として `viewUVWdata_elitpc` か `mass_create_clean_images_elitpc` に渡す。
- TH2D の y bins は miniTPC と異なり U=132, V=225, W=226。canvas 幅は 512px、高さは `3 × ストリップ数`。
- view モードでは `strip_section == 0` のものだけ描画するフィルタが入っている（section 1 を見るにはコメントアウト部の編集が必要）。

### 4.3 引数パース

両マクロ共通で、以下の順序でパース:
1. `-view` | `-convert`
2. `-norm0` | `-norm1`
3. `-clean0` | `-clean1`
4. `-zip0` | `-zip1`
5. 以降: 1 個以上の `.root` ファイルパス

---

## 5. Python 側モジュール

### 5.1 `python_comp/src/`

- **`openImage.py`**: `matplotlib.image.imread` で PNG を読み、`trimImage` で白背景をクロップする軽量ユーティリティ。
- **`openCSV.py`**: `pandas.read_csv` のラッパ。
- **`processUVWLines.py`**
  - `modelDataUVW` / `modelDataUVWHough` というデータクラスに slope/intercept/min-max を保存。
  - `fitNLines`: `skimage.measure.ransac` で plane (0/1/2) 毎に `n` 本の直線を反復抽出。インライア描画とモデル保存を同時に行う。
  - `fitNLinesHough`: `scipy.optimize.minimize(method="BFGS")` で Hesse の正規形 `(ρ, θ)` を最適化し、閾値以下の点を取り除きながら直線を積み上げる。縦線は `axvline` でハンドリング。
- **`processXYZLines.py`**
  - `fitNLines`: 3D 点群に対して `skimage` の RANSAC (`LineModelND`) を反復適用。
  - `fitNLinesv2`: `(x0, y0, z0, dx, dy, dz)` の 6 パラメータを BFGS で最小化するオリジナル 3D 直線フィット。`findLambdaForPoint` で x からパラメータ化。

### 5.2 `python_comp/*.ipynb`

- **`printFit.ipynb`**: PNG → skimage の `skeletonize` で 1 px 線化 → `cleanNoise` で小さい連結成分除去 → RANSAC で直線フィット → `perpendicular_distance` で外れ値再フィルタ、という基礎的なトラック候補抽出。
- **`radonTransformImage.ipynb`**: `skimage.transform.radon` でシノグラムを作り `scipy.signal.find_peaks` + `sklearn.cluster.KMeans` で卓越する角度を見つけ、`skimage.draw.line` で 2D 画像にオーバレイ。直線的トラックの「主軸」を抽出する。
- **`llmClassification.ipynb`**: `openImage` を使って PNG を読み、`localhost:1234` に立ち上げた LLaVA モデルに `requests.post` して分類結果を受け取る（対応するモデルが別途必要）。

### 5.3 `python_comp/obsolete/`, `junk/`

旧実装の XYZ 変換・Hough notebook・GUI・その他。再利用候補のアイデアとして `junk/convertXYZ.cpp`, `junk/filterEventsXY.cpp`, `junk/findInterPoint.ipynb`, `junk/fitUVWLine.ipynb`, `junk/fitXYZLine.ipynb` などが参考になる（API は現ヘッダと噛み合わないので移植必須）。

---

## 6. 物理座標と時間軸の対応

### 6.1 基本パラメータ

| 項目 | 値 (ELITPC default) | 出所 |
| --- | --- | --- |
| サンプル点数 / event | 512 | `GDataSample::fBuckIdx` の range |
| サンプリングレート | 25.0 MHz (⇒ 40 ns/bin) | geometry ファイル |
| 記録時間窓 | 512 × 40 ns = 20.48 μs | 計算 |
| ドリフト速度 | 0.724 cm/μs | geometry ファイル |
| ドリフト距離窓 | 20.48 μs × 0.724 cm/μs ≒ 14.8 cm | 計算 |
| トリガー遅延 | 10.24 μs | geometry ファイル |
| ドリフトケージ受容域 | ±98 mm (ELITPC) / ±100 mm (mini) | geometry ファイル |
| U/V/W 角度 | (0°, -120°, -60°) ELITPC / (90°, -30°, 30°) mini | geometry ファイル |
| pad 幅 | 1.0 mm (DIAMOND SIZE) | geometry ファイル |

> 註: 現行コードは**時間軸を bin 番号のままで使っている**。物理単位に落とす処理（ドリフト時間 → z [mm]）はまだ実装されていない。

### 6.2 時間 → z 変換

```
t_bin = k (k = 0..511)
t     = (t_bin - t0_bin) / 25 MHz        # [μs]。t0_bin は TRIGGER DELAY × 25 MHz = 256
z     = t × v_drift (= 0.724 cm/μs)      # [cm]
```

トリガー遅延 10.24 μs は `512 / 2 / 25 MHz` に合致しているので、ビーム軸を `z = 0` とするには `t0_bin = 256` を基準とするのが自然。

### 6.3 UVW ↔ XY の幾何

各 plane の strip 位置 `s_i`（strip_nr から `pad_pitch_offset`, `strip_pitch_offset`, 角度 `θ_i` を合わせて実座標化）に対し、XY 平面上の点 `(x, y)` は

```
u = x cos(θ_U) + y sin(θ_U) + c_U
v = x cos(θ_V) + y sin(θ_V) + c_V
w = x cos(θ_W) + y sin(θ_W) + c_W
```

の 3 式で表される。3 本中 2 本を選べば交点が決まるが、3 本の整合性（u + v + w = const の関係等）を使ってゴーストヒットを除外できる。`c_*` は `REFERENCE POINT` + 各 plane の `strip_pitch_offset` で決まる定数オフセット。

> **注意**: 現行コードはこの変換を実装していない。`utils/*.dat` の `strip_pitch_offset` / `pad_pitch_offset` を読み出す処理を追加する必要がある（今は `openSpecFile` で `filler` として読み捨てている）。

---

## 7. 3D トラック再構成への拡張方針

### 7.1 基本戦略

1. `.root` → UVW （既存）。
2. 各 plane で `(time_bin, strip)` → `(x_strip, time_bin)` の物理座標へ変換（`strip_pitch` + 角度 → `REFERENCE POINT` 基準の XY 上の線分）。
3. 3 plane の 2D hits を時間 bin 毎にマッチングして `(x, y)` を決定。
4. 時間 bin を z [mm] に変換。
5. 3D 点群 (x, y, z) を `processXYZLines.fitNLinesv2` などで直線フィット。

### 7.2 既存コードの再利用ポイント

- **前処理**: `loadData` + `convertUVW_elitpc` + `cleanUVW` はそのまま使える。`generalDataStorage.uvw_data` 1 件が 1 event 相当。
- **CSV 入出力**: `convertUVW_mini::convertToCSV` を雛形に UVW dump → Python 読み込みが可能。ELITPC 版にも拡張を推奨。
- **Python**: `processXYZLines` が既に 3D 直線フィットの API を持っている。入力は `entry[['x','y','z']]` の DataFrame。
- **ジオメトリ**: `utils/geometry_*.dat` の `ANGLES`, `REFERENCE POINT`, `DIAMOND SIZE`, `DRIFT VELOCITY`, `SAMPLING RATE`, `TRIGGER DELAY`、および strip 毎の `pad_pitch_offset`/`strip_pitch_offset` を ROOT 側 or Python 側でパースする utility が必要。今は `filler` として読み捨てている列。

### 7.3 実装時の留意点

- **ゴーストヒット**: plane 3 本のヒット同時性を時間 bin 窓 + 電荷比で絞らないと組み合わせ爆発する。`cleanUVW::calculateChargeHist` を参考に、各 bin の総電荷を使って u + v + w の整合性を確認できる。
- **FPN 再確認**: `loadData::removeFPN` は現状で `<10` を 10 にクリップするが、これは後段のベースライン処理と干渉しうる。3D 再構成ではベースライン減算後の「実信号があった bin」のみを拾うため、このクリップを無効化した経路も試したほうが良い。
- **ストリップ分割**: ELITPC の `mergeSplitStrips` は単純加算している（平均ではない）。drift が長い方向で 2 倍の電荷になる点を、今後 drift correction と合わせて再検討するのが望ましい。
- **時間原点**: `cleanUVW::substractBl` は先頭 10 / 末尾 12 bin を 0 詰めしている。物理トラックがこの縁に出る event では情報欠落になるため、3D 側では未クロップ版の UVW を別経路で持つ設計が良い。
- **ジオメトリファイルの 50 MHz 版**: `geometry_ELITPC_80mbar_50MHz.dat` は `SAMPLING RATE` を 50 MHz にしてある想定の別条件用。デフォルトでは `geometry_ELITPC.dat`(25 MHz) が読まれる点に注意。ハードコード (`m_specfilename`) なので、run に応じて切り替えるには convertUVW_elitpc にパス指定引数を足す必要がある。
- **単位系**: `convertUVW_*` 内で strip を整数番号として扱っているが、pad_pitch と DIAMOND SIZE は mm 単位。3D 化のためには `(plane, strip_nr)` → `(u_mm)` を計算する pure-function を新設し、`dataUVW` の派生クラスに `u_mm` を保持させると拡張しやすい。

---

## 8. 今後の作業のチェックリスト

- [ ] `geometry_*.dat` のフルスキーマパーサを ROOT or Python で実装し `strip_nr → u_mm` を得られるようにする。
- [ ] `dataUVW` を拡張して `time_ns` / `u_mm` を持たせ、UVW→XYZ 変換ヘルパを追加。
- [ ] UVW TH2D から `(plane, strip, time_bin, charge)` の hit 点群を抽出 → CSV / ROOT TTree 出力する経路を追加（既存の `convertToCSV` を ELITPC に展開）。
- [ ] Python 側で `processUVWLines` の fit 結果と `processXYZLines` を連携させ、同じ event_id を使って 3D 点群を作る。
- [ ] `mergeSplitStrips` を平均に切り替える/切り替えない両方で 3D 再構成精度を比較。
- [ ] `loadData::removeFPN` を無効化したデータに対するベースライン再評価。
- [ ] `cleanUVW::substractBl` で 0 埋めする縁領域の長さを config 化。
- [ ] `geometry_ELITPC_80mbar_50MHz.dat` のようなバリアントを runtime に選べるよう `convertUVW_elitpc::openSpecFile` に path 引数を追加。
- [ ] `.graw → .root` の自動化スクリプト（現状 `howto.txt` の手作業）。

以上を足掛かりに、`tpcanalysis-main` を 3D 再構成パイプラインへ発展させていける。
