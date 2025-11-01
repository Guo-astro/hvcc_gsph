# SPHCODE
Smoothed Particle Hydrodynamics (SPH)法のサンプルコードです。圧縮性流体専用です。

> **🎉 最近のアップデート (2025-11-01):**  
> **Checkpoint/Resume System**: 長時間シミュレーションの一時停止・再開機能を実装！  
> - 自動チェックポイント保存（設定可能な間隔）  
> - Ctrl+Cによる安全な中断とチェックポイント保存  
> - シミュレーションの正確な再開  
> 詳細は [CHECKPOINT_COMPLETE_SUMMARY.md](CHECKPOINT_COMPLETE_SUMMARY.md) をご覧ください。
>
> **プロジェクトリファクタリング (2025-10-31):**  
> コードベースが階層的に整理され、Python解析ツールが `uv` を使用するように modernized されました。  
> 詳細は [REFACTORING_SUMMARY.md](REFACTORING_SUMMARY.md) をご覧ください。

## プロジェクト構造

### C++コード
```
src/
├── core/            # シミュレーションフレームワーク (solver, simulation, output)
├── modules/         # モジュールシステム (pre_interaction, fluid_force, etc.)
├── algorithms/      # SPHアルゴリズム実装
│   ├── ssph/       # Standard SPH
│   ├── disph/      # Density Independent SPH
│   ├── gsph/       # Godunov SPH
│   └── gdisph/     # Godunov-DISPH
├── tree/           # 近傍粒子探索 (Barnes-Hut tree)
├── utilities/      # ヘルパーコード
└── sample/         # サンプルシミュレーション
```

### Python解析ツール
```
analysis/
├── readers.py       # データ読み込み
├── conservation.py  # 保存量チェック
├── plotting.py      # 可視化
├── theoretical.py   # 理論解
└── cli/            # コマンドラインツール
```

詳細は [ARCHITECTURE.md](ARCHITECTURE.md) と [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) をご覧ください。

## コンパイル
次元を `include/utilities/defines.hpp` の `DIM` に設定してからコンパイルします。

### CMake (推奨)
MacOS/Linuxの場合、次のコマンドでビルドします：

```bash
# Nixを使う場合（推奨）
nix develop        # 開発環境をセットアップ
rm -rf build && mkdir build && cd build
cmake ..
make -j8
./sph shock_tube ../sample/shock_tube/shock_tube.json 1
```

MacOS (Homebrewを使う場合):
```bash
brew install llvm libomp boost
brew --prefix llvm  
brew --prefix libomp  
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"

# Build all dimensions (1D, 2D, 3D)
./scripts/build_all_dimensions.sh build

# Or build single dimension
cmake -B build -DBUILD_DIM=2  # 1, 2, or 3
cmake --build build -j8

# Run simulations
./build/sph1d shock_tube  # 1D
./build/sph2d khi         # 2D
./build/sph3d evrard      # 3D
```

**Note**: GSPHCODE now supports building for all three spatial dimensions (1D, 2D, 3D) simultaneously. See `DIMENSION_BUILD_SYSTEM.md` for details.

Windows (Visual Studio):
```bash
# Visual Studio 2017以降
# 環境変数を設定: BOOST_INC_PATH=C:\boost\boost_1_67_0\include\boost-1_67
cmake -B build -DBUILD_DIM=2 -G "Visual Studio 15 2017 Win64"
# Visual StudioでSPHCODE.slnを開いてビルド
```

## Python解析ツール

### セットアップ
Pythonツールは `uv` パッケージマネージャを使用します：

```bash
# uvのインストール（初回のみ）
curl -LsSf https://astral.sh/uv/install.sh | sh

# 依存パッケージのインストール
uv sync

# 解析例
cd sample/shock_tube
../../build/sph shock_tube shock_tube.json 1

# クイック解析
uv run python -m analysis.cli.analyze quick shock_tube

# アニメーション作成
uv run python -m analysis.cli.animate shock_tube
```

### Jupyter Notebook
```bash
uv run jupyter lab
# analysis/example_analysis.ipynb を開く
```

### 手動解析（Pythonスクリプト）
```python
import sys
sys.path.append('../../')  # analysisモジュールへのパス

from analysis import readers, plotting, conservation

# データ読み込み
data = readers.read_all_csv('results/DISPH/shock_tube/1D/')

# プロット
plotting.plot_shock_tube(data, theoretical_solution=True)

# 保存量チェック
conservation.check_energy_conservation(data)
```

## 実行
### サンプル実行（プリセット）
`sample/` ディレクトリにあるプリセット設定を使用：
```bash
cd build
./sph shock_tube ../sample/shock_tube/shock_tube.json 1
```

#### サンプル一覧

|サンプル名|DIM|説明|
|:---|:---|:---|
|shock_tube|1|衝撃波管問題 (e.g. Hernquist & Katz 1989)|
|shock_tube_2d|2|2次元衝撃波管問題|
|sedov_taylor|2|Sedov-Taylor爆発|
|evrard|3|Evrard collapse (自己重力テスト)|
|gresho_chan_vortex|2|Gresho-Chan vortex (圧力平衡)|
|hydrostatic|2|静水圧平衡 (Saitoh & Makino 2013)|
|khi|2|Kelvin-Helmholtz 不安定性|
|lane_emden|3|Lane-Emden球 (自己重力)|
|pairing_instability|2|粒子ペアリング不安定性テスト|
|vacuum_test|2|真空境界テスト|

その他15+サンプルが利用可能 (`ls sample/` で確認)。

#### \<threads\>
OpenMPのスレッド数を指定します。省略した場合は使用可能な最大スレッド数 (`omp_get_max_threads()` の戻り値)となります。

### カスタム設定で実行
独自のJSON設定ファイルを作成して実行：
```bash
cd build
./sph <parameter_file> <threads>
```

設定ファイルのテンプレートは `configs/base/` にあります。

### チェックポイント/再開機能 (NEW!)
長時間シミュレーションを一時停止・再開できます：

#### 自動チェックポイント
```json
{
  "simulation": "shock_tube",
  "endTime": 100.0,
  "enableCheckpointing": true,
  "checkpointInterval": 10.0,
  "checkpointMaxKeep": 3,
  "checkpointOnInterrupt": true
}
```

#### Ctrl+Cで安全に中断
シミュレーション実行中に Ctrl+C を押すと自動的にチェックポイントが保存されます：
```
*** Interrupt signal received (Ctrl+C) ***
Saving checkpoint at t=47.3 to output/run_xyz/checkpoints/checkpoint_t47.300000.chk
Resume with: "resumeFromCheckpoint": true, "resumeCheckpointFile": "output/run_xyz/checkpoints/checkpoint_t47.300000.chk"
```

#### チェックポイントから再開
```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "output/run_xyz/checkpoints/checkpoint_t47.300000.chk",
  "enableCheckpointing": true
}
```

詳細は [CHECKPOINT_COMPLETE_SUMMARY.md](CHECKPOINT_COMPLETE_SUMMARY.md) を参照してください。

## 開発とコントリビューション

### プロジェクト構造の理解
- **ARCHITECTURE.md** - コードベースのアーキテクチャ解説
- **DEVELOPER_GUIDE.md** - 開発者向けガイド（モジュール追加方法など）
- **REFACTORING_SUMMARY.md** - リファクタリングの詳細記録
- **QUICK_REFERENCE.md** - クイックリファレンス

### 開発環境のセットアップ
```bash
# Nix環境（推奨）
nix develop

# または手動セットアップ
brew install llvm libomp boost cmake
# または
apt-get install clang libomp-dev libboost-all-dev cmake
```

### テストの実行
```bash
# 全サンプルの実行確認
cd build
for sample in ../sample/*/; do
    name=$(basename "$sample")
    if [ -f "../sample/$name/$name.json" ]; then
        echo "Testing $name..."
        ./sph "$name" "../sample/$name/$name.json" 1
    fi
done

# Python解析テスト
uv run pytest analysis/  # (テスト実装予定)
```

### コントリビューション
新しいサンプルやSPH手法の追加については `DEVELOPER_GUIDE.md` をご覧ください。

## 計算例
### 衝撃波管問題
Godunov SPH法を使用

* 密度の時間発展

![shocktube_gsph.gif](https://raw.githubusercontent.com/mitchiinaga/sphcode/master/images/shocktube_gsph.gif)

* 厳密解との比較

![shocktube_dens.gif](https://raw.githubusercontent.com/mitchiinaga/sphcode/master/images/shocktube_dens.gif)

### Kelvin-Helmholtz不安定性
* Standard SPH

![khi_st.gif](https://raw.githubusercontent.com/mitchiinaga/sphcode/master/images/khi_st.gif)

* Density Independent SPH

![khi_di.gif](https://raw.githubusercontent.com/mitchiinaga/sphcode/master/images/khi_di.gif)

### Evrard collapse
Density Independent SPH法 + Balsara switch + 時間依存人工粘性

![evrard_di.gif](https://raw.githubusercontent.com/mitchiinaga/sphcode/master/images/evrard_di.gif)

## 実装
### SPH方程式
#### Standard SPH (density-energy formulation; Springel & Hernquist 2002, Monaghan 2002)
最小作用の原理から導出したSPH法の方程式です。smoothing length の空間微分 (grad-h term)を考慮した方程式系になっています。

#### Density Independent SPH (pressure-energy formulation; Saitoh & Makino 2013; Hopkins 2013)
状態方程式からSPH粒子の体積を求めることによって、密度に陽に依存しない方程式にします。接触不連続面を正しく扱えるようになります。

#### Godunov SPH (Inutsuka 2002; Cha & Whitworth 2003; Murante et al. 2011)
Riemann solverを使って粒子間相互作用を計算することで、計算を安定化させる数値拡散が自動的に入るようになります。

### カーネル関数
#### Cubic spline (e.g. Monaghan 1992)
昔からよく使われているオーソドックスなカーネル関数です。

#### Wendland C4 (Wendland 1995)
Cubic splineカーネルより高次のカーネル関数です。影響半径内の粒子数が大きいときに粒子同士がくっついてしまう不安定性 (pairing instability; Dehnen & Aly 2012) を防ぐことができます。

### 人工粘性
#### signal velocity formulation (Monaghan 1997)
Riemann solverによる数値拡散から類推して導出された人工粘性です。

#### Balsara switch (Balsara 1995)
速度の回転が発散より大きい領域で人工粘性係数を小さくすることで、シアー領域に余分な粘性が効かないようにします。

#### 時間依存人工粘性係数 (Rosswog et al. 2000)
SPH粒子それぞれの人工粘性係数を時間変化させます。圧縮領域では人工粘性係数を大きく、それ以外では小さくするようにします。

### その他
#### 人工熱伝導 (Price 2008; Wadsley et al. 2008)
エネルギー方程式に人工的な熱伝導項を入れることで、接触不連続面での非物理的な圧力ジャンプを抑制できます。

#### 自己重力 (Hernquist & Katz 1989)
カーネル関数とsmoothing lengthによって重力ソフトニングを行います。

#### Tree (Hernquist & Katz 1989)
近傍粒子探索に木構造を使うことで、計算量のオーダーをO(N^2)からO(N logN)に減らすことができます。

## パラメータ
デフォルト値が空欄のパラメータは、必ず指定する必要があります。

|パラメータ|型|説明|デフォルト値|
|:---|:---|:---|:---|
|outputDirectory|string|結果ファイルの出力先ディレクトリ||
|startTime|real|計算開始時刻|0|
|endTime|real|計算終了時刻||
|outputTime|real|粒子データの出力間隔を時間で指定|(endTime - startTime) / 100|
|energyTime|real|エネルギーの出力間隔を時間で指定|outputTime|
|SPHType|string|SPH方程式の指定。"ssph", "disph", "gsph"のいずれか|"ssph"|
|cflSound|real|音速による時間刻み制限を決めるパラメータ|0.3|
|cflForce|real|粒子に働く力による時間刻み制限を決めるパラメータ|0.125|
|avAlpha|real|初期人工粘性係数|1|
|useBalsaraSwitch|bool|Balsara switchを有効にする|true|
|useTimeDependentAV|bool|時間依存人工粘性を有効にする|false|
|alphaMax|real|時間依存人工粘性の係数の上限値|2.0|
|alphaMin|real|時間依存人工粘性の係数の下限値|0.1|
|epsilonAV|real|時間依存人工粘性の係数の減衰タイムスケールを決めるパラメータ|0.2|
|useArtificialConductivity|bool|人工熱伝導を使用する|false|
|alphaAC|real|人工熱伝導係数|1.0|
|maxTreeLevel|int|ツリーの最大レベル|20|
|leafParticleNumber|int|ツリーの葉ノードに入る粒子数の最大値|1|
|neighborNumber|int|近傍粒子数|32|
|gamma|real|比熱比||
|kernel|string|カーネル関数。"cubic_spline"または"wendland"|"cubic_spline"|
|iterativeSmoothingLength|bool|smoothing lengthをNewton法で求める|true|
|periodic|bool|周期境界を使用する|false|
|rangeMax|real array|周期境界使用時の座標の上限||
|rangeMin|real array|周期境界使用時の座標の下限||
|useGravity|bool|重力計算を有効にする|false|
|G|real|重力定数|1|
|theta|real|ツリー法で使用する見込み角|0.5|
|use2ndOrderGSPH|bool|Godunov SPH法でMUSCL補間を使用する|true|

## 参考文献
* Balsara, D. S. (1995). von Neumann stability analysis of smoothed particle hydrodynamics--suggestions for optimal algorithms. Journal of Computational Physics, 121(2), 357–372. https://doi.org/10.1016/S0021-9991(95)90221-X
* Cha, S. H., & Whitworth, A. P. (2003). Implementations and tests of Godunov-type particle hydrodynamics. Monthly Notices of the Royal Astronomical Society, 340(1), 73–90. https://doi.org/10.1046/j.1365-8711.2003.06266.x
* Dehnen, W., & Aly, H. (2012). Improving convergence in smoothed particle hydrodynamics simulations without pairing instability. Monthly Notices of the Royal Astronomical Society, 425(2), 1068–1082. https://doi.org/10.1111/j.1365-2966.2012.21439.x
* Evrard, A. E. (1988). Beyond N-body: 3D cosmological gas dynamics. Monthly Notices of the Royal Astronomical Society, 235(3), 911–934. https://doi.org/10.1093/mnras/235.3.911
* Gresho, P. M., & Chan, S. T. (1990). On the theory of semi-implicit projection methods for viscous incompressible flow and its implementation via a finite element method that also introduces a nearly consistent mass matrix. Part 2: Implementation. International Journal for Numerical Methods in Fluids, 11(5), 621–659. https://doi.org/10.1002/fld.1650110510
* Hernquist, L., & Katz, N. (1989). TREESPH - A unification of SPH with the hierarchical tree method. The Astrophysical Journal Supplement Series, 70, 419. https://doi.org/10.1086/191344
* Hopkins, P. F. (2013). A general class of Lagrangian smoothed particle hydrodynamics methods and implications for fluid mixing problems. Monthly Notices of the Royal Astronomical Society, 428(4), 2840–2856. https://doi.org/10.1093/mnras/sts210
* Inutsuka, S. (2002). Reformulation of Smoothed Particle Hydrodynamics with Riemann Solver. Journal of Computational Physics, 179, 238. https://doi.org/10.1006/jcph.2002.7053
* Murante, G., Borgani, S., Brunino, R., & Cha, S.-H. (2011). Hydrodynamic simulations with the Godunov smoothed particle hydrodynamics. Monthly Notices of the Royal Astronomical Society, 417(1), 136–153. https://doi.org/10.1111/j.1365-2966.2011.19021.x
* Monaghan, J. J. (1992). Smoothed Particle Hydrodynamics. Annual Review of Astronomy and Astrophysics, 30(1), 543–574. https://doi.org/10.1146/annurev.aa.30.090192.002551
* Monaghan, J. J. (1997). SPH and Riemann Solvers. Journal of Computational Physics, 136(2), 298–307. https://doi.org/10.1006/jcph.1997.5732
* Monaghan, J. J. (2002). SPH compressible turbulence. Monthly Notices of the Royal Astronomical Society, 335(3), 843–852. https://doi.org/10.1046/j.1365-8711.2002.05678.x
* Price, D. J. (2008). Modelling discontinuities and Kelvin–Helmholtz instabilities in SPH. Journal of Computational Physics, 227(24), 10040–10057. https://doi.org/10.1016/j.jcp.2008.08.011
* Rosswog, S., Davies, M. B., Thielemann, F.-K., & Piran, T. (2000). Merging neutron stars: asymmetric systems. Astronomy and Astrophysics, 360, 171–184. Retrieved from http://adsabs.harvard.edu/abs/2000A&A...360..171R%5Cnpapers2://publication/uuid/39C9D6F4-C091-499D-8F66-867A98C4DD32
* Saitoh, T. R., & Makino, J. (2013). A DENSITY-INDEPENDENT FORMULATION OF SMOOTHED PARTICLE HYDRODYNAMICS. The Astrophysical Journal, 768(1), 44. https://doi.org/10.1088/0004-637X/768/1/44
* Springel, V. (2010). E pur si muove: Galilean-invariant cosmological hydrodynamical simulations on a moving mesh. Monthly Notices of the Royal Astronomical Society, 401(2), 791–851. https://doi.org/10.1111/j.1365-2966.2009.15715.x
* Springel, V., & Hernquist, L. (2002). Cosmological smoothed particle hydrodynamics simulations: the entropy equation. Monthly Notices of the Royal Astronomical Society, 333(3), 649–664. https://doi.org/10.1046/j.1365-8711.2002.05445.x
* Wadsley, J. W., Veeravalli, G., & Couchman, H. M. P. (2008). On the treatment of entropy mixing in numerical cosmology. Monthly Notices of the Royal Astronomical Society, 387(1), 427–438. https://doi.org/10.1111/j.1365-2966.2008.13260.x
* Wendland, H. (1995). Piecewise polynomial, positive definite and compactly supported radial functions of minimal degree. Advances in Computational Mathematics, 4(1), 389–396. https://doi.org/10.1007/BF02123482


