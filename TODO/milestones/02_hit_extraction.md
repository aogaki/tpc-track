# M2: ヒット抽出

**status**: done

## ゴール

ストリップ毎の時系列波形から、各 `(plane, strip, time_bin)` に対して「閾値を超えた bin = ヒット」を抽出する純粋関数を作る。出力は `{plane, strip, time_bin, charge}` の配列。

## 入力型について (KISS)

`tpcanalysis-main/include/dataUVW.hpp` を直接 include すると vendored 依存になるので、**我々のプロジェクトでは中間型 `StripSignal { int plane; int strip; std::vector<double> signal; }` を使う**。M5 の統合時に `dataUVW → StripSignal` の薄いアダプタを書く。

## 受け入れ条件 (テストで縛る)

- 入力にゼロ信号のみ (全 bin=0) を与えたらヒット 0 件。
- 単一ストリップに閾値超えのパルスを置く → ピーク幅に相当する件数のヒット、charge は signal 値と一致。
- 閾値 `= 入力の信号値` ちょうどの bin は拾わない (厳密大小比較)。
- 閾値未満の bin は拾わない。
- 複数ストリップを混ぜて渡す → 各ストリップの閾値超え bin がすべて返る。
- 両端 (cleanUVW で 0 埋めされる領域) は、入力がゼロなら結果的にヒット 0 件になることだけ確認 (M2 自身は 0 埋めを知らない)。

## 実装方針 (KISS)

- `include/tpctrack/hit.hpp`: `Hit` 構造体。
- `include/tpctrack/hit_extraction.hpp`: `std::vector<Hit> extractHits(const std::vector<StripSignal>&, double threshold)`。
- `src/hit_extraction.cpp`: 二重ループ + `> threshold` 判定のみ。
- `tests/test_hit_extraction.cpp`: doctest でテスト。

## やらないこと

- ピークフィット (2 次多項式ピーク中心推定など)
- 隣接 bin のグルーピングによる「クラスタ化」
- 閾値の自動推定
- `dataUVW` ↔ `StripSignal` のアダプタ (M5 で実装)
