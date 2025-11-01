# Quick Start: Creating Your First SPH Workflow

**Time to complete: ~5 minutes**

This guide walks you through creating, building, and running your first SPH simulation workflow from scratch.

---

## Step 1: Create the Workflow (30 seconds)

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows

# Create a 2D collision simulation workflow
./create_workflow.sh my_first_sim collision --dim=2
```

**Output:**
```
═══════════════════════════════════════════════════════════════
✅ Workflow created successfully!
═══════════════════════════════════════════════════════════════

Location: my_first_sim_workflow/
```

**What you got:**
- Complete folder structure (config/, src/, scripts/, etc.)
- Template C++ plugin with working code
- CMakeLists.txt configured for 2D
- Test & production JSON configs
- Makefile with common tasks
- Documentation templates

---

## Step 2: Explore the Generated Files (1 minute)

```bash
cd my_first_sim_workflow

# See the structure
ls -la 01_simulation/

# Check the auto-generated plugin
head -50 01_simulation/src/plugin.cpp
```

**Key files:**
```
01_simulation/
├── src/plugin.cpp         ← Your simulation code (customize this!)
├── config/
│   ├── test.json         ← Quick test (short runtime)
│   └── production.json   ← Full simulation
├── CMakeLists.txt        ← Build configuration
└── README.md             ← Step documentation
```

---

## Step 3: Build the Plugin (30 seconds)

```bash
# Build using the Makefile
make build
```

**Output:**
```
Building workflow...
[ 50%] Building CXX object CMakeFiles/my_first_sim_plugin.dir/src/plugin.cpp.o
[100%] Linking CXX shared library libmy_first_sim_plugin.dylib
[100%] Built target my_first_sim_plugin
```

**Verification:**
```bash
ls -lh 01_simulation/build/*.dylib
# -rwxr-xr-x  1 user  staff   45K  libmy_first_sim_plugin.dylib
```

---

## Step 4: Run a Test Simulation (1 minute)

```bash
# Run quick test
make run-test
```

**Output:**
```
Running test simulation...
Initializing My_first_sim simulation...
Created 2500 particles
  Particle mass:   4e-05
  Smoothing length: 0.04
Initialization complete!
... (simulation runs)
```

**Check results:**
```bash
ls -lh 01_simulation/output/my_first_sim_test/
# snapshot_0000.csv
# snapshot_0001.csv
# ...
```

---

## Step 5: Customize the Simulation (2 minutes)

Edit `01_simulation/src/plugin.cpp`:

### Change Resolution
```cpp
const int particles_per_dim = 100;   // was 50 → now 10,000 particles
```

### Add a Perturbation
```cpp
// After particle creation, add:
// TODO: Add any perturbations or special initial conditions here

// Add density perturbation
for (auto& pp : particles) {
    real r = std::sqrt(pp.pos[0]*pp.pos[0] + pp.pos[1]*pp.pos[1]);
    if (r < 0.3) {
        pp.dens *= 2.0;  // Double density in center
        pp.pres *= 2.0;
        pp.ene = pp.pres / ((gamma - 1.0) * pp.dens);
    }
}
```

### Rebuild and Test
```bash
make clean
make build
make run-test
```

---

## Step 6: Run Full Simulation (optional)

Once your test looks good:

```bash
make run-prod
```

This runs with production config (`config/production.json`) which typically has:
- Longer runtime
- More snapshots
- Higher resolution (if you edited configs)

---

## What's Next?

### 🔍 Analyze Results

```bash
cd ../../analysis

# Quick visualization
python quick_analysis.py \
  --input ../simulations/workflows/my_first_sim_workflow/01_simulation/output/my_first_sim_test/ \
  --output plots/
```

### 📝 Document Your Workflow

Edit the README files:
- `01_simulation/README.md` - Document what your simulation does
- `README.md` - Workflow overview

### 🚀 Create More Complex Workflows

```bash
# Multi-step workflow (e.g., relaxation + evolution)
./create_workflow.sh binary_merger merger --multi-step --dim=3

# 1D shock tube
./create_workflow.sh riemann_test shock --dim=1 --sph-type=disph

# 3D disk with gravity
./create_workflow.sh disk_sim disk --dim=3
```

---

## Common Workflow Pattern

```bash
# 1. Create
./create_workflow.sh <name> <type> [options]

# 2. Edit
vim <name>_workflow/01_simulation/src/plugin.cpp

# 3. Build
cd <name>_workflow && make build

# 4. Test
make run-test

# 5. Iterate (repeat 2-4 until working)

# 6. Production
make run-prod

# 7. Analyze
python ../../analysis/...
```

---

## Tips for Success

### ✅ Always Test First
```bash
make run-test  # Quick validation
# If successful →
make run-prod  # Full simulation
```

### ✅ Use Version Control
```bash
cd my_first_sim_workflow
git add .
git commit -m "Initial workflow setup"

# After customization
git commit -am "Added density perturbation"
```

### ✅ Read the Generated TODO Comments
The template is filled with helpful TODOs marking customization points:
```cpp
// TODO: Set domain size
// TODO: Set resolution
// TODO: Add perturbations
```

### ✅ Check Other Workflows for Examples
```bash
# See how disk relaxation works
cat razor_thin_hvcc_gdisph_workflow/01_relaxation/src/plugin.cpp

# See 1D shock tube
cat shock_tube_workflow/01_simulation/src/plugin.cpp
```

---

## Troubleshooting

### Build Fails
```bash
# Check CMake paths
cat 01_simulation/CMakeLists.txt

# Verify SPH library exists
ls ../../../build/libsph_lib.a
```

### Runtime Errors
```bash
# Check dimension matches
grep "DIM !=" 01_simulation/src/plugin.cpp
# Should match --dim flag used in creation

# Check config is valid JSON
python -m json.tool config/test.json
```

### No Output
```bash
# Check output directory was created
ls -la 01_simulation/output/

# Check simulation completed
echo $?  # Should be 0
```

---

## Complete Example Session

```bash
# Terminal session showing everything

$ cd /Users/guo/OSS/sphcode/simulations/workflows

$ ./create_workflow.sh demo custom --dim=2
✅ Workflow created successfully!

$ cd demo_workflow

$ make build
[100%] Built target demo_plugin

$ make run-test
Initializing Demo simulation...
Created 2500 particles
... simulation runs ...
Simulation complete!

$ ls -lh 01_simulation/output/demo_test/
snapshot_0000.csv  (2.5K particles, t=0.00)
snapshot_0001.csv  (t=0.05)
snapshot_0002.csv  (t=0.10)

$ # Success! Now customize...
```

---

## Ready to Start?

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows
./create_workflow.sh --help
```

**Choose your adventure:**
- Simple test: `./create_workflow.sh test custom`
- Real science: `./create_workflow.sh my_research <type> --dim=<N>`

**Time from idea to running simulation: ~5 minutes!** 🚀
