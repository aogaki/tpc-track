# M5: 引数 1 個のマクロに集約

**status**: done

## ゴール

ユーザが `run("CoBo_2026-04-21_0000.root")` の 1 呼び出しで、全イベントを 3D 点群ファイル (CSV or ROOT TTree) に書けるようにする。

## 受け入れ条件

- 新規 `tpcanalysis-main/run_mini.cpp` を作り (既存 `runmacro_mini.cpp` はそのまま) `run_mini(TString file)` の 1 関数だけ公開。
- 既定値は `norm=true, clean=true` で内部固定。
- 出力は 1 ファイル: `<入力名>_points.csv`
  - カラム: `event_id, x_mm, y_mm, z_mm, charge`
- 既存の PNG 出力経路は**足さない** (必要なら別 macro に切り出す)。KISS。
- M3 `hitsToPoints` の「3 plane ちょうど 1 ヒットずつ」条件は実データでは機能しないので、**cartesian product に緩和** してマルチヒット対応 (ゴースト除去は後回し)。

## 実装方針

- M1 〜 M3 を `loadData → convertUVW_mini → cleanUVW → extractHits → uvwToXyz` の順に繋げる。
- 出力は **CSV をまず作る**。Plotly 側が pandas で読める。ROOT TTree 版は Python から読む障壁が下がれば検討。
- `hitsToPoints` の改善: 各 time_bin で U/V/W 全組合せを生成し、各 (u, v) から (x, y) を解き、z と charge (3 plane 平均) をセットして返す。テストも更新。
- インタプリタ (cling) で走らせる (ACLiC はここでも落ちる)。

## やらないこと

- オプション引数 (norm/clean を戻す)
- 複数ファイル一括処理
- 進捗バー
- 更なるゴースト除去 (基本の charge-ratio フィルタは入れた; 追加チューニングは M6 以降)
- ROOT TTree 出力 (必要なら追加)

## M5 実行結果 (2026-04-22)

- 入力: `raw_run_files/CoBo_2026-04-06_0001.root` (142 events)
- 出力 CSV: **461 MB**, **13.6 M 点群**
- フィルタなし → 53.6 M 点 / 1.8 GB だったので `max_charge_ratio=2.0` で 4× 削減
- 最悪 event: 97 万点, 最小 event: 1 万点。**Plotly に渡すには更なる削減が必要** (M6 で)。
- `tpctrack_core` のユニットテストは全 3 target (13 ケース) green を維持。
