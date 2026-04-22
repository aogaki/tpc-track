# M6: Plotly で 3D ぐりぐり

**status**: done

## ゴール

M5 が吐いた点群 CSV を読んで、1 イベントを Plotly の `scatter_3d` で回せるようにする。**最小構成: Jupyter Notebook 1 本 + event 番号スライダ**。

## 受け入れ条件

- ノートブックを開いて実行すると、指定したイベントの点群が 3D で回せる状態で表示される。
- 点の色は charge (log scale 推奨)、マーカーサイズは一定で良い。
- 軸は `x_mm, y_mm, z_mm` [mm] でラベル付き。
- event 番号を変えると再描画される (ipywidgets の Slider か、Dash が要るなら Dash)。

## 実装方針 (KISS)

- 新規 `python/viewer3d.py` に `load_points()` と `plot_event(df, event_id, max_points=20000)` を置く。
- ノートブック `python/viewer3d.ipynb` から呼ぶ (ipywidgets のスライダ付き)。
- pandas で CSV 読み、`df[df.event_id == N]` でフィルタ。
- `max_points` を超える event は `nlargest(max_points, "charge")` で top-charge をサンプリング。

## M6 実行結果 (2026-04-22)

- 依存パッケージはプロジェクト venv (`.venv/`) で管理。`plotly 6.7 / pandas 3.0 / ipywidgets 8.1 / scikit-image / scipy / scikit-learn`。
- `CoBo_2026-04-06_0001_points.csv` (13.6 M 点, 142 event) を pandas で読み込み、event 0 を 20k 点 (charge top) に downsample → `plotly.graph_objects.Scatter3d` で書き出し可能であることを確認。
- 既定 color scale は `log10(charge)` with Viridis, marker size 2 px, `aspectmode="data"`。

## やらないこと

- Streamlit / Dash のフル UI (M8)
- 複数イベント重ね描き
- トラックフィット結果の重ね描き (M7)
