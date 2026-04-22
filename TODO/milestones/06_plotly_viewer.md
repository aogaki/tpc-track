# M6: Plotly で 3D ぐりぐり

**status**: todo

## ゴール

M5 が吐いた点群 CSV を読んで、1 イベントを Plotly の `scatter_3d` で回せるようにする。**最小構成: Jupyter Notebook 1 本 + event 番号スライダ**。

## 受け入れ条件

- ノートブックを開いて実行すると、指定したイベントの点群が 3D で回せる状態で表示される。
- 点の色は charge (log scale 推奨)、マーカーサイズは一定で良い。
- 軸は `x_mm, y_mm, z_mm` [mm] でラベル付き。
- event 番号を変えると再描画される (ipywidgets の Slider か、Dash が要るなら Dash)。

## 実装方針 (KISS)

- 新規 `python_comp/src/viewer3d.py` に `plot_event(df, event_id)` のような純粋関数を置く。
- ノートブック `python_comp/viewer3d.ipynb` から呼ぶ。
- pandas で CSV を読み、`df.query("event_id == N")` でフィルタ。

## やらないこと

- Streamlit / Dash のフル UI (M8)
- 複数イベント重ね描き
- トラックフィット結果の重ね描き (M7)
