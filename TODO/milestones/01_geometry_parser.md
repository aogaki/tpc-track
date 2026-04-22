# M1: geometry パーサ

**status**: todo

## ゴール

`utils/new_geometry_mini_eTPC.dat` を読み、以下を取得できるようにする。

- ヘッダ定数: `ANGLES (U,V,W)`, `DIAMOND SIZE`, `REFERENCE POINT (x0, y0)`, `DRIFT VELOCITY`, `SAMPLING RATE`, `TRIGGER DELAY`, `DRIFT CAGE ACCEPTANCE`
- strip マッピング: `(plane, strip_nr) → (u_mm, strip 方向ベクトル, length_in_pads)`

## 受け入れ条件 (テストで縛る)

- `parseGeometryHeader("utils/new_geometry_mini_eTPC.dat")` がヘッダ値を struct で返し、期待値と一致する。
- `stripPosition(U, 1)` などの関数が、geometry ファイル先頭の数 strip について事前に手計算した `(u_mm)` と一致する（誤差 < 1e-6）。
- 未登録の `(plane, strip)` では例外 (もしくは明示エラー) を返す。

## 実装方針 (KISS)

- 新規ヘッダ `include/geometry.hpp` + `src/geometry.cpp` を 1 本追加。
- 既存の `convertUVW_mini::openSpecFile` が `filler` で読み捨てている `pad_pitch_offset, strip_pitch_offset, strip_length_in_pads` 列をここで保持する。
- ROOT 依存を増やさない。標準 C++ のみ。
- 言語: C++。Python 側でも使うなら CSV に吐き直す小物を M5 で足す。

## やらないこと

- ELITPC 版 (`geometry_ELITPC.dat`) 対応
- `50MHz` 版の切替
- geometry struct をシリアライズして Python に渡す仕組み（M5 で検討）
