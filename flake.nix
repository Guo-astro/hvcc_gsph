{
  description = "GSPH - Godunov Smoothed Particle Hydrodynamics simulation code";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        # C++ Build Dependencies
        cppBuildInputs = with pkgs; [
          cmake
          boost
          llvmPackages_latest.openmp
          openssl  # Required for SHA-256 in checkpoint system
          gtest    # Google Test library for unit tests
        ];
        
        # C++ Native Build Tools
        cppNativeInputs = with pkgs; [
          cmake
          llvmPackages_latest.clang
          pkg-config
        ];
        
        # Python Version
        python = pkgs.python311;
        
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "gsphcode";
          version = "1.0.0";
          
          src = ./.;
          
          buildInputs = cppBuildInputs;
          nativeBuildInputs = cppNativeInputs ++ [ 
            pkgs.git 
            pkgs.cacert  # SSL certificates for git
          ];
          
          # Set SSL cert path for git
          NIX_SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";
          GIT_SSL_CAINFO = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";
          
          cmakeFlags = [
            "-DCMAKE_CXX_STANDARD=14"
            "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
            "-DBUILD_DIM=2"  # Default to 2D builds
          ] ++ pkgs.lib.optionals pkgs.stdenv.isDarwin [
            "-DCMAKE_OSX_ARCHITECTURES=arm64"
          ];
          
          configurePhase = ''
            export OpenMP_ROOT=${pkgs.llvmPackages_latest.openmp}
            export OpenMP_CXX_FLAGS="-fopenmp"
            export OpenMP_CXX_LIB_NAMES="omp"
            export OpenMP_omp_LIBRARY=${pkgs.llvmPackages_latest.openmp}/lib/libomp.dylib
            
            cmake -B build \
              -DCMAKE_CXX_STANDARD=14 \
              -DCMAKE_CXX_STANDARD_REQUIRED=ON \
              -DBUILD_DIM=2 \
              -DOpenMP_CXX_FLAGS="-fopenmp" \
              -DOpenMP_CXX_LIB_NAMES="omp" \
              -DOpenMP_omp_LIBRARY=${pkgs.llvmPackages_latest.openmp}/lib/libomp.dylib
          '';
          
          buildPhase = ''
            cd build
            make -j$NIX_BUILD_CORES
          '';
          
          installPhase = ''
            mkdir -p $out/bin
            cp sph2d $out/bin/
            
            # Also install sample configs for reference
            mkdir -p $out/share/gsphcode/samples
            cp -r ../sample/* $out/share/gsphcode/samples/ || true
          '';
          
          meta = with pkgs.lib; {
            description = "Godunov Smoothed Particle Hydrodynamics simulation code";
            license = licenses.mit;
            platforms = platforms.unix;
          };
        };
        
        devShells.default = pkgs.mkShell {
          buildInputs = cppBuildInputs ++ (with pkgs; [
            # Python and package management
            python
            uv  # Modern Python package manager (10-100x faster than pip)
            
            # Python development tools
            ruff  # Fast Python linter and formatter
            
            # Visualization and media
            ffmpeg  # For creating animations from simulation data
            
            # Documentation tools
            doxygen  # C++ documentation
            graphviz  # For diagrams
            
            # Utilities
            jq  # JSON processing for config files
          ]);
          
          nativeBuildInputs = cppNativeInputs ++ (with pkgs; [
            ninja  # Fast build system alternative to make
            
            # Debugging tools
            gdb  # GNU debugger
            lldb  # LLVM debugger
          ] ++ pkgs.lib.optionals (pkgs.stdenv.isLinux && !pkgs.stdenv.isDarwin) [
            # valgrind only on Linux (not available/broken on macOS and other platforms)
            # Commented out for now due to package being marked as broken
            # valgrind
          ]);
          
          shellHook = ''
            # Print banner
            echo "╔════════════════════════════════════════════════════════════╗"
            echo "║          GSPH Development Environment                     ║"
            echo "║  Godunov Smoothed Particle Hydrodynamics                  ║"
            echo "╚════════════════════════════════════════════════════════════╝"
            echo ""
            
            # C++ Environment Info
            echo "📦 C++ Build Environment:"
            echo "   CMake:  $(cmake --version | head -n1 | cut -d' ' -f3)"
            echo "   Clang:  $(clang --version | head -n1 | awk '{print $4}')"
            echo "   Boost:  ${pkgs.boost.version}"
            echo "   OpenMP: Available"
            echo ""
            
            # Python Environment Info
            echo "🐍 Python Analysis Environment:"
            echo "   Python: $(${python}/bin/python --version | cut -d' ' -f2)"
            echo "   uv:     $(uv --version 2>/dev/null | awk '{print $2}' || echo 'installed')"
            echo ""
            
            # Quick Start Commands
            echo "🚀 Quick Start Commands:"
            echo ""
            echo "  C++ Development:"
            echo "    Build:           mkdir -p build && cd build && cmake .. && make"
            echo "    Clean build:     rm -rf build && mkdir build && cd build && cmake .. && make"
            echo "    Run example:     ./build/sph shock_tube sample/shock_tube/shock_tube.json 8"
            echo ""
            echo "  Python Analysis (using uv):"
            echo "    Setup env:       uv sync"
            echo "    Add package:     uv add <package-name>"
            echo "    Run analysis:    uv run python analysis/quick_analysis.py results/shock_tube"
            echo "    Run CLI tool:    uv run gsph-analyze quick results/shock_tube"
            echo "    Activate env:    source .venv/bin/activate"
            echo ""
            echo "  Alternative (pip - deprecated):"
            echo "    pip install -r analysis/requirements.txt"
            echo "    python analysis/quick_analysis.py results/shock_tube"
            echo ""
            echo "📚 Documentation:"
            echo "   DOCUMENTATION.md    - Start here for navigation"
            echo "   DEVELOPER_GUIDE.md  - Complete development guide"
            echo "   QUICK_REFERENCE.md  - Quick recipes and cheat sheet"
            echo "   REFACTORING_PLAN.md - Architecture improvement plan"
            echo ""
            
            # Set OpenMP threads (use all available cores by default)
            export OMP_NUM_THREADS=''${OMP_NUM_THREADS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}
            echo "⚙️  OpenMP threads: $OMP_NUM_THREADS (override with OMP_NUM_THREADS=N)"
            echo ""
            
            # Add build directory to PATH if it exists
            if [ -d "build" ]; then
              export PATH="$PWD/build:$PATH"
              echo "✓ Added ./build to PATH"
            fi
            
            # Add current directory to PYTHONPATH for development
            export PYTHONPATH="$PWD:$PYTHONPATH"
            
            # Set up virtual environment location for uv
            export UV_PROJECT_ENVIRONMENT="$PWD/.venv"
            
            # Helpful aliases
            alias build-sph='mkdir -p build && cd build && cmake .. && make && cd ..'
            alias clean-build='rm -rf build && mkdir build && cd build && cmake .. && make && cd ..'
            alias run-shock='./build/sph shock_tube sample/shock_tube/shock_tube.json 8'
            
            echo "✓ Aliases: build-sph, clean-build, run-shock"
            echo ""
            echo "Ready to develop! 🎉"
            echo ""
          '';
        };
      }
    );
}
