# tpc-track プロジェクト計画

## ゴール

**1 つの `.root` ファイルを入力に、1 イベントを 3 次元でインタラクティブに回せる可視化 (Plotly) を作る。** 将来的には 3D トラックフィットまで繋げる。

## 前提と制約

- 対象検出器は **miniTPC** 想定。`runmacro_mini` 系をベースにする。
- AsAd 1 枚 / 1 ファイル完結運用。ELITPC の 4 ファイル束ね (`miscFunc_elitpc::groupEntriesByEvent`) は今回使わない。
- サンプリング 25 MHz、512 time bin、ドリフト速度 0.724 cm/μs、トリガー遅延 10.24 μs (= 256 bin)。
- ストリップ数: U=72, V=92, W=92。角度 (U,V,W) = (90°, -30°, 30°)。
- 1 イベントの 3D ヒット点数は数百〜数千と想定 → **Plotly `scatter_3d` で十分**。

## 原則

- **KISS**: 機能を足す前に本当に必要か問う。既定値で動くものを優先、オプションは後付け。
- **TDD**: ユニットは「テストを書く → 赤 → 実装 → 緑 → リファクタ」の順に実装する。テストが書けない抽象化は足さない。

## 言語分担

- **C++ / ROOT マクロ**: `.root` 読み取りと UVW 変換。GET の `GDataFrame` ディクショナリを使うためここは C++ に残す。
  - `loadData` + `convertUVW_mini` + `cleanUVW` を流用。
  - 出力: イベント毎の `(event_id, plane, strip, time_bin, charge)` ヒット点群、あるいは `(x_mm, y_mm, z_mm, charge)` の点群を CSV か ROOT TTree に書く。
- **Python**: 可視化 (Plotly) + 将来のフィット。点群 I/O とジオメトリ計算はこちら側で持ってもよい。
  - 既存の `python_comp/src/processXYZLines.py` の 3D フィットを将来流用。

## データフロー

```
.root (GDataFrame)
  ↓ [C++] loadData             (FPN 除去は既定 on)
rawData (chip, ch, signal[512])
  ↓ [C++] convertUVW_mini      (norm=true 既定)
dataUVW (plane, strip, signal[512])
  ↓ [C++] cleanUVW             (clean=true 既定、baseline 減算 + SG 平滑)
dataUVW (cleaned)
  ↓ [C++] hit extraction       (閾値超えの time bin を hit 化)
ヒット点群 (plane, strip, time_bin, charge)
  ↓ [C++ or Python] UVW→XYZ    (geometry 1 個から厳密に計算)
点群 (x_mm, y_mm, z_mm, charge)
  ↓ [Python] Plotly scatter_3d
3D ぐりぐり
```

## 引数設計

- マクロは **ファイル名 1 つだけを受け取る**。`norm=true, clean=true, zip=false` は内部固定。
- それ以外のオプションが必要になるのは「動かしてから」。先回りしない。

## マイルストーン

下記の順で進める。各マイルストーンに対応するタスクファイルは `milestones/` に置く。

1. **[M1](milestones/01_geometry_parser.md)** ✅ done — geometry パーサ (header + strip metadata + 方向ベクトル)。6 test case。
2. **[M2](milestones/02_hit_extraction.md)** ✅ done — StripSignal → Hit の閾値抽出。6 test case。
3. **[M3](milestones/03_uvw_to_xyz.md)** ✅ done — UVW → (x, y, z, charge)。hitsToPoints は cartesian product + `max_charge_ratio` フィルタ。9 test case。
4. **[M4](milestones/04_reference_check.md)** ✅ done — `verify_plot.cpp` が `runmacro_mini -norm0 -clean0` とピクセル一致で UVW PNG を出し、XY PNG は angle / reference point が正しいことを目視確認済み。
5. **[M5](milestones/05_single_arg_macro.md)** ✅ done — `run_mini("path.root")` で全 event を点群 CSV に出力。ACLiC 経由でビルド速度 40% 改善。
6. **[M6](milestones/06_plotly_viewer.md)** ✅ done — Plotly `scatter_3d` + event ナビゲーション + voxel-coincidence フィルタスライダ。
7. **[M7](milestones/07_line_fit.md)** ✅ done — PCA + BFGS の iterative 3D line fit + Plotly 重ね描き。5 pytest case。
8. **[M8](milestones/08_polish.md)** ⏸ deferred — 具体的要望が出たタイミングで再開。

## やらないこと (今は)

- ELITPC の 4 AsAd 束ね
- `geometry_*.dat` のバリアント自動切替
- graw→root 変換の自動化
- LLM 分類 (`llmClassification.ipynb` 系) のパイプライン化
- 3D Radon や skeletonize を 3D に拡張すること

必要になったら `milestones/` に追加する。
