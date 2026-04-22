# M8: 仕上げと拡張

**status**: todo (置き場)

ここは**必要になった時点で項目を追加**していく場所。先回りしない (KISS)。

想定候補:

- Streamlit もしくは Dash による一体化 UI (ファイル選択 + event スライダ + 3D viewer)
- `cleanUVW::substractBl` の両端 0 埋め幅 (10 / 12 bin) を config 化
- `geometry_*_50MHz.dat` などのバリアントへのランタイム切替
- ELITPC 4 AsAd 束ねの有効化 (必要になったら)
- `.graw → .root` 変換の自動化
- drift diffusion / 電場歪み補正
- 複数イベントの重ね描き / アニメーション

実装前に必ず `plan.md` と該当ミルストーンファイルを更新してから着手。
