# 詳細設計書：Area Select Window UI再設計

**対象ブランチ**: dx11  
**対象ファイル**: resource.rc / WinMain.cpp / Area.h  
**作成日**: 2026-05-12  
**更新日**: 2026-05-13

---

## 0. 前提条件（実装可否の判断基準）

**本修正は、IDC_LIST1 / IDC_LIST2 の領域を カスタム描画エリア（キャンバス）に置き換えられることを前提とする。**

| 前提 | 判断 |
|---|---|
| ダイアログ内に WM_PAINT による任意描画領域を設置できる | → **本設計を実装する** |
| 上記が実現不可能 | → **本修正全体を実装しない** |

カスタム描画エリアの実現方法（別途調査・判断が必要）：

| 方式 | 概要 |
|---|---|
| STATIC コントロール + SS_OWNERDRAW | 既存コントロールをオーナードロー化 |
| カスタムウィンドウクラス登録 | RegisterClass で描画専用クラスを定義し CreateWindow で配置 |
| 子ウィンドウ（WS_CHILD）直接生成 | ダイアログ上に WS_CHILD ウィンドウを重ねて配置 |

---

## 1. 変更目的と方針

| 項目 | 現状 | 変更後 |
|---|---|---|
| COMBO1 | Area選択 | **変更なし** |
| COMBO2 | Ground 表示距離 | **変更なし** |
| COMBO5 | Props 表示距離 | **変更なし** |
| COMBO3 | エフェクトクラス名一覧 | CAreaMesh の m_AreaName 一覧（ファイル出現順） |
| COMBO4 | エフェクトインスタンス一覧 | 有効な EffectModel を持つ CEffect の一覧（逆順表示） |
| LIST1 | エフェクトプロパティ | **削除** → カスタム描画エリアに置き換え |
| LIST2 | キーフレーム値 | **削除** → カスタム描画エリアに置き換え |

旧エフェクト表示（class / effect / outputProp / outputValue 連動）は**完全削除**。

---

## 2. 現状分析

### 2-1. ダイアログレイアウト（現状）

単位はダイアログ単位（DU）。ダイアログサイズ = 292 × 298 DU。

```
┌─────────────────────────────────────────────────────────────┐
│ Area Select Window                                          │
├─────────────────────────────────────────────────────────────┤
│ Area   [───────────── IDC_COMBO1 (w=248) ─────────────── ▼]│  y=12
│ Ground [── IDC_COMBO2 (w=110) ──▼]  Props [IDC_COMBO5 ──▼] │  y=28
│ class  [── IDC_COMBO3 (w=110) ──▼]  effect[IDC_COMBO4 ──▼] │  y=43
├─────────────────────────────────────────────────────────────┤
│                                                             │
│              IDC_LIST1  (278 × 139 DU)                     │  y=59
│                                                             │
├─────────────────────────────────────────────────────────────┤
│              IDC_LIST2  (278 × 90  DU)                     │  y=202
└─────────────────────────────────────────────────────────────┘
```

### 2-2. 現在のデータ構造

#### CAreaMesh（メッシュ本体）

| メンバ | 型 | 内容 | 既存アクセサ |
|---|---|---|---|
| m_AreaName | std::string | 16バイトバイナリID（ファイルから直読み） | GetAreaName() |
| m_AreaType | std::string | 4バイト種別コード | GetAreaType() |
| m_NumVertices | unsigned long | 頂点数 | GetNumVertices() |
| m_NumFaces | unsigned long | 面数 | GetNumFaces() |
| m_BoxLow / m_BoxHigh | XMFLOAT3 | バウンディングボックス | GetBoxLow/High() |

格納場所：`CArea::m_AreaMeshs`（CList型連結リスト、**protected**）  
登録順：LoadAreaFromFile() 内で `InsertEnd()` → ファイル出現順 = 配列番号順

#### CEffect（エフェクトインスタンス）

| メンバ | 型 | 内容 |
|---|---|---|
| m_class | std::string | エフェクトクラス名 |
| m_name | std::string | エフェクト名 |
| m_target | std::string | 4バイト種別コード（EffectModel の m_type と照合） |
| m_ModelType | int | モデル種別番号（EffectModel の m_ModelType と照合） |
| m_pEffectModel | CEffectModel* | 紐付けされた EffectModel へのポインタ（nullptr = 紐付けなし） |
| m_pAreaMesh | CAreaMesh* | 紐付けされた AreaMesh へのポインタ |
| m_p01 | XMFLOAT3 | **配置座標**（x, y, z） |
| m_r09 | XMFLOAT3 | **回転**（x, y, z、単位：ラジアン） |
| m_s0F | XMFLOAT3 | **スケール**（x, y, z） |
| m_color | XMFLOAT4 | **色**（r, g, b, a） |

格納場所：`CArea::m_Effects`（CList型連結リスト、**public**）  
登録順：`LoadEffectFromFile()` 内で `InsertTop()` → **逆順に積まれる**  
紐付け：`LoadEffectFromFile()` の末尾ループで `m_target` と `m_ModelType` を EffectModel と照合し `m_pEffectModel` を設定

#### CEffectModel（エフェクトモデル）

| メンバ | 型 | 内容 |
|---|---|---|
| m_Name | std::string | モデル名 |
| m_type | std::string | 4バイト種別コード |
| m_ModelType | int | モデル種別番号 |
| m_ModelNo | DWORD | モデル番号 |
| m_ModelTotal | DWORD | 総モデル数 |
| m_NumVertices / m_NumFaces | unsigned long | 頂点数・面数 |

格納場所：`CArea::m_EffectModels`（CList型連結リスト、**protected**）  
登録順：LoadEffectModelFromFile() 内で `InsertTop()` → **逆順に積まれる**（逆順表示で対応）

#### OBJINFO（オブジェクト配置情報）

| メンバ | 型 | 内容 |
|---|---|---|
| mObj.id | char[16] | m_AreaName と対応するID |
| mObj.fTransX/Y/Z | float | 配置座標 |
| mObj.fRotX/Y/Z | float | 回転 |
| mObj.fScaleX/Y/Z | float | スケール |
| pAreaMesh | CAreaMesh* | 対応メッシュへのポインタ |

格納場所：`CArea::m_pObjInfo[]`（動的配列、要素数 = `m_nObj`、**protected**）

### 2-3. アクセス制御の問題

以下のメンバはすべて `CArea` の **protected** で WinMain.cpp から直接アクセス不可。

| メンバ | 型 | 追加するアクセサ |
|---|---|---|
| m_AreaMeshs | CList | GetAreaMeshs() |
| m_EffectModels | CList | GetEffectModels()（※combo4 用途では不要になったが既存コードのために維持） |
| m_nObj | int | GetNObj() |
| m_pObjInfo | OBJINFO* | GetObjInfo(int i) ※境界チェック付き |

`m_Effects`（CEffect リスト）は **public** メンバのため追加アクセサ不要。WinMain.cpp から直接 `g_mArea.m_Effects` でアクセスできる。

### 2-4. 現在の処理フロー（削除対象）

```
[COMBO3 変更] ─→ COMBO4 フィルタリング           ← 削除
[COMBO4 変更] ─→ LIST1 に outputProp() 表示      ← 削除
[LIST1 選択]  ─→ LIST2 に outputValue() 表示     ← 削除
```

---

## 3. 変更後の設計

### 3-1. ダイアログレイアウト（変更後）

```
┌─────────────────────────────────────────────────────────────┐
│ Area Select Window                                          │
├─────────────────────────────────────────────────────────────┤
│ Area   [───────────── IDC_COMBO1 (w=248) ─────────────── ▼]│  変更なし
│ Ground [── IDC_COMBO2 ──▼]  Props  [── IDC_COMBO5 ───────▼]│  変更なし
│ Mesh   [── IDC_COMBO3 ──▼]  Effect [── IDC_COMBO4 ───────▼]│  ラベルのみ変更
├─────────────────────────────────────────────────────────────┤
│                                                             │
│         カスタム描画エリア（キャンバス）                     │  LIST1/2 を置き換え
│         ※ IDC_LIST1 / IDC_LIST2 は削除                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

カスタム描画エリアのサイズ：x=7, y=59, w=278, h=233（LIST1+LIST2 の合計領域）

---

### 3-2. クラス変更設計（Area.h）

既存の `GetArea()` / `SetArea()` の直後に以下4つのアクセサを **public** セクションに追加する。

| アクセサ名 | 戻り値型 | 返す対象 |
|---|---|---|
| GetAreaMeshs() | CList& | m_AreaMeshs |
| GetEffectModels() | CList& | m_EffectModels |
| GetNObj() | int | m_nObj |
| GetObjInfo(int i) | OBJINFO* | m_pObjInfo[i]（0 ≤ i < m_nObj の範囲チェック付き） |

---

### 3-3. COMBO3（メッシュ名）の列挙仕様

#### 表示フォーマット

```
[000] <AreaName>
[001] <AreaName>
...
```

#### m_AreaName の表示処理

`m_AreaName` は16バイトバイナリのため、そのまま表示するとコンボボックスが乱れる。  
以下の手順でサニタイズしてから表示する。

1. 先頭から `\0` が現れる位置までを有効文字列長とする
2. 0x00〜0x1F・0x7F以上の文字はスペースに置換する

#### 列挙順序

`GetAreaMeshs().Top()` → `->Next` の順（InsertEnd 挿入順 = ファイル出現順）を配列番号とする。  
COMBO3 の選択インデックス → `GetAreaMeshs().Data(index)` で CAreaMesh を逆引き。

---

### 3-4. COMBO4（Effect）の列挙仕様

#### フィルタ条件

`m_Effects` を走査し、**`m_pEffectModel != nullptr`** の CEffect のみを対象とする。  
`m_pEffectModel` は `LoadEffectFromFile()` 末尾で設定済みなので、列挙時の追加照合は不要。

#### 表示フォーマット

```
[000] <m_name>  class:<m_class>  → <m_pEffectModel->m_Name>
[001] ...
```

#### 列挙順序

`InsertTop()` で積まれているため連結リスト上は**逆順**。  
→ フィルタリングしながらリスト順（ファイル出現の逆）に追加。追加処理なし。

#### インデックス→CEffect の対応

COMBO4 の選択インデックス `sel` は、フィルタ後の連番であるため直接 `m_Effects.Data(sel)` では取得できない。  
代わりに列挙時に別途インデックスカウンタ（またはポインタ配列）を保持して対応する。

推奨実装：列挙時に `std::vector<CEffect*> g_combo4Effects` に有効な CEffect* を積み、  
`sel` → `g_combo4Effects[sel]` で取得。

#### 描画エリアへの表示項目

COMBO4 で CEffect が選択されたとき、カスタム描画エリアに以下を表示する。

| 項目 | ソース | 表示例 |
|---|---|---|
| エフェクト名 | m_name | `name: tpA1` |
| クラス名 | m_class | `class: taki` |
| 配置座標 | m_p01 (XMFLOAT3) | `pos: (0.00, -0.17, 10.35)` |
| 回転（ラジアン） | m_r09 (XMFLOAT3) | `rot: (0.00, 3.14, 0.00) rad` |
| スケール | m_s0F (XMFLOAT3) | `scale: (0.16, 0.20, 0.20)` |
| 色 (RGBA) | m_color (XMFLOAT4) | `color: (0.50, 0.50, 0.50, 1.00)` |
| リンク EffectModel | m_pEffectModel->m_Name | `model: <Name>` |
| EffectModel 種別 | m_pEffectModel->m_type | `type: tkm1` |
| EffectModel モデル種別 | m_pEffectModel->m_ModelType | `modelType: 31` |

---

### 3-5. 変更後の処理フロー

```
[COMBO1 変更]
    │
    ├─ g_mArea.LoadMAP()
    │
    ├─ COMBO3 クリア
    │   └─ GetAreaMeshs().Top() から ->Next で走査
    │       各 m_AreaName をサニタイズして [nnn] 形式で CB_ADDSTRING
    │       → COMBO3[0] を自動選択 → 描画エリアを更新
    │
    └─ COMBO4 クリア + g_combo4Effects クリア
        └─ g_mArea.m_Effects.Top() から ->Next で走査
            m_pEffectModel != nullptr のものだけ：
                g_combo4Effects に CEffect* を push_back
                m_name / m_class / m_pEffectModel->m_Name を整形して CB_ADDSTRING
            → COMBO4[0] を自動選択

[COMBO3 変更]
    └─ GetAreaMeshs().Data(選択インデックス) で CAreaMesh を取得
        → 描画エリアに選択メッシュの情報を描画（詳細は描画エリア設計に委ねる）

[COMBO4 変更]
    └─ g_combo4Effects[選択インデックス] で CEffect* を取得
        → 描画エリアに以下を表示：
            name, class
            pos: m_p01.x/y/z
            rot: m_r09.x/y/z (rad)
            scale: m_s0F.x/y/z
            color: m_color.x/y/z/w (rgba)
            linked model: m_pEffectModel->m_Name / m_type / m_ModelType

[COMBO2 / COMBO5 変更]
    └─ 変更なし（従来どおり g_mDispArea / g_mDispTree を更新）
```

---

### 3-6. 削除する処理一覧

| 処理 | 現在の場所 | 削除理由 |
|---|---|---|
| COMBO3 → COMBO4 フィルタリング | Dlg1Proc IDC_COMBO3 ハンドラ | COMBO3/4 が独立になるため |
| LIST1 ← CEffect::outputProp() | Dlg1Proc IDC_COMBO4 ハンドラ | COMBO4 の役割変更のため |
| LIST2 ← CKeyFrame::outputValue() | Dlg1Proc IDC_LIST1 ハンドラ | LIST1/2 を削除するため |
| IDC_LIST1 コントロール | resource.rc | カスタム描画エリアに置き換え |
| IDC_LIST2 コントロール | resource.rc | カスタム描画エリアに置き換え |

`CEffect::outputProp()` / `CKeyFrame::outputValue()` 自体は EffectSystem.cpp から**削除しない**。

#### 追加するグローバル変数

WinMain.cpp に以下を追加し、COMBO4 のインデックスと CEffect* の対応を保持する。

```cpp
static std::vector<CEffect*> g_combo4Effects;  // COMBO4 列挙時に構築、LoadMAP() 後に再構築
```

---

## 4. 変更ファイル一覧

| ファイル | 変更種別 | 変更内容 |
|---|---|---|
| Area.h | 追加 | アクセサ4メソッドを public セクションに追加 |
| resource.rc | 変更・削除 | "class"→"Mesh"、"effect"→"Effect"のラベル変更、IDC_LIST1/IDC_LIST2 削除、カスタム描画エリア用コントロール追加 |
| WinMain.cpp | 変更 | Dlg1Proc の COMBO3・COMBO4 ハンドラ差し替え、LIST1/LIST2 関連処理を削除 |

**変更しないファイル**：Dx.cpp / Render.cpp / EffectSystem.cpp / AreaMesh.cpp / Area.cpp

---

## 5. 考慮事項・リスク

| 項目 | 内容 | 対応 |
|---|---|---|
| **前提条件の成立確認** | カスタム描画エリアが実現できない場合は本修正全体を中止 | 先行して描画エリアの実現可否を検証する |
| m_AreaName の非表示文字 | 16バイトバイナリに NULL・制御文字が含まれる | 表示前にサニタイズ処理を挟む |
| 有効な EffectModel を持つ Effect がゼロの場合 | エリアによっては該当 Effect が存在しない | COMBO4 を空のまま表示。g_combo4Effects が空の場合は描画エリア更新をスキップ |
| g_combo4Effects と COMBO4 の同期 | LoadMAP() 呼び出し後に必ず再列挙が必要 | COMBO1 変更ハンドラで g_combo4Effects.clear() → 再列挙を忘れないこと |
| IDC_LIST1/2 の resource ID 再利用 | resource.h の IDC_LIST1=1037・IDC_LIST2=1038 が未使用になる | カスタム描画エリアの ID として転用、または削除 |

---

## 6. 未決事項（解決済み）

| No. | 項目 | 決定内容 |
|---|---|---|
| 1 | LIST1/LIST2 の扱い | 削除。カスタム描画エリアに置き換え |
| 2 | EffectModel の表示順 | 逆順のまま表示（追加処理なし） |
| 3 | 旧エフェクト表示の扱い | 完全削除 |
