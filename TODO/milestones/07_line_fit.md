# M7: 3D 直線フィットの重ね描き

**status**: todo

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
