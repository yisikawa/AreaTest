# CAreaクラスのユーティリティ関数およびデコードロジックの抽出計画

## 目的
`Area.cpp` に集中している、クラスの状態に依存しないユーティリティ関数およびデータデコードロジックを `Utils.h` / `Utils.cpp` に抽出し、コードの再利用性とメンテナンス性を向上させる。

## 実施内容

### 1. [NEW] Utils.h / Utils.cpp の作成
以下の関数群を `CArea` クラスから独立した関数として定義・実装する。

#### 文字列・数値操作
- `Trim(char *s)`
- `strrstr(char *s1, char *s2)`
- `Min4(float a, float b, float c, float d)`
- `Max4(float a, float b, float c, float d)`
- `Max5(float a, float b, float c, float d, float e)`

#### パス・ファイル操作
- `convert_path(char *path, const char *mesh_path)`

#### 3D計算
- `IsMirrorMatrix(const D3DXMATRIX *pMatrix)`
- `ComputeFaceNormal(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *p0, const D3DXVECTOR3 *p1, const D3DXVECTOR3 *p2)`

#### データデコードロジック (MMB/MZB)
- `DecodeMMBSub(char *pdat)`
- `DecodeMMB(char *pdat)`
- `DecodeMZB(char *pdat)`
- ※ 関連する `key_table`, `key_table2` 定数も `Utils.cpp` 内に閉じた static 定数として移行する。

### 2. [MODIFY] Area.h
- `CArea` クラス宣言から以下のメンバ関数の宣言を削除する。
  - `DecodeMMBSub`
  - `DecodeMMB`
  - `DecodeMZB`

### 3. [MODIFY] Area.cpp
- `#include "Utils.h"` を追加。
- 移行した関数群の実装を削除。
- `CArea` 内での呼び出し箇所を、外部関数（`::DecodeMMB` 等）の呼び出しに修正。
- **注意点**: 以下の前方宣言が `Area.cpp` 内で依然として必要となるため、削除しないよう留意する。
  - `GetFileNameFromDno`
  - `ConvertStr2Dno`

### 4. [MODIFY] AreaTest.vcxproj
- `Utils.cpp` および `Utils.h` をプロジェクト構成に追加する。

## 修正時の留意事項
- **コードの完全性**: 関数を削除する際、周辺のループ構造や条件分岐（特に `while (pos < dwSize)` や `switch` 文）を破壊しないよう慎重に範囲を指定する。
- **依存関係**: 移行する関数が `CArea` のメンバ変数（`m_Textures` 等）にアクセスしている場合は、引数として渡すように変更する。
