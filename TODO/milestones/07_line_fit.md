# M7: 3D 直線フィットの重ね描き

**status**: done

## ゴール

M6 の 3D ビューに、`processXYZLines.fitNLinesv2` (BFGS 版) で検出した直線を重ねて表示。

## 受け入れ条件

- 1 イベントの点群に対し N 本 (初期値 N=1) の直線がフィットされ、線分として Plotly に描かれる。
- 直線の端点は点群の lambda の min/max で決める (既存実装と同じ)。
- インライア / アウトライアを色分けして表示 (オプション)。

## 実装方針 (KISS)

- `processXYZLines.py` は既に動く。viewer3d に `overlay_lines(fig, models)` を追加するだけ。
- `distance_threshold` は暫定 3.5 mm (既存デフォルト)。
- 複数トラックが想定されるイベントで N を増やす実験は別 issue。

## やらないこと

- 独自 3D フィッタの開発
- ピーク位置の再推定

## M7 実行結果 (2026-04-22)

- `python/line_fit.py` に `fit_lines(points, n_lines, distance_threshold_mm, min_inliers)` を実装。PCA で初期推定 → scipy BFGS で垂直距離最小化 → inlier 除去 → 反復。
- `python/tests/test_line_fit.py` に 5 ケース (単一クリーン線 / ノイズ付 / vertex 共有の 2 本 / 不十分点で早期終了 / endpoints 形状)。全 green。
- `viewer3d.plot_event` に `n_lines, fit_threshold_mm, fit_min_inliers` パラメータを追加。`overlay_lines()` が Scatter3d の line mode で fit 結果を重ね描き。
- ノートブックにスライダ `n lines` (0-6) と `fit thresh` (0.5-10 mm) を追加。n_lines=0 でフィット off (default)。
- 実データ検証 (event 0, voxel=1mm, min_count=10, n_lines=3, thresh=3mm): 3 本全てフィット、最大 inlier 9691 の主軸ライン + 2 本の枝ラインが重ね描きされることを確認。
