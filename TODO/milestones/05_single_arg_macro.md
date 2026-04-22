# M5: 引数 1 個のマクロに集約

**status**: todo

## ゴール

ユーザが `run("CoBo_2026-04-21_0000.root")` の 1 呼び出しで、全イベントを 3D 点群ファイル (CSV or ROOT TTree) に書けるようにする。

## 受け入れ条件

- `runmacro_mini.cpp` の引数パース (L537 以降) が消えて、`run(TString file)` 1 本になる。
- 既定値は `norm=true, clean=true` で内部固定。
- 出力は 1 ファイル: `<入力名>_points.csv` あるいは `<入力名>_points.root` (TTree)。
  - カラム: `event_id, x_mm, y_mm, z_mm, charge`
- 既存の PNG 出力経路は **残さない** (必要なら別 macro に切り出す)。KISS。

## 実装方針

- M1 〜 M3 を `loadData → convertUVW_mini → cleanUVW → extractHits → uvwToXyz` の順に繋げるだけ。
- 出力は **CSV をまず作る**。Plotly 側が pandas で読めるため。TTree 版は Python から読む障壁が下がれば検討。
- ROOT の `TTree::Fill` を使うなら `SetBasketSize` などはデフォルトで十分。

## やらないこと

- オプション引数 (norm/clean を戻す)
- 複数ファイル一括処理
- 進捗バー
