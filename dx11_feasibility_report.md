# DX11 移行計画 実現性調査レポート

## 調査概要

現在の `dx11` ブランチ上のコードを全ファイルにわたって詳細分析した結果をまとめます。

---

## 1. 現状の DX9 依存箇所の全容

調査した結果、DX9 依存は複数のファイルにわたって広範に存在します。

### 依存ファイルマップ

| ファイル | DX9 依存の内容 | 重大度 |
|:---|:---|:---|
| `Dx.h` / `WinMain.h` | `#include <d3d9.h>` / `<d3dx9.h>` | 🔴 高 |
| `Area.h` | `IDirect3DVertexDeclaration9*`, `IDirect3DVertexShader9*`, `D3DXMATRIX`, `D3DXVECTOR3` | 🔴 高 |
| `AreaMesh.h` | `IDirect3DTexture9*`, `LPDIRECT3DVERTEXBUFFER9`, `LPDIRECT3DINDEXBUFFER9`, `D3DXVECTOR3`, `D3DPRIMITIVETYPE` | 🔴 高 |
| `EffectSystem.h` | `IDirect3DVertexDeclaration9*`, `LPDIRECT3DVERTEXBUFFER9`, `LPDIRECT3DINDEXBUFFER9`, `D3DXMATRIX` | 🔴 高 |
| `Area.cpp` | `D3DXAssembleShader`, `D3DXSaveTextureToFile`, `D3DPOOL_MANAGED`, vs_1_1 アセンブラ, `SetRenderState`, `SetVertexShader`, etc. | 🔴 高 |
| `Render.cpp` | `IDirect3D9`, `IDirect3DDevice9`, `D3DPRESENT_PARAMETERS`, `D3DX*Matrix*`, `D3DLight9` | 🔴 高 |
| `WinMain.cpp` | `D3DXMatrix*`, `D3DXVec3*` 多数（50箇所以上） | 🔴 高 |
| `AreaMesh.cpp` | `D3DPOOL_MANAGED`, `D3DLOCK_DISCARD` | 🔸 中 |
| `Dx.cpp` | `Direct3DCreate9`, `D3DPRESENT_PARAMETERS`, `CreateDevice` | 🔴 高 |

---

## 2. 移行の難所（ファイル別詳細）

### 🔴 最難関: `Area.cpp`

**vs_1_1 アセンブラシェーダー**（L61-196）

```cpp
// 現在のコード: vs_1_1 アセンブラ (DX9専用)
"vs_1_1\n"
"dcl_position0  v0\n"
"m4x3  r5, v0, c[10]\n"
...
// D3DXAssembleShader でコンパイルしている
hr = D3DXAssembleShader( pVertexShaders[0], ... );
```

> [!CAUTION]
> `vs_1_1` アセンブリシェーダーと `D3DXAssembleShader` は **DX11 では完全廃止**。
> ただし、`hlsl.fx` が **既にDX11用 HLSL で書き直されている**ため、
> この部分は `hlsl.fx` に置き換えることで解決可能。

**テクスチャ生成（独自フォーマット解析）**（L306-360）

```cpp
// DXT3/DXT1/パレット形式をロックして直接書き込む
hr = GetDevice()->CreateTexture(xx,yy,0,0,D3DFMT_DXT3,D3DPOOL_MANAGED,&pTex,NULL);
hr = pTex->LockRect(0, &rc, NULL, 0);
CopyMemory(rc.pBits, pdat+pos+33+0x28+12, ...);
```

> [!WARNING]
> `LockRect` と `D3DPOOL_MANAGED` は **DX11 に存在しない**。
> `UpdateSubresource` または `Map/Unmap` に書き換え必要。
> DXT フォーマット自体は DX11 でも `DXGI_FORMAT_BC2_UNORM` 等で対応可能。

**レンダリングループのステート設定**（L875-1039）

```cpp
GetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
GetDevice()->SetVertexShader(m_hVertexShader);
GetDevice()->SetStreamSource(0, pAreaMesh->GetlpVB(), 0, ...);
GetDevice()->DrawIndexedPrimitive(...);
```

> [!NOTE]
> これらは全てDX11の `ID3D11DeviceContext` メソッドに対応するものがある。
> ブレンドステートは `ID3D11BlendState` オブジェクトに事前設定が必要。

---

### 🔴 高難: `WinMain.cpp` (50箇所以上の D3DX 関数)

主な D3DX 関数と DirectXMath への対応表:

| D3DX 関数 | DirectXMath 対応 |
|:---|:---|
| `D3DXMatrixLookAtLH` | `XMMatrixLookAtLH` |
| `D3DXMatrixRotationY/X` | `XMMatrixRotationY/X` |
| `D3DXMatrixScaling` | `XMMatrixScaling` |
| `D3DXMatrixMultiply` | `XMMatrixMultiply` |
| `D3DXVec3Normalize` | `XMVector3Normalize` |
| `D3DXVec3TransformNormal` | `XMVector3TransformNormal` |
| `D3DXVec3TransformCoord` | `XMVector3TransformCoord` |
| `D3DXMATRIX` | `XMMATRIX` または `XMFLOAT4X4` |
| `D3DXVECTOR3` | `XMVECTOR` または `XMFLOAT3` |

> [!TIP]
> `XMMATRIX` は SSE2 の16バイトアライメントが必要で、クラスメンバに直接使うと注意が必要。
> クラスメンバには `XMFLOAT4X4` / `XMFLOAT3` を使い、演算時に `XMLoad*` / `XMStore*` を利用するのが標準パターン。

---

### 🔴 高難: `Render.cpp`

`D3DLIGHT9`, `SetLight`, `LightEnable` は DX11 には存在せず、ライト処理を定数バッファで実装し直す必要があります。
ただし `hlsl.fx` の VS は既にライト計算なし（定数バッファ `mCOL` で色を渡す方式）なので、Render.cpp 側のライト設定コードは削除対象です。

---

### 🟢 好材料: `hlsl.fx` — 既にDX11対応済み

```hlsl
// cbuffer は DX11 の定数バッファ構文
cbuffer ConstantBuffer : register(b0) {
    matrix mWVP;
    float2 mUV;
    float4 mCOL;
}
// SV_POSITION, SV_Target は DX11 のセマンティクス
float4 PS(PS_INPUT input) : SV_Target { ... }
```

> [!NOTE]
> `hlsl.fx` は **既にDX11 HLSL として完全に書かれています**。
> VS/PS エントリポイント（`VS`, `PS`）も定義済みです。
> `D3DCompileFromFile` でそのままコンパイル可能です。

---

### 🟡 中難: テクスチャ保存 `D3DXSaveTextureToFile` (Area.cpp L1117)

```cpp
// 現在のコード
D3DXSaveTextureToFile(texpath, D3DXIFF_PNG, pTexture->GetTexture(), NULL);
```

MQO書き出し機能（`saveMQO`）でのみ使用。**DirectXTex** ライブラリの `SaveToWICFile` で代替可能。または `d3dx11.lib` は廃止済みなので DirectXTex 一択。

---

## 3. 実現性の総合評価

| 評価項目 | 評価 | 根拠 |
|:---|:---|:---|
| **技術的実現性** | ✅ 実現可能 | DX11 API は全ての機能に対応するものが存在する |
| **既存シェーダーの流用** | ✅ そのまま使える | `hlsl.fx` は DX11 対応済み |
| **テクスチャ読み込みロジック** | ✅ 移植可能 | フォーマット解析ロジックはそのまま流用、書き込みAPIを変更 |
| **数学ライブラリ移行** | 🔸 機械的だが量が多い | D3DX → DirectXMath の変換が 50 箇所以上 |
| **レンダーステート移行** | 🔸 設計変更が必要 | ステートオブジェクト化が必要 |
| **工数** | 🔸 中〜大規模 | 全ファイルに影響。バグリスクあり |

---

## 4. 推奨する移行アプローチ（段階的）

### Phase 1: インフラ整備（Dx.h / Dx.cpp の置き換え）
- DX11 デバイス・スワップチェーン初期化
- レンダーターゲット / 深度バッファの作成
- **工数目安: 0.5〜1日**

### Phase 2: 頂点・インデックスバッファ移行（AreaMesh.cpp）
- `CreateVB` / `CreateIB` を DX11 バッファ作成に変換
- `Map/Unmap` によるデータ転送に変換
- `LPDIRECT3DVERTEXBUFFER9` → `ID3D11Buffer*` の型置換
- **工数目安: 1日**

### Phase 3: テクスチャ読み込み移行（Area.cpp LoadTextureFromFile）
- `CreateTexture` + `LockRect` → `CreateTexture2D` + `UpdateSubresource`
- DXT3/DXT1 → `DXGI_FORMAT_BC2_UNORM` / `DXGI_FORMAT_BC1_UNORM`
- **工数目安: 1日**

### Phase 4: シェーダーパイプライン構築（Area.cpp CreateVertexShader / Rendering）
- `vs_1_1` アセンブラを廃止し `hlsl.fx` を使用
- `D3D11CompileFromFile` でのコンパイル
- `InputLayout` の定義（既存の `VSFormat` と `hlsl.fx` の `VS_INPUT` を対応付け）
- 定数バッファの作成・更新ループ実装
- **工数目安: 1〜2日**

### Phase 5: レンダリングループ移行（Area.cpp Rendering / Render.cpp）
- `SetRenderState` 系 → `ID3D11BlendState` / `ID3D11RasterizerState` / `ID3D11DepthStencilState` オブジェクトに事前設定
- `DrawIndexedPrimitive` → `DrawIndexed`
- ライト・フォグ関連（DX9固有）の削除
- **工数目安: 1〜2日**

### Phase 6: 数学ライブラリ移行（WinMain.cpp, Area.cpp, 全ヘッダ）
- `D3DXMATRIX` → `XMFLOAT4X4`, `XMMATRIX`
- `D3DXMatrix*` 系関数の一括置換
- **工数目安: 0.5〜1日（機械的作業が多い）**

### Phase 7: テクスチャ保存移行（saveMQO 系）
- `D3DXSaveTextureToFile` → DirectXTex の `SaveToWICFile`
- DirectXTex の NuGet 追加が必要
- **工数目安: 0.5日**

---

## 5. リスクと対策

> [!WARNING]
> **最大のリスク**: `XMMATRIX` の16バイトアライメント問題
> クラスメンバーに `XMMATRIX` を直接使うとクラッシュする場合があります。
> 対策: クラスメンバには `XMFLOAT4X4` を使用すること。

> [!CAUTION]
> **DX9 の `Dx.cpp` と `Render.cpp` に同じ初期化コードが二重定義されています**。
> 移行前に統合するか、どちらか一方を削除する必要があります。

> [!NOTE]
> **総工数見積もり**: 約 5〜8 日（実装 + デバッグ）
> `hlsl.fx` が DX11 対応済みなことで、シェーダー開発工数が大幅に削減されます。
