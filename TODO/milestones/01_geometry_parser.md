# M1: geometry パーサ

**status**: done

## ゴール

`utils/new_geometry_mini_eTPC.dat` を読み、以下を取得できるようにする。

- ヘッダ定数: `ANGLES (U,V,W)`, `DIAMOND SIZE`, `REFERENCE POINT (x0, y0)`, `DRIFT VELOCITY`, `SAMPLING RATE`, `TRIGGER DELAY`, `DRIFT CAGE ACCEPTANCE`
- strip メタデータ: `(plane, strip_nr) → {strip_section, cobo, asad, aget, aget_ch, pad_pitch_offset, strip_pitch_offset, length_in_pads}`
- 各平面の strip 方向単位ベクトル (`ANGLES` から導出)

## スコープに関する注記

「`(plane, strip)` → `u_mm` (物理 mm 座標)」は当初 M1 に含めていたが、DIAMOND_SIZE / pad_pitch / strip_pitch の組み合わせ方 (hex/diamond 格子の幾何) を決定するには UVW→XY のラウンドトリップ検証が必要で、これは M3 で一緒にやる方が自然。**M1 は raw metadata の取得に絞る**。

## 受け入れ条件 (テストで縛る)

- `parseGeometry("utils/new_geometry_mini_eTPC.dat")` が `GeometryHeader` をファイルの値通りに返す (±1e-9)。
- `geo.stripInfo(Plane::U, 1)` が先頭行のメタデータを返し、期待値 (`strip_section=0, cobo=0, asad=0, aget=2, aget_ch=62, pad_pitch_offset=0, strip_pitch_offset=-71, length_in_pads=56`) と一致する。
- 未登録の `(plane, strip)` では `std::out_of_range` を投げる。
- `geo.stripAxisDirection(Plane::U)` が `(cos 90°, sin 90°) = (0, 1)` を返す (±1e-9)。

## 実装方針 (KISS)

- 新規 `include/tpctrack/geometry.hpp` + `src/geometry.cpp` を 1 本追加。
- 既存の `convertUVW_mini::openSpecFile` が `filler` で読み捨てている `pad_pitch_offset, strip_pitch_offset, strip_length_in_pads` 列をここで保持する。
- ROOT 依存を増やさない。標準 C++ のみ (C++17)。
- テストは doctest を FetchContent で取得。
- 言語: C++。Python 側でも使うなら CSV に吐き直す小物を M5 で足す。

## やらないこと

- ELITPC 版 (`geometry_ELITPC.dat`) 対応
- `50MHz` 版の切替
- `u_mm` 計算 (M3 で実装)
- geometry struct をシリアライズして Python に渡す仕組み（M5 で検討）
