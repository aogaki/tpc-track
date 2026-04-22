# tpc-track プロジェクト運用メモ (Claude Code 用)

このファイルは Claude Code がセッション開始時に読み込む運用ルール。

## 開発原則

- **KISS (Keep It Simple, Stupid)**: 機能追加・抽象化・オプション化は「今必要か」を必ず問う。先回りして作らない。既定値で動くものを最優先。
- **TDD (Test-Driven Development)**: 新しいユニット (関数・クラス) は「テストを書く → 赤 → 最小実装 → 緑 → リファクタ」の順で進める。テストが書けない抽象化は入れない。C++ は Catch2 / GoogleTest、Python は pytest を基本とする (導入は最初に必要になった時点で)。

## 現状把握と次の手

**新しい作業を始める前に必ず `TODO/` ディレクトリを読む。作業後は `TODO/` を更新する。**

- [TODO/README.md](TODO/README.md): ディレクトリの使い方
- [TODO/plan.md](TODO/plan.md): プロジェクト全体の方針と原則、マイルストーン一覧
- [TODO/milestones/](TODO/milestones/): マイルストーン単位の詳細タスクと受け入れ条件

完了した項目は削除せず、`status: done` に書き換えて履歴として残す。計画が変わった時は実装より先に `plan.md` と該当マイルストーンファイルを更新する。

## プロジェクト概要

- 目的: miniTPC の `.root` ファイルを読み、**1 イベントを Plotly で 3D ぐりぐり回せる**可視化を作る。将来は 3D 直線フィットへ。
- コードベースの全体解析: [docs/tpcanalysis_overview.md](docs/tpcanalysis_overview.md)
- 上流の既存コード: [tpcanalysis-main/](tpcanalysis-main/) (C++ / ROOT マクロ)
- 生データ: [raw_run_files/](raw_run_files/) に `CoBo_2026-04-*_*.root`

## 言語分担

- **C++ / ROOT マクロ**: `.root` → UVW → ヒット点群 → `(x, y, z, charge)` の CSV/TTree 出力まで
- **Python (Plotly)**: 点群読み込み → 3D 可視化 → 3D 直線フィット

## 確認済みの物理パラメータ (miniTPC)

- サンプリング 25 MHz、512 time bin、ドリフト速度 0.724 cm/μs、トリガー遅延 10.24 μs (= 256 bin)
- ストリップ数 U=72, V=92, W=92、角度 (90°, -30°, 30°)
- 1 イベントの 3D 点数は数百〜数千オーダー想定
