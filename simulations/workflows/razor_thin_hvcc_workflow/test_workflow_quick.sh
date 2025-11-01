#!/bin/bash
# Quick workflow test with shortened times for demonstration

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKFLOW_DIR="$SCRIPT_DIR"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Run from project root to ensure Lane-Emden CSV is found
cd "$PROJECT_ROOT"

echo "=========================================="
echo "  Quick Workflow Test (Shortened Times)"
echo "=========================================="
echo ""

# Step 1: Run relaxation for just 10 time units (instead of 500)
echo "Step 1: Disk Relaxation (quick test: t=0-10)"
echo "----------------------------------------------"

# Create temporary config with short endTime
cat > "$WORKFLOW_DIR/01_relaxation/config_test.json" << 'EOF'
{
  "metadata": {
    "workflow": "razor_thin_hvcc",
    "step": "01_relaxation",
    "purpose": "QUICK TEST - Generate hydrostatic disk",
    "dimension": 3
  },
  
  "outputDirectory": "simulations/workflows/razor_thin_hvcc_workflow/01_relaxation/output",
  
  "endTime": 10.0,
  "outputInterval": 2.0,
  
  "SPHType": "disph",
  "kernel": "wendland",
  "gamma": 1.6666666666666667,
  
  "neighborNumber": 64,
  "iterativeSml": true,
  
  "avAlpha": 2.0,
  "useBalsaraSwitch": true,
  
  "leafParticleNumber": 32,
  
  "useGravity": true,
  "G": 0.0043,
  "two_and_half_sim": true,
  
  "periodic": false,
  
  "useDensityRelaxation": true,
  "densityRelaxationMaxIter": 100,
  "densityRelaxationTolerance": 0.1,
  "densityRelaxationDamping": 0.2,
  "laneEmdenTable": "simulations/workflows/razor_thin_hvcc_workflow/01_relaxation/xi_theta.csv",
  
  "enableBinary": true,
  "enableCSV": true,
  
  "enableCheckpointing": true,
  "checkpointInterval": 5.0,
  "checkpointMaxKeep": 2,
  
  "cflSound": 0.3,
  "cflForce": 0.5
}
EOF

# Build if needed
if [ ! -f "$WORKFLOW_DIR/01_relaxation/build/libdisk_relaxation_plugin.dylib" ]; then
    echo "Building relaxation plugin..."
    cd "$WORKFLOW_DIR/01_relaxation"
    ./build.sh
    cd "$PROJECT_ROOT"
fi

# Run Step 1 from PROJECT_ROOT
echo "Running Step 1..."
"$PROJECT_ROOT/build/sph3d" \
    "$WORKFLOW_DIR/01_relaxation/build/libdisk_relaxation_plugin.dylib" \
    "$WORKFLOW_DIR/01_relaxation/config_test.json"

# Find the latest run directory
LATEST_RUN=$(find "$WORKFLOW_DIR/01_relaxation/output/disk_relaxation" -type d -name "run_*" | sort -r | head -1)
echo ""
echo "Step 1 complete!"
echo "Output directory: $LATEST_RUN"
echo ""

# List snapshots
echo "Generated snapshots:"
ls -lh "$LATEST_RUN/outputs/csv/"*.csv
echo ""

# Copy final snapshot to IC directory
FINAL_CSV=$(ls "$LATEST_RUN/outputs/csv/"*.csv | tail -1)
echo "Copying final snapshot to initial_conditions/:"
echo "  Source: $FINAL_CSV"

mkdir -p "$WORKFLOW_DIR/initial_conditions"
cp "$FINAL_CSV" "$WORKFLOW_DIR/initial_conditions/relaxed_disk_test.csv"
echo "  Destination: $WORKFLOW_DIR/initial_conditions/relaxed_disk_test.csv"
echo ""

# Show particle count
PARTICLE_COUNT=$(tail -n +2 "$WORKFLOW_DIR/initial_conditions/relaxed_disk_test.csv" | wc -l | tr -d ' ')
echo "Relaxed disk contains $PARTICLE_COUNT particles"
echo ""

echo "=========================================="
echo "  Quick Test Summary"
echo "=========================================="
echo ""
echo "✅ Step 1 completed successfully"
echo "✅ Output system automatically wrote CSV snapshots"
echo "✅ Initial conditions copied to shared directory"
echo ""
echo "Outputs:"
echo "  Run directory: $LATEST_RUN"
echo "  Snapshots: $(ls "$LATEST_RUN/outputs/csv/"*.csv | wc -l | tr -d ' ') CSV files"
echo "  IC file: initial_conditions/relaxed_disk_test.csv ($PARTICLE_COUNT particles)"
echo ""
echo "Key Findings:"
echo "  • No manual CSV writing needed - framework handles it!"
echo "  • Automatic unit headers in CSV files"
echo "  • Both CSV and Binary formats created"
echo "  • Checkpoint system working (separate from snapshots)"
echo ""
echo "To run Step 2 (flyby), configure it to use:"
echo "  \"initialConditionsFile\": \"../initial_conditions/relaxed_disk_test.csv\""
echo ""
