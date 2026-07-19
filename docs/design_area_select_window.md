# 詳細設計書：Area Select Window UI再設計

**対象ブランチ**: dx11  
**対象ファイル**: resource.rc / WinMain.cpp / Area.h  
**作成日**: 2026-05-12  
**更新日**: 2026-05-16（実装完了に合わせて実態反映）

---

## 0. 前提条件（実装完了）

**実現方式**: カスタムウィンドウクラス登録（`RegisterClass` + `CreateWindowEx`）を採用。  
`WinMain()` でウィンドウクラス `"AreaTestCanvas"` を登録し、`WM_INITDIALOG` 内で `CreateWindowEx` によりダイアログの子ウィンドウとして生成する。

| 方式 | 採否 |
|---|---|
| STATIC コントロール + SS_OWNERDRAW | 不採用 |
| カスタムウィンドウクラス登録 | **採用** |
| 子ウィンドウ（WS_CHILD）直接生成 | 上記と同義・採用済み |

---

## 1. 変更目的と方針

| 項目 | 変更前 | 変更後 |
|---|---|---|
| COMBO1 | Area選択 | **変更なし** |
| COMBO2 | Ground 表示距離 | **変更なし** |
| COMBO5 | Props 表示距離 | **変更なし** |
| COMBO3 | エフェクトクラス名一覧（ラベル "class"） | CAreaMesh の m_AreaName 一覧（ラベル "Mesh"） |
| COMBO4 | エフェクトインスタンス一覧（ラベル "effect"） | 有効な EffectModel を持つ CEffect の一覧（ラベル "Effect"） |
| LIST1 | エフェクトプロパティ | **削除** → ナビゲーションボタン行＋カスタム描画エリアに置き換え |
| LIST2 | キーフレーム値 | **削除** → カスタム描画エリアに統合 |

旧エフェクト表示（class / effect / outputProp / outputValue 連動）は**完全削除**。

---

## 2. 現状分析

### 2-1. ダイアログレイアウト（変更前）

変更前の IDD_DIALOG1 レイアウト。単位はダイアログ単位（DU）。ダイアログサイズ = 292 × 298 DU。

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
| m_AreaName | char[16] | 16バイトバイナリID（ファイルから直読み） | GetAreaName() |
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
| m_lifeTime | DWORD | **ライフタイム**（ミリ秒） |

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
| m_ModelTotal | DWORD | 総モデル数（フリップブック枚数） |
| m_NumVertices / m_NumFaces | unsigned long | 頂点数・面数 |

格納場所：`CArea::m_EffectModels`（CList型連結リスト、**protected**）  
登録順：LoadEffectModelFromFile() 内で `InsertTop()` → **逆順に積まれる**

#### OBJINFO（オブジェクト配置情報）

| メンバ | 型 | 内容 |
|---|---|---|
| mObj.id | char[16] | m_AreaName と対応するID |
| mObj.fTransX/Y/Z | float | 配置座標 |
| mObj.fRotX/Y/Z | float | 回転 |
| mObj.fScaleX/Y/Z | float | スケール |
| pAreaMesh | CAreaMesh* | 対応メッシュへのポインタ |

格納場所：`CArea::m_pObjInfo[]`（動的配列、要素数 = `m_nObj`、**protected**）

### 2-3. アクセス制御

以下のメンバはすべて `CArea` の **protected**。追加アクセサで公開済み。

| メンバ | 型 | アクセサ |
|---|---|---|
| m_AreaMeshs | CList | GetAreaMeshs() |
| m_EffectModels | CList | GetEffectModels() |
| m_nObj | int | GetNObj() |
| m_pObjInfo | OBJINFO* | GetObjInfo(int i)（0 ≤ i < m_nObj の境界チェック付き） |

`m_Effects`（CEffect リスト）は **public** メンバのため追加アクセサ不要。`g_mArea.m_Effects` で直接アクセス。

### 2-4. 削除した処理

```
[COMBO3 変更] ─→ COMBO4 フィルタリング           ← 削除済み
[COMBO4 変更] ─→ LIST1 に outputProp() 表示      ← 削除済み
[LIST1 選択]  ─→ LIST2 に outputValue() 表示     ← 削除済み
```

---

## 3. 変更後の設計（実装済み）

### 3-1. ダイアログレイアウト（変更後）

```
┌─────────────────────────────────────────────────────────────┐
│ Area Select Window                                          │
├─────────────────────────────────────────────────────────────┤
│ Area   [───────────── IDC_COMBO1 (w=248) ─────────────── ▼]│  y=12
│ Ground [── IDC_COMBO2 (w=110) ──▼]  Props  [IDC_COMBO5 ──▼]│  y=28
│ Mesh   [── IDC_COMBO3 (w=110) ──▼]  Effect [IDC_COMBO4 ──▼]│  y=43
├─────────────────────────────────────────────────────────────┤
│  [Top]          [<]          [>]          [Last]           │  y=59, h=14
├─────────────────────────────────────────────────────────────┤
│                                                             │
│         カスタム描画エリア（キャンバス）                     │  y=75, h=217
│         スクロール対応・行ハイライト付き                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

ダイアログサイズは変更前と同じ 292 × 298 DU。

**ナビゲーションボタン**（y=59、各 w=68, h=14）：

| コントロールID | ラベル | 機能 |
|---|---|---|
| IDC_BTN_OBJ_TOP | Top | 先頭インスタンスへジャンプ |
| IDC_BTN_OBJ_PREV | < | 前のインスタンスへジャンプ |
| IDC_BTN_OBJ_NEXT | > | 次のインスタンスへジャンプ |
| IDC_BTN_OBJ_LAST | Last | 末尾インスタンスへジャンプ |

**カスタム描画キャンバス**：x=7, y=75, w=278, h=217（DU）  
`MapDialogRect` で変換後 `CreateWindowEx` により生成。ウィンドウクラス名 `"AreaTestCanvas"`。

---

### 3-2. クラス変更設計（Area.h）

`GetArea()` / `SetArea()` の直後に以下4つのアクセサを **public** セクションに追加（実装済み）。

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

#### m_AreaName の表示処理（SanitizeAreaName）

`m_AreaName` は16バイトバイナリのため、以下の手順でサニタイズして表示する。

1. 先頭から `\0` が現れる位置までを有効文字列長とする
2. 0x20 未満・0x7F 以上の文字はスペースに置換する
3. 結果が空文字列の場合は `"(empty)"` を返す

#### 列挙順序

`GetAreaMeshs().Top()` → `->Next` の順（InsertEnd 挿入順 = ファイル出現順）。  
COMBO3 の選択インデックス → `GetAreaMeshs().Data(index)` で CAreaMesh を逆引き。

---

### 3-4. COMBO4（Effect）の列挙仕様

#### フィルタ条件

`m_Effects` を走査し、**`m_pEffectModel != nullptr`** の CEffect のみを対象とする。

#### 表示フォーマット

```
[000] <m_name>  class:<m_class>  -> <m_pEffectModel->m_Name>
[001] ...
```

※ 矢印は ASCII の `->` を使用。

#### 列挙順序

`InsertTop()` で積まれているため連結リスト上は**逆順**。追加処理なし。

#### インデックス→CEffect の対応

```cpp
static std::vector<CEffect*> g_combo4Effects;
```

列挙時に `g_combo4Effects` へ有効な `CEffect*` を積み、選択インデックス `sel` → `g_combo4Effects[sel]` で取得。

#### 描画エリアへの表示項目（Effect選択時：SetCanvasEffect）

```
[ Effect Info ]
Name      : <m_name>
Class     : <m_class>
lifetime  : <m_lifeTime> ms
pos       : X=...  Y=...  Z=...
rot  [rad]: X=...  Y=...  Z=...
scale     : X=...  Y=...  Z=...
color RGBA: R=...  G=...  B=...  A=...
--- EffectModel ---
Name      : <m_pEffectModel->m_Name>
Type      : <m_pEffectModel->m_type（先頭4文字）>
ModelType : <m_pEffectModel->m_ModelType>
ModelNo   : <m_pEffectModel->m_ModelNo>
ModelTotal: <m_pEffectModel->m_ModelTotal>
```

ModelType == 0x21（フリップブック）の場合、以下を追加表示：

```
Flipbook  : <ModelTotal> frames（平方数の場合 <N> x <N> grid）
Frame dur : <lifeTime / ModelTotal> ms/frame（lifeTime == 0 の場合 "(no lifetime)"）
```

#### 描画エリアへの表示項目（Mesh選択時：SetCanvasMesh）

```
[ Mesh Info ]
AreaName  : <SanitizeAreaName(m_AreaName)>
AreaType  : <m_AreaType（先頭4文字）>
Vertices  : <m_NumVertices>
Faces     : <m_NumFaces>
BoxLow    : X=...  Y=...  Z=...
BoxHigh   : X=...  Y=...  Z=...
--- Instances ---
OBJ[nnn]  pos(<fTransX>,<fTransY>,<fTransZ>)   ← 選択中インスタンス行はハイライト表示
...
```

インスタンスが存在しない場合は `"  (no instances)"` を表示。

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
    │       → COMBO3[0] を自動選択
    │
    └─ COMBO4 クリア + g_combo4Effects.clear()
        └─ g_mArea.m_Effects.Top() から ->Next で走査
            m_pEffectModel != nullptr のものだけ：
                g_combo4Effects に CEffect* を push_back
                [nnn] <name>  class:<class>  -> <model name> 形式で CB_ADDSTRING
            → COMBO4[0] を自動選択
            → Effect があれば SetCanvasEffect(先頭Effect)
              なければ SetCanvasMesh(先頭Mesh)

[COMBO3 変更]
    └─ GetAreaMeshs().Data(選択インデックス) で CAreaMesh を取得
        → SetCanvasMesh(pMesh) を呼び出し：
            SetHighlightMesh(pMesh)  // 3Dビュー上でエッジハイライト
            Mesh Info + Instances 一覧をキャンバスに表示

[COMBO4 変更]
    └─ g_combo4Effects[選択インデックス] で CEffect* を取得
        → SetCanvasEffect(pEff) を呼び出し：
            SetHighlightEffect(pEff)  // 3Dビュー上でエフェクト単体描画
            Effect Info + EffectModel Info をキャンバスに表示

[ナビゲーションボタン操作]
    └─ MoveToObjInst(instIdx) を呼び出し：
        g_objInstCur = instIdx
        g_canvasHighlightLine = g_instBaseLine + instIdx
        キャンバスを当該行にスクロール・ハイライト更新
        MoveToObject(g_objInstances[instIdx])
            → g_mAt を OBJ の fTransX/Y/Z にセット
            → カメラ注視点を更新

[COMBO2 / COMBO5 変更]
    └─ 変更なし（従来どおり g_mDispArea / g_mDispTree を更新）
```

---

### 3-6. 削除した処理一覧

| 処理 | 削除理由 |
|---|---|
| COMBO3 → COMBO4 フィルタリング | COMBO3/4 が独立になったため |
| LIST1 ← CEffect::outputProp() | COMBO4 の役割変更のため |
| LIST2 ← CKeyFrame::outputValue() | LIST1/2 を削除したため |
| IDC_LIST1 コントロール（resource.rc） | カスタム描画エリアに置き換え |
| IDC_LIST2 コントロール（resource.rc） | カスタム描画エリアに置き換え |

`CEffect::outputProp()` / `CKeyFrame::outputValue()` 自体は EffectSystem.cpp から**削除しない**。

#### 追加したグローバル変数（WinMain.cpp）

```cpp
static std::vector<CEffect*> g_combo4Effects;     // COMBO4 列挙時に構築
static std::vector<int>  g_objInstances;           // Mesh選択時のインスタンス ObjInfo インデックス
static int               g_objInstCur = -1;        // 現在のインスタンスカーソル位置
static int               g_instBaseLine = 0;       // キャンバス上のインスタンス開始行インデックス
static int               g_canvasHighlightLine = -1; // キャンバスハイライト行（-1=なし）
static int               g_scrollPos = 0;          // キャンバス縦スクロール位置（px）
```

---

## 4. 変更ファイル一覧

| ファイル | 変更種別 | 変更内容 |
|---|---|---|
| Area.h | 追加 | アクセサ4メソッド（GetAreaMeshs / GetEffectModels / GetNObj / GetObjInfo）をpublicセクションに追加 |
| resource.rc | 変更・削除・追加 | "class"→"Mesh"、"effect"→"Effect" のラベル変更；IDC_LIST1/IDC_LIST2 削除；IDC_BTN_OBJ_TOP/PREV/NEXT/LAST を追加 |
| WinMain.cpp | 変更・追加 | "AreaTestCanvas" ウィンドウクラス登録；CanvasProc 追加；Dlg1Proc のCOMBO3/4ハンドラ差し替え；LIST1/LIST2 関連処理削除；ナビゲーションボタンハンドラ追加；各種ヘルパー関数（SanitizeAreaName / SetCanvasMesh / SetCanvasEffect / MoveToObjInst）追加 |

**変更しないファイル**：Dx.cpp / EffectSystem.cpp / AreaMesh.cpp / Area.cpp  
**備考**：IDC_LIST1=1037 / IDC_LIST2=1038 は resource.h に残存するが、resource.rc の IDD_DIALOG1 からは削除済み。

---

## 5. 考慮事項

| 項目 | 対応状況 |
|---|---|
| カスタム描画エリアの実現可否 | 解決済み。カスタムウィンドウクラス方式で実装 |
| m_AreaName の非表示文字 | 解決済み。SanitizeAreaName() でサニタイズ |
| 有効な EffectModel を持つ Effect がゼロの場合 | 解決済み。COMBO4 は空表示、キャンバスはMesh情報を表示 |
| g_combo4Effects と COMBO4 の同期 | 解決済み。COMBO1 変更ハンドラで必ず clear() → 再列挙 |
| IDC_LIST1/2 の resource ID 再利用 | 未転用。resource.h には残存するが未使用のまま |
| キャンバス位置（y座標） | ナビゲーションボタン行（h=14+ギャップ2）を挿入したため y=75 に確定 |

---

## 6. 未決事項（全件解決済み）

| No. | 項目 | 決定内容 |
|---|---|---|
| 1 | LIST1/LIST2 の扱い | 削除。カスタム描画エリアに置き換え |
| 2 | EffectModel の表示順 | 逆順のまま表示（追加処理なし） |
| 3 | 旧エフェクト表示の扱い | 完全削除 |
| 4 | カスタム描画エリアの実現方式 | カスタムウィンドウクラス登録方式を採用 |
| 5 | Mesh選択時の描画内容 | Mesh情報 + インスタンス一覧（OBJ番号・配置座標） |
| 6 | インスタンスナビゲーション | Top/</>/ Last ボタンでカメラ注視点をジャンプ |
