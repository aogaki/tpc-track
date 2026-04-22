# M2: ヒット抽出

**status**: todo

## ゴール

`cleanUVW` 後の `std::vector<dataUVW>` から、各 `(plane, strip, time_bin)` に対して「ベースラインを超えた bin = ヒット」を抽出する関数を作る。出力は `{plane, strip, time_bin, charge}` の配列。

## 受け入れ条件 (テストで縛る)

- 入力にゼロ信号のみを与えたらヒット 0 件。
- 入力の 1 strip だけに閾値超えのピークを置いたら、ピーク幅に対応する件数のヒットが返る。
- 閾値より低い bin は拾われない。
- 両端 (cleanUVW で 0 埋めされる先頭 10 / 末尾 12 bin) は出てこない。

## 実装方針 (KISS)

- 既存の `cleanUVW::returnDataUVW()` の出力を入力に取る純粋関数 `std::vector<Hit> extractHits(const std::vector<dataUVW>&, double threshold)`。
- `threshold` のデフォルトは baseline + 数 ADC くらいで暫定。ここも後でチューニング対象。
- `Hit` は POD 構造体で OK。`{int plane; int strip; int time_bin; double charge;}`

## やらないこと

- ピークフィット (2 次多項式ピーク中心推定など)
- 隣接 bin のグルーピングによる「クラスタ化」
- 閾値の自動推定
