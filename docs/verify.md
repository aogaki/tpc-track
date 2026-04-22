# M4 検証手順 — リファレンス PNG との突き合わせ

新パイプラインが出す UVW PNG を既存 `runmacro_mini` の `-norm0 -clean0` 出力とピクセル比較する手順、および時間積分 XY PNG の視覚確認手順。

## 必要なもの

- ROOT (6.x。プロジェクトでは 6.36 で確認)
- Python 3 + Pillow (`pip install pillow`)
- 1 つの `.root` 入力 (例: [`raw_run_files/CoBo_2026-04-21_0000.root`](../raw_run_files/))

以下では入力を `<root>` と表記する。

## 事前準備: GET ディクショナリライブラリの build (1 回だけ)

`verify_plot.cpp` も `run_mini.cpp` も `tpcanalysis-main/dict/build/libMyLib.dylib` を `R__LOAD_LIBRARY` で読み込む。未ビルドなら最初に作る:

```bash
cd tpcanalysis-main/dict
cmake -B build -S . && cmake --build build
```

install_name は絶対パスで焼かれるので、DYLD_LIBRARY_PATH は不要。

## Step 1: リファレンス PNG (既存マクロ)

既存の `runmacro_mini` を `-convert -norm0 -clean0 -zip0` で走らせる。出力は入力 `.root` と同じディレクトリの `images_none/<rootname>/` に入る。

```bash
cd tpcanalysis-main
root -b -q 'runmacro_mini.cpp({"-convert","-norm0","-clean0","-zip0","<root>"})'
```

`<root>` を `../raw_run_files/CoBo_2026-04-21_0000.root` とした場合の出力先:
`raw_run_files/images_none/CoBo_2026-04-21_0000/{0_u.png, 0_v.png, 0_w.png, ...}`

**注意**: `runmacro_mini` は entry 数ぶんループするので、大きなファイルだと時間がかかる。

## Step 2: 新パイプライン PNG (verify_plot)

1 event だけ処理する検証マクロ。UVW PNG は Step 1 と同条件、加えて M1–M3 のストリップ座標変換 + `extractHits` を用いた XY PNG (ストリップ線ラスタライズ) を出す。

通常はインタプリタ (cling) で走らせる。これは `howto.txt` の `.L runmacro_mini.cpp` と同じ方式:

```bash
cd tpcanalysis-main
ROOT_INCLUDE_PATH=../include root -b -q 'verify_plot.cpp("<root>", 0)'
```

**ACLiC で速く走らせたい場合** (40% 程度高速, 初回は compile 込み ~15秒):

```bash
cd tpcanalysis-main
ROOT_INCLUDE_PATH=../include root -b -q \
    -e 'gSystem->AddLinkedLibs("'$PWD'/dict/build/libMyLib.dylib");' \
    'verify_plot.cpp+("<root>", 0)'
```

`AddLinkedLibs` を ACLiC の link 段階に渡す必要がある。2 回目以降は `.so` がキャッシュされるので `-e` 無しでも動く (引数部分のみで OK)。

出力先: `verification/new/<rootname>/{0_u.png, 0_v.png, 0_w.png, 0_xy.png}`

## Step 3: ピクセル比較

```bash
cd <project root>
python3 scripts/compare_png.py \
    raw_run_files/images_none/<rootname> \
    verification/new/<rootname> \
    --pattern '0_[uvw].png'
```

期待: 全ファイルが `OK` (0 pixel diff)。

**確認済みの挙動** (ROOT 6.36.10):

- 同じ ROOT バージョンで Step 1 と Step 2 を走らせると **完全ピクセル一致** (0 diff)
- `raw_run_files/images_none/` の `CoBo_2026-04-21_0000/` に既にあった PNG は、別 ROOT で生成されていたため 10-15% のピクセル差 (max_delta=236) が出た。画像サイズは同じなので ROOT のカラーパレットかアンチエイリアスの差と判断。**新パイプラインの健全性とは無関係**。

ズレた場合は `DIFF (N/T px = P%, max_delta=D)` が出る。原因候補:

- ROOT バージョン差によるアンチエイリアス / フォント / カラーパレット (Step 1 を同じ ROOT で再生成すれば一致するはず)
- `SetMinimum / SetMaximum` の設定差
- 画像サイズ (`SetWindowSize`) が違う

## Step 4: XY PNG の視覚確認

`verification/new/<rootname>/0_xy.png` を目視でチェック。

### XY の意味

`hitsToPoints` (M3) は「全 3 plane が同一 time_bin にちょうど 1 hit ずつある」という厳しい条件で、実データの多重ヒットイベントではほぼ 0 点になる。そこで XY は代わりに「各ヒットが属するストリップを physical (x, y) 平面に線として描画し、charge で重み付け」する方式で可視化する。

- U plane (θ=90°): U 軸が +Y → ストリップは **水平線**
- V plane (θ=-30°): V 軸が右下方向 → ストリップは +60° 傾いた線
- W plane (θ=30°): W 軸が右上方向 → ストリップは -60° 傾いた線

3 family の線が重なるところが実際の相互作用点の候補。

### チェックポイント

- 3 方向のストリップバンドが期待の角度で現れる
- バンドの中心が `REFERENCE POINT = (-53.25, -52.395)` mm 付近に来る
- バンドが pad 平面の物理サイズに合致 (miniTPC なら ±50〜100 mm 程度)

ここで geometry の角度や pitch の仮定 (`DIAMOND_SIZE × √3 / 2`) に明らかな破綻があれば、M3 の `stripPitchMm()` を再検討する。

**確認済み**: `CoBo_2026-04-06_0001.root` event 0 で U は数本の水平線、V / W はそれぞれ 1 本ずつ太いバンドとして現れ、交点が pad 平面中央 (−53, −52) 付近に一致した。angle/reference point は正しい。pitch の絶対値 (バンド幅) は目視では判定しづらいので、M5 で点群化した後に再評価する。
