# M4: リファレンス PNG との突き合わせ (ひと段落チェックポイント)

**status**: done

## ゴール

M1〜M3 で作った新パイプラインの健全性を、**`tpcanalysis-main/howto.txt` の既存コマンドで生成した PNG とピクセル一致で比較**して確認する。加えて、新パイプラインだけが持つ **XY 2D (時間積分)** の PNG を出して geometry 変換の正しさを視覚確認する。

## 比較対象と期待

- **UVW 2D**: 既存の `runmacro_mini({"-convert","-norm0","-clean0","-zip0","<file>.root"})` と**ピクセル一致**
  - 同条件 (norm=false, clean=false) で新パイプラインからも PNG を出す。
  - ROOT 描画のバージョン依存があり得るため、完全一致が難しい場合はしきい値付き一致 (diff ピクセル率 < 1% など) にフォールバックして良い。その場合はしきい値と根拠を受け入れ条件に追記する。
- **XY 2D**: 新規成果物のため自動比較対象なし。**視覚確認のみ**。
  - 時間積分 (全 time bin のヒット電荷を合算) して `(x_mm, y_mm)` 平面にヒストグラム化。
  - geometry の角度・REFERENCE POINT・pad_pitch が正しければ、pad 平面の物理境界内に収まった分布になる。

## 受け入れ条件

- `verify_plot.cpp` (仮) の 1 コマンド実行で、同じ入力 `.root` から以下が出る:
  - 新パイプラインの UVW PNG (`<event>_u.png`, `_v.png`, `_w.png`) — `norm=false, clean=false` 版
  - 新パイプラインの XY PNG (`<event>_xy.png`) — 時間積分
- 既存 `runmacro_mini` の `-convert -norm0 -clean0 -zip0` を別途走らせて reference PNG を取得する手順を README か `docs/verify.md` に書く。
- 両者の UVW PNG の差分を取るスクリプト (`scripts/compare_png.py` 等) を用意し、1 イベント以上で OK を確認する。
  - 完全一致 → そのまま合格
  - 一部ズレ → 差分率と原因 (アンチエイリアス / フォント / カラーパレット) を記録

## 実装方針 (KISS)

- 新マクロは `tpcanalysis-main/verify_plot.cpp` に 1 本。本番マクロ (M5) と**完全に別物**にする (本番は norm=true, clean=true 固定のまま)。
- `verify_plot.cpp` は既存 `drawUVWimage_mini` とほぼ同じロジックにして、norm/clean を切って PNG を出すだけ。XY は M3 の点群を `(x, y)` に射影して `TH2D` に charge で積むだけ。
- PNG 差分は Python の `PIL.ImageChops.difference` で十分。依存を増やすなら `numpy` のみ追加。
- 出力先は `verification/<入力名>/{new,reference}/<event>_{u,v,w,xy}.png`。ディレクトリ構造だけで対になっているのがわかる形。

## 本番パイプラインへの影響

- **なし**。M5 (ファイル名 1 引数の本番マクロ) では `norm=true, clean=true` 固定を維持する。
- `verify_plot.cpp` は検証専用で、CI や日常運用に組み込まない。

## M4 実行結果 (2026-04-22)

- 入力: `raw_run_files/CoBo_2026-04-06_0001.root`, event 0
- **UVW PNG ピクセル比較**: 同じ ROOT (6.36.10) で `runmacro_mini -convert -norm0 -clean0` と `verify_plot` を走らせると **0 pixel diff** (完全一致)。既存の `raw_run_files/images_none/` にあった PNG は別 ROOT で生成されていて 10-15% 差 (max_delta=236) が出たが、同一 ROOT での再生成で解消。**パイプラインの健全性には無関係**と結論。
- **XY PNG**: `hitsToPoints` (M3) は実データ多重ヒットでは 0 点になるので、XY は各ヒットのストリップ線を (x, y) にラスタライズする方式に変更。U (水平) / V (+60°) / W (-60°) の 3 ストリップ family が期待角度で現れ、REFERENCE_POINT (-53, -52) mm 付近にバンドが集中。**angle と reference point は正しい**。pitch の絶対値は目視判定できず、M5 点群化後に再評価。
- **ACLiC 不使用**: ROOT 6.36 の ACLiC + GET ディクショナリでリンクが壊れる (`Class()` / `Streamer` / vtable が未解決)。cling インタプリタ (`root -q 'file.cpp(args)'`) で走らせる + `ROOT_INCLUDE_PATH=../include` を事前に export。

## やらないこと

- ACLiC のリンクエラー解決 (cling で動くので不要)
- ピクセル不一致を完全に 0 にすること (ROOT バージョン差を追い込む価値は薄い)
- XY の自動比較 (reference がないため)
- 複数ファイルのバッチ検証
- 検証結果を CI テストに組み込むこと
