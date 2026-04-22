# M3: UVW → XYZ 変換

**status**: todo

## ゴール

M2 で得たヒット (plane, strip, time_bin, charge) を 3 plane 横断で突き合わせ、`(x_mm, y_mm, z_mm, charge)` の 3D 点群を作る。

## 受け入れ条件 (テストで縛る)

- 仮想的に 1 本の直線トラックを投入 (3 plane それぞれに期待される strip/time_bin を手計算) した時、変換後の点が同じ直線に載る (residual < strip_pitch)。
- `time_bin → z_mm` が `(bin - trigger_delay_bins) × v_drift / sampling_rate` で与えられる。
- 時間 bin が同じ U/V/W 3 点を 1 つの (x, y) に解決する。3 点整合しないもの (ゴースト) は捨てる or 別ラベルで残す。

## 実装方針 (KISS)

- M1 の geometry 結果を使って
  - `u_mm = cos(θ_U)·x + sin(θ_U)·y + c_U` を 3 plane 分立てて 3 元連立 (2 個で十分だが 3 plane 整合チェックに使う)
  - 線形最小二乗で `(x, y)` を求める
- `z_mm = (time_bin - 256) × (1 / 25e6) × 0.724 × 10` （bin → μs → cm → mm）
- 時間 bin の一致判定は ±1 bin の窓から始める。後で広げるかもしれない。

## ゴーストヒット対策 (最低限)

- 3 plane の charge がオーダーで揃っているかのチェック (log ratio 窓) のみ。精緻なものは後回し。

## やらないこと

- 3D クラスタリング
- drift diffusion 補正
- 電場歪み補正
