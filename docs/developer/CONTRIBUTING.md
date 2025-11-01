# Contributing to SPHCODE

このドキュメントでは、SPHCODEプロジェクトへの貢献方法について説明します。

## 目次
- [開発環境のセットアップ](#開発環境のセットアップ)
- [プロジェクト構造](#プロジェクト構造)
- [新しいサンプルの追加](#新しいサンプルの追加)
- [新しいSPH手法の実装](#新しいsph手法の実装)
- [新しいモジュールの追加](#新しいモジュールの追加)
- [コードスタイル](#コードスタイル)
- [テスト](#テスト)
- [ドキュメント](#ドキュメント)

## 開発環境のセットアップ

### Nix環境（推奨）
```bash
# Nixがインストールされている場合
nix develop

# 開発環境が自動的にセットアップされます
# - clang 18.1.8
# - CMake 3.30.5
# - Boost 1.86.0
# - Python 3.13, uv
```

### 手動セットアップ

#### macOS (Homebrew)
```bash
brew install llvm libomp boost cmake

# 環境変数の設定
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install clang libomp-dev libboost-all-dev cmake
```

### Python環境
```bash
# uvのインストール
curl -LsSf https://astral.sh/uv/install.sh | sh

# 依存パッケージのインストール
uv sync
```

### ビルド確認
```bash
rm -rf build && mkdir build && cd build
cmake ..
make -j8

# テスト実行
./sph shock_tube ../sample/shock_tube/shock_tube.json 1
```

## プロジェクト構造

```
sphcode/
├── src/               # C++ソースコード
│   ├── core/         # シミュレーションフレームワーク
│   ├── modules/      # モジュールシステム（相互作用、タイムステップなど）
│   ├── algorithms/   # SPH実装（ssph, disph, gsph, gdisph）
│   ├── tree/         # 近傍粒子探索
│   └── utilities/    # ヘルパー関数
├── include/          # ヘッダファイル（src/と同じ構造）
├── sample/           # サンプルシミュレーション設定
├── configs/          # 設定ファイルテンプレート
├── analysis/         # Python解析ツール
│   ├── readers.py
│   ├── conservation.py
│   ├── plotting.py
│   ├── theoretical.py
│   └── cli/
└── test/            # テストコード
```

詳細は [ARCHITECTURE.md](ARCHITECTURE.md) をご覧ください。

## 新しいサンプルの追加

### 1. サンプルディレクトリの作成
```bash
cd sample
mkdir my_new_test
cd my_new_test
```

### 2. JSON設定ファイルの作成
`configs/base/` からテンプレートをコピーして編集：
```bash
cp ../../configs/base/ssph_2d_template.json my_new_test.json
```

主要なパラメータ：
```json
{
  "outputDirectory": "results/SSPH/my_new_test/2D/",
  "endTime": 1.0,
  "outputTime": 0.01,
  "SPHType": "ssph",
  "gamma": 1.4,
  "kernel": "cubic_spline",
  "neighborNumber": 32,
  "useGravity": false,
  "periodic": false
}
```

### 3. 初期条件の実装
`src/core/sample_registry.cpp` に初期条件生成関数を追加：

```cpp
#include "particle.hpp"
#include <vector>

namespace {
std::vector<Particle> make_my_new_test() {
    std::vector<Particle> particles;
    
    // 粒子の初期化
    for (int i = 0; i < N; ++i) {
        Particle p;
        p.r = /* 位置 */;
        p.v = /* 速度 */;
        p.m = /* 質量 */;
        p.u = /* 内部エネルギー */;
        particles.push_back(p);
    }
    
    return particles;
}
}

// SampleRegistry::get_initial_condition() に追加：
if (name == "my_new_test") {
    return make_my_new_test();
}
```

### 4. レジストリへの登録
`SampleRegistry::get_initial_condition()` の `if-else` チェインに追加。

### 5. README作成
サンプルディレクトリに `README.md` を作成して説明を記載：

```markdown
# My New Test

## 概要
このテストの物理的な説明...

## パラメータ
- 次元: 2D
- 粒子数: 10000
- SPH手法: Standard SPH
- 期待される結果: ...

## 実行方法
\`\`\`bash
cd build
./sph my_new_test ../sample/my_new_test/my_new_test.json 4
\`\`\`

## 参考文献
...
```

### 6. テスト
```bash
cd build
make
./sph my_new_test ../sample/my_new_test/my_new_test.json 1

# 結果確認
cd ../sample/my_new_test/results/SSPH/my_new_test/2D/
ls -la  # CSV出力を確認
```

## 新しいSPH手法の実装

### 1. ディレクトリ構造の作成
```bash
mkdir -p src/algorithms/mysph
mkdir -p include/mysph
```

### 2. 基底クラスの継承
`include/mysph/mysph_solver.hpp`:
```cpp
#ifndef MYSPH_SOLVER_HPP
#define MYSPH_SOLVER_HPP

#include "solver.hpp"

class MySPHSolver : public Solver {
public:
    MySPHSolver(const std::string& parameter_file);
    
protected:
    void calculate_derivative() override;
    void calculate_physical_value() override;
    
private:
    // MySPH特有のメンバー変数
};

#endif
```

### 3. 実装ファイル
`src/algorithms/mysph/mysph_solver.cpp`:
```cpp
#include "mysph/mysph_solver.hpp"

MySPHSolver::MySPHSolver(const std::string& parameter_file)
    : Solver(parameter_file) {
    // 初期化
}

void MySPHSolver::calculate_derivative() {
    // SPH方程式の実装
    #pragma omp parallel for
    for (size_t i = 0; i < particles.size(); ++i) {
        // 微分項の計算
    }
}

void MySPHSolver::calculate_physical_value() {
    // 密度、圧力などの計算
}
```

### 4. ファクトリへの登録
`src/core/solver.cpp` の `Solver::create()` メソッドに追加：
```cpp
std::unique_ptr<Solver> Solver::create(const std::string& parameter_file) {
    Parameters params(parameter_file);
    std::string sph_type = params.get<std::string>("SPHType");
    
    if (sph_type == "mysph") {
        return std::make_unique<MySPHSolver>(parameter_file);
    }
    // ...
}
```

### 5. CMakeLists.txtの更新
`src/algorithms/mysph/CMakeLists.txt` を作成：
```cmake
add_library(mysph
    mysph_solver.cpp
)

target_link_libraries(mysph
    PUBLIC core
)
```

`src/algorithms/CMakeLists.txt` に追加：
```cmake
add_subdirectory(mysph)
```

### 6. テスト設定の作成
`configs/base/mysph_template.json` を作成。

## 新しいモジュールの追加

モジュールシステムについては [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) の「モジュールの追加方法」セクションを参照してください。

簡単な手順：
1. `include/module.hpp` を継承したクラスを作成
2. `initialize()`, `execute()`, `finalize()` を実装
3. `module_factory.cpp` に登録
4. JSON設定で有効化

## コードスタイル

### C++
- **標準**: C++14
- **インデント**: スペース4つ
- **命名規則**:
  - クラス名: `PascalCase` (例: `FluidForce`)
  - 関数名: `snake_case` (例: `calculate_density`)
  - 変数名: `snake_case` (例: `neighbor_list`)
  - メンバー変数: 接頭辞なし
  - 定数: `UPPER_SNAKE_CASE`
- **ヘッダーガード**: `#ifndef FILENAME_HPP` / `#define FILENAME_HPP`
- **コメント**: `//` を使用、複数行は `/* */`

### フォーマット例
```cpp
#include "particle.hpp"
#include <vector>

class MyClass {
public:
    MyClass(int param);
    void do_something();
    
private:
    int member_variable;
    std::vector<double> data;
};

void MyClass::do_something() {
    for (size_t i = 0; i < data.size(); ++i) {
        // 処理
    }
}
```

### Python
- **標準**: PEP 8
- **インデント**: スペース4つ
- **型ヒント**: 関数シグネチャに推奨
- **Docstring**: Google形式

```python
def calculate_energy(particles: list[Particle]) -> float:
    """Calculate total energy of particle system.
    
    Args:
        particles: List of SPH particles
        
    Returns:
        Total energy (kinetic + internal)
        
    Example:
        >>> energy = calculate_energy(my_particles)
    """
    return sum(p.kinetic_energy + p.internal_energy for p in particles)
```

### JSON設定ファイル
- インデント: スペース2つ
- キーはアルファベット順（論理的なグループ分けがある場合は例外）

## テスト

### C++テスト
```bash
cd build
make test  # CTestを実行（将来実装予定）
```

### サンプル実行テスト
すべてのサンプルが正常に実行できることを確認：
```bash
cd build
for sample in ../sample/*/; do
    name=$(basename "$sample")
    if [ -f "../sample/$name/$name.json" ]; then
        echo "Testing $name..."
        ./sph "$name" "../sample/$name/$name.json" 1 || echo "FAILED: $name"
    fi
done
```

### Python解析のテスト
```bash
# データ読み込みテスト
uv run python -c "from analysis import readers; print('OK')"

# 保存量チェックテスト
cd sample/shock_tube
uv run python -m analysis.cli.analyze conservation shock_tube
```

## ドキュメント

### ドキュメント構造
- **README.md**: プロジェクト概要、クイックスタート
- **ARCHITECTURE.md**: コードベース構造の詳細
- **DEVELOPER_GUIDE.md**: 開発者向け詳細ガイド
- **QUICK_REFERENCE.md**: パラメータ、コマンドのクイックリファレンス
- **CONTRIBUTING.md** (このファイル): コントリビューションガイド

### 新機能の追加時
1. コードにコメントを追加
2. 該当するドキュメントファイルを更新
3. サンプルコードや使用例を追加

### Doxygen（将来実装予定）
C++コードにはDoxygenコメントを推奨：
```cpp
/**
 * @brief Calculate SPH density summation
 * @param particles Particle array
 * @param neighbor_list Neighbor indices for each particle
 * @return void (modifies particles in-place)
 */
void calculate_density(std::vector<Particle>& particles, 
                      const NeighborList& neighbor_list);
```

## プルリクエストのガイドライン

1. **ブランチ**: `feature/your-feature-name` または `fix/bug-description`
2. **コミットメッセージ**: 英語または日本語、明確で簡潔に
   - 例: `Add Wendland C6 kernel implementation`
   - 例: `Fix energy conservation in GSPH`
3. **テスト**: 既存のサンプルが動作することを確認
4. **ドキュメント**: README/ARCHITECTURE等を必要に応じて更新

## 質問・議論
- GitHub Issues: バグ報告、機能リクエスト
- GitHub Discussions: 使い方の質問、アイデア共有

## ライセンス
コントリビューションはすべてMITライセンスの下で公開されます。
