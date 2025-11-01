# SPH Workflow Directory Structure Template

## 📁 Standard Structure

Apply this structure to ALL workflow steps for consistency:

```
workflow_name/
├── README.md                      # Workflow overview & quick start
├── CMakeLists.txt                 # Build configuration (if multi-step)
├── Makefile                       # Build targets (optional)
├── .gitignore                     # Ignore build/output directories
│
├── workflow_logs/                 # 📋 Workflow execution logs
│   ├── run_YYYYMMDD_HHMMSS.log   # Timestamped execution logs
│   └── performance_metrics.txt   # Timing and diagnostics
│
├── shared_data/                   # 🔄 Data shared between steps
│   ├── initial_conditions.csv    # ICs used by multiple steps
│   ├── relaxed_state.csv         # Output from step N → input to step N+1
│   └── reference_solutions/      # Analytical solutions, benchmarks
│
├── workflow_results/              # 📊 Workflow-level results
│   ├── combined_animation.mp4    # Multi-step visualizations
│   ├── comparison_plots/         # Cross-step analysis
│   └── workflow_summary.md       # Overall results report
│
├── 01_simulation/                 # Step 1 (or 01_relaxation, etc.)
│   ├── README.md                  # Step-specific documentation
│   ├── CMakeLists.txt             # Build configuration
│   ├── .gitignore                 # Ignore patterns  
│   ├── debug.json                # Debug configuration (optional)
│   └── variants/                 # Alternative configs (optional)
│       ├── high_res.json
│       └── low_mass.json
│
├── src/                          # 💻 Source code
│   ├── plugin.cpp                # Main plugin implementation
│   ├── physics/                  # Physics modules (optional)
│   ├── initialization/           # IC generators (optional)
│   └── *.hpp                     # Helper headers
│
├── scripts/                      # 🐍 Python/Shell scripts
│   ├── generate_initial_conditions.py
│   ├── animate_results.py
│   ├── analyze_data.py
│   ├── visualize.py
│   └── utils/                    # Shared utilities
│       ├── plotting.py
│       └── io.py
│
├── data/                         # 📊 Input data files
│   ├── tables/                   # Lookup tables
│   │   └── lane_emden.csv
│   ├── reference/                # Reference solutions
│   └── initial_conditions/       # Pre-generated ICs
│
├── docs/                         # 📝 Documentation
│   ├── physics.md               # Physics background
│   ├── implementation.md        # Code details
│   ├── troubleshooting.md       # Common issues
│   └── benchmarks/              # Benchmark results
│
├── output/                       # 🗂️ Simulation outputs (gitignored)
│   ├── .gitkeep
│   └── simulation_name/
│       └── run_YYYY-MM-DD_HHMMSS/
│           ├── outputs/
│           │   ├── csv/
│           │   └── binary/
│           ├── metadata.json
│           └── config.json
│
├── results/                      # 📈 Post-processed results
│   ├── animations/              # MP4 videos
│   ├── plots/                   # PNG/PDF figures
│   ├── analysis/                # Analysis outputs
│   └── reports/                 # Summary reports
│
└── build/                        # 🔧 Build artifacts (gitignored)
    ├── .gitkeep
    └── lib*.dylib
```

## 🎯 Key Principles

### 1. Separation of Concerns
- **`workflow_logs/`**: Workflow execution logs and diagnostics
- **`shared_data/`**: Data files used across multiple workflow steps
- **`workflow_results/`**: Cross-step visualizations and reports
- **`config/`**: Only JSON configuration files (step-level)
- **`src/`**: Only C++ source code (step-level)
- **`scripts/`**: Only Python/Shell scripts (step-level)
- **`data/`**: Only input data (step-level)
- **`docs/`**: Only documentation (step-level)
- **`output/`**: Only raw simulation outputs (gitignored, step-level)
- **`results/`**: Only post-processed results (step-level)

### 2. Naming Conventions
- **Config files**: `production.json`, `test.json`, `debug.json`
- **Main plugin**: Always `src/plugin.cpp` (consistency)
- **Scripts**: Descriptive names (`animate_results.py`, not `anim.py`)
- **Output dirs**: `output/` for raw, `results/` for processed

### 3. Git Hygiene
Always include `.gitignore`:
```gitignore
build/
output/
*.dylib
*.so
__pycache__/
.DS_Store
```

Keep `.gitkeep` in `output/` and `build/` so directories exist.

### 4. Documentation Requirements
Every workflow MUST have:
- **README.md**: Quick start, directory structure, usage examples
- **docs/**: Detailed implementation notes, physics background
- Clear comments in source code

## 🔄 Migration Script

Use this to reorganize an existing workflow:

```bash
#!/bin/bash
# migrate_workflow.sh

WORKFLOW_DIR="path/to/workflow"
cd "$WORKFLOW_DIR"

# Create directory structure
mkdir -p config src scripts data docs results/{animations,plots,analysis}
touch output/.gitkeep build/.gitkeep

# Move files
mv config.json config/production.json
mv config_test.json config/test.json
mv *.cpp src/plugin.cpp
mv *.hpp src/
mv *.py scripts/
mv *.csv data/
mv *.md docs/
mv *.png *.mp4 results/plots/ 2>/dev/null || true

# Update CMakeLists.txt to use src/plugin.cpp
sed -i '' 's/add_library(.*cpp/add_library(... src\/plugin.cpp/' CMakeLists.txt

# Update Makefile paths
sed -i '' 's/config\.json/config\/production.json/g' ../Makefile
sed -i '' 's/config_test\.json/config\/test.json/g' ../Makefile

echo "✓ Migration complete!"
```

## 📋 Template Files

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.23)
project(workflow_plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../" ABSOLUTE)
set(SPH_BUILD_DIR "${SPH_ROOT}/build")
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")

include_directories(${SPH_INCLUDE_DIR})
add_compile_definitions(DIM=3)

# Main plugin from src/
add_library(workflow_plugin SHARED src/plugin.cpp)
target_link_libraries(workflow_plugin PRIVATE ${SPH_BUILD_DIR}/libsph_lib.a)

set_target_properties(workflow_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
```

### .gitignore
```gitignore
# Build
build/
*.dylib
*.so
*.o
*.a
CMakeCache.txt
CMakeFiles/
cmake_install.cmake

# Output
output/
!output/.gitkeep

# Temp
*.swp
*~
.DS_Store
__pycache__/
*.pyc
```

### README.md Template
```markdown
# Workflow Name

Brief description of what this workflow does.

## Quick Start

\`\`\`bash
make build-workflow
make run-workflow-test
make visualize
\`\`\`

## Directory Structure

[Use structure from above]

## Configuration

- **production.json**: Full simulation (500s, high-res)
- **test.json**: Quick test (10s, low-res)

## Output

- Simulation data: `output/`
- Results: `results/plots/`, `results/animations/`

## Physics

[Brief description of physics/numerics]
```

## ✅ Checklist for New Workflows

- [ ] Create directory structure
- [ ] Write README.md with quick start
- [ ] Add .gitignore
- [ ] Place configs in `config/`
- [ ] Place source in `src/plugin.cpp`
- [ ] Place scripts in `scripts/`
- [ ] Update CMakeLists.txt for `src/plugin.cpp`
- [ ] Update Makefile for new config paths
- [ ] Add docs/ for implementation notes
- [ ] Test build and run

## 🔍 Benefits of This Structure

1. **Consistency**: Same structure across all workflows
2. **Clarity**: Easy to find files ("Where's the config?" → `config/`)
3. **Scalability**: Add more files without clutter
4. **Collaboration**: New developers know where things go
5. **Version Control**: Clean .gitignore, meaningful commits
6. **Automation**: Standard paths for Makefiles
7. **Documentation**: Self-organizing documentation structure

## 🎓 Examples

See implemented examples:
- `razor_thin_hvcc_gdisph_workflow/01_relaxation/` (✅ Migrated)
- `razor_thin_hvcc_gdisph_workflow/02_flyby/` (TODO)
- `razor_thin_hvcc_workflow/01_relaxation/` (TODO)
