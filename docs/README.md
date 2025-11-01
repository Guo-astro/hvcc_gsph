# SPH Code Documentation

Centralized documentation for the SPH hydrodynamics code.

## Directory Structure

### 📐 Architecture (`architecture/`)
Core design and technical implementation details:
- **ARCHITECTURE.md** - Overall code architecture and design patterns
- **DIMENSION_HANDLING.md** - Multi-dimensional support (1D/2D/3D)
- **DIMENSION_BUILD_SYSTEM.md** - CMake build system for dimension handling
- **SIMULATION_RUN_STRUCTURE.md** - ⭐ New organized output structure for reproducibility

### 🎯 Benchmarks (`benchmarks/`)
Validation and testing framework:
- **BENCHMARK_VALIDATION.md** - Comprehensive benchmark validation framework
- **QUICK_START_VALIDATION.md** - Quick reference for running benchmarks

### 👨‍💻 Developer (`developer/`)
Information for code contributors:
- **DEVELOPER_GUIDE.md** - Developer onboarding and contribution guide
- **CONTRIBUTING.md** - Contribution guidelines
- **OUTPUT_FORMAT_IMPLEMENTATION.md** - ⭐ Output format system (CSV, Binary, NumPy)
- **DEBUG_DISPH.md** - DISPH formulation debugging notes
- **DEBUG_FORMULATION.md** - General formulation debugging
- **DISPH_SUCCESSFUL_IMPLEMENTATION.md** - DISPH implementation case study
- **REFACTORING_PLAN.md** - Refactoring roadmap
- **REFACTORING_CHECKLIST.md** - Refactoring task tracking
- **REFACTORING_SUMMARY.md** - Completed refactoring summary
- **LEGACY_CLEANUP_SUMMARY.md** - Legacy code cleanup notes

### 📖 User Guide (`user_guide/`)
End-user documentation:
- **QUICKSTART.md** - Getting started guide
- **QUICK_REFERENCE.md** - Quick command reference
- **DOCUMENTATION.md** - Comprehensive user documentation
- **OUTPUT_FORMATS.md** - ⭐ Output format guide (CSV, Binary, performance comparison)
- **OUTPUT_QUICK_REF.md** - ⭐ Output format quick reference
- **UNIT_SYSTEM.md** - Unit conversion system guide
- **UNIT_QUICK_REF.md** - Unit conversion quick reference
- **UNIT_SYSTEM_SUMMARY.md** - Unit system implementation summary

## Quick Links

**New Users**: Start with `user_guide/QUICKSTART.md`

**Output & Organization**: See `architecture/SIMULATION_RUN_STRUCTURE.md` for the new organized output system

**Output Formats**: See `user_guide/OUTPUT_FORMATS.md` for CSV vs Binary comparison

**Developers**: Read `developer/DEVELOPER_GUIDE.md` and `developer/CONTRIBUTING.md`

**Running Benchmarks**: See `benchmarks/QUICK_START_VALIDATION.md`

**Understanding Architecture**: Start with `architecture/ARCHITECTURE.md`

**Unit Conversions**: See `user_guide/UNIT_QUICK_REF.md` for quick examples

## What's New

### 🎯 Organized Simulation Runs (November 2025)
Complete refactor to self-contained simulation directories:
- All outputs, configs, and metadata in one place: `simulations/{sample}/{run_id}/`
- Automatic version tracking (git hash, parameters, performance)
- Multiple output formats (CSV, Binary)
- Python tools for finding and loading runs
- See `architecture/SIMULATION_RUN_STRUCTURE.md`

### ⚡ High-Performance Output Formats
- **Binary format**: 9x smaller, 20x faster than CSV
- **NumPy format**: Python-friendly compressed output (planned)
- See `user_guide/OUTPUT_FORMATS.md`

### 🔄 Unit Conversion System
- Full SI, CGS, and Galactic unit support
- Auto-detection from file headers
- See `user_guide/UNIT_SYSTEM.md`

## Documentation Status

✅ All documentation organized and categorized  
✅ New simulation run structure documented  
✅ Output format system fully documented  
✅ Unit conversion system fully documented  
✅ Benchmark validation framework documented  
✅ Architecture and design patterns documented
