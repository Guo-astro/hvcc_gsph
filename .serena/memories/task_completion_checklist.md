# Task Completion Checklist

When you complete a task involving code changes, verify:

## Build Verification
- [ ] Code compiles without errors: `cd build && make`
- [ ] No new compiler warnings introduced
- [ ] Dimension (`DIM`) is set appropriately in `include/defines.hpp`

## Code Quality
- [ ] Follows naming conventions (see code_style_conventions.md)
- [ ] Uses `m_` prefix for member variables
- [ ] Proper namespace usage (`sph` namespace)
- [ ] Memory management uses smart pointers
- [ ] OpenMP pragmas for parallelizable loops

## Registration (if adding new components)
- [ ] Sample uses `REGISTER_SAMPLE(name, function)` macro
- [ ] Module uses `REGISTER_MODULE(sph_type, module_type, class)` macro
- [ ] Registration happens in anonymous namespace (for samples)
- [ ] JSON config file created in appropriate directory

## File Organization
- [ ] Headers go in `include/` (or subdirectory)
- [ ] Implementation goes in `src/` (or subdirectory)
- [ ] Sample configs go in `sample/<name>/`
- [ ] Production configs go in `production_sims/<name>/`
- [ ] CMakeLists.txt automatically picks up new .cpp files (via globbing)

## Documentation
- [ ] Update README.md if adding major features
- [ ] Add JSON parameter documentation if new params introduced
- [ ] Update Makefile with new run target (optional but helpful)

## Testing
- [ ] Test compilation: `make build`
- [ ] Test execution: `./build/sph <sample_name> <config.json> 8`
- [ ] Verify output appears in `results/` directory
- [ ] Check log files for errors/warnings
- [ ] Run kernel tests if modifying core algorithms: `make run_kernel_test`

## Performance Considerations
- [ ] Critical loops use OpenMP parallelization
- [ ] No unnecessary memory allocations in hot paths
- [ ] Tree structure used for O(N log N) neighbor search
- [ ] Appropriate compiler optimizations enabled (`-ffast-math`, `-funroll-loops`)

## Version Control
- [ ] Stage changes: `git add <files>`
- [ ] Commit with descriptive message: `git commit -m "Description"`
- [ ] Build directory not committed (in .gitignore)
- [ ] JSON configs committed with code

## Platform-Specific
### macOS
- [ ] OpenMP paths point to Homebrew libomp
- [ ] Architecture set to arm64 in CMakeLists.txt
- [ ] Tested with LLVM/Clang compiler

### Linux
- [ ] GCC version >= 7.4.0
- [ ] OpenMP available via compiler
- [ ] Makefile builds work

## No Explicit Action Required
- **Linting**: No automated linter configured
- **Formatting**: No automated formatter (clang-format not configured)
- **Unit tests**: Limited test infrastructure (only kernel_test currently)
- **CI/CD**: No continuous integration setup
