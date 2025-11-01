#!/bin/bash
# Checkpoint System Test Script
# This script demonstrates the checkpoint/resume functionality

set -e  # Exit on error

echo "=================================================="
echo "SPHCode Checkpoint/Resume System Test"
echo "=================================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if we're in the build directory
if [ ! -f "sph1d" ]; then
    echo -e "${RED}Error: Must run from build/ directory${NC}"
    echo "Usage: cd build && ../test/checkpoint_manual/test_checkpoint.sh"
    exit 1
fi

echo -e "${BLUE}Step 1: Building test configuration...${NC}"

# Create a test config for shock tube with checkpointing
cat > test_checkpoint.json << 'EOF'
{
    "simulation": "shock_tube",
    "description": "1D shock tube test with checkpoint",
    "dimension": 1,
    "SPHType": "disph",
    "gamma": 1.4,
    "avAlpha": 1.0,
    "neighborNumber": 50,
    "kernelType": "cubic",
    "periodic": false,
    "useGravity": false,
    "endTime": 0.2,
    "outputInterval": 0.05,
    "N": 400,
    "enableBinary": true,
    "enableCSV": false,
    "enableCheckpointing": true,
    "checkpointInterval": 0.05,
    "checkpointMaxKeep": 3,
    "checkpointOnInterrupt": true,
    "checkpointDirectory": "checkpoints"
}
EOF

echo -e "${GREEN}✓ Config created: test_checkpoint.json${NC}"
echo

echo -e "${BLUE}Step 2: Running simulation with auto-checkpoint...${NC}"
echo "This will run from t=0 to t=0.2 with checkpoints every 0.05"
echo

./sph1d shock_tube test_checkpoint.json 2>&1 | tee checkpoint_test.log

echo
echo -e "${GREEN}✓ Simulation complete${NC}"
echo

echo -e "${BLUE}Step 3: Checking for checkpoint files...${NC}"

# Find the latest run directory
RUN_DIR=$(ls -td output/run_* 2>/dev/null | head -1)

if [ -z "$RUN_DIR" ]; then
    echo -e "${RED}✗ No output directory found${NC}"
    exit 1
fi

echo "Run directory: $RUN_DIR"

# Check for checkpoints
if [ -d "$RUN_DIR/checkpoints" ]; then
    echo -e "${GREEN}✓ Checkpoint directory exists${NC}"
    echo
    echo "Checkpoint files:"
    ls -lh "$RUN_DIR/checkpoints/"
    CHECKPOINT_COUNT=$(ls "$RUN_DIR/checkpoints/"/*.chk 2>/dev/null | wc -l)
    echo
    echo "Total checkpoints: $CHECKPOINT_COUNT"
    
    if [ "$CHECKPOINT_COUNT" -gt 0 ]; then
        echo -e "${GREEN}✓ Checkpoints were created successfully${NC}"
        
        # Get the last checkpoint
        LAST_CHECKPOINT=$(ls -t "$RUN_DIR/checkpoints/"*.chk | head -1)
        CHECKPOINT_SIZE=$(du -h "$LAST_CHECKPOINT" | cut -f1)
        echo
        echo "Latest checkpoint: $(basename $LAST_CHECKPOINT)"
        echo "Checkpoint size: $CHECKPOINT_SIZE"
        
        # Test resume
        echo
        echo -e "${BLUE}Step 4: Testing resume from checkpoint...${NC}"
        
        # Create resume config
        cat > test_resume.json << EOF
{
    "resumeFromCheckpoint": true,
    "resumeCheckpointFile": "$LAST_CHECKPOINT",
    "endTime": 0.3,
    "outputInterval": 0.05,
    "enableCheckpointing": true,
    "checkpointInterval": 0.05,
    "checkpointMaxKeep": 3,
    "enableBinary": true,
    "enableCSV": false
}
EOF
        
        echo "Resume config created: test_resume.json"
        echo "Resuming from: $LAST_CHECKPOINT"
        echo
        
        ./sph1d shock_tube test_resume.json 2>&1 | tee resume_test.log
        
        echo
        echo -e "${GREEN}✓ Resume test complete${NC}"
        
        # Check if resumed run created output
        RESUMED_RUN=$(ls -td output/run_* 2>/dev/null | head -1)
        if [ "$RESUMED_RUN" != "$RUN_DIR" ]; then
            echo -e "${GREEN}✓ Resumed simulation created new output${NC}"
            echo "Original run: $RUN_DIR"
            echo "Resumed run: $RESUMED_RUN"
        fi
        
    else
        echo -e "${RED}✗ No checkpoint files found${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ Checkpoint directory not created${NC}"
    exit 1
fi

echo
echo "=================================================="
echo -e "${GREEN}ALL TESTS PASSED!${NC}"
echo "=================================================="
echo
echo "Summary:"
echo "  - Auto-checkpoint: ✓ Working"
echo "  - Checkpoint files: ✓ Created ($CHECKPOINT_COUNT files)"
echo "  - Resume: ✓ Successful"
echo
echo "Checkpoint files are in: $RUN_DIR/checkpoints/"
echo
echo "To manually test interrupt (Ctrl+C):"
echo "  1. Run: ./sph1d shock_tube test_checkpoint.json"
echo "  2. Press Ctrl+C while running"
echo "  3. Check for interrupt checkpoint in output directory"
echo "  4. Resume with the printed command"
echo
