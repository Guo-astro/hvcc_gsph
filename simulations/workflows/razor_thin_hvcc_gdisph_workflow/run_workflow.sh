#!/bin/bash
# Automated 2-Step Razor-Thin HVCC GDISPH Workflow
# Step 1: Generate relaxed disk using Lane-Emden profile + density relaxation
# Step 2: Run IMBH flyby simulation with relaxed disk as initial conditions (GDISPH)

set -e  # Exit on error

# Change to project root (3 levels up from workflow dir)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

WORKFLOW_DIR="$SCRIPT_DIR"
IC_DIR="$WORKFLOW_DIR/initial_conditions"
SPH3D="$PROJECT_ROOT/build/sph3d"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}  Razor-Thin HVCC GDISPH Workflow (2-Step)${NC}"
echo -e "${BLUE}============================================${NC}"
echo
echo -e "${GREEN}Step 1:${NC} Disk Relaxation (Lane-Emden + Hydrostatic Equilibrium)"
echo -e "${GREEN}Step 2:${NC} IMBH Flyby Simulation (GDISPH)"
echo
echo -e "Workflow directory: ${WORKFLOW_DIR}"
echo -e "Shared IC directory: ${IC_DIR}"
echo

# Parse command-line arguments
RUN_STEP1=true
RUN_STEP2=true
SPH_METHOD="gdisph"  # Default: GDISPH for this workflow
SKIP_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --step1-only)
            RUN_STEP2=false
            shift
            ;;
        --step2-only)
            RUN_STEP1=false
            shift
            ;;
        --gdisph)
            SPH_METHOD="gdisph"
            shift
            ;;
        --disph)
            SPH_METHOD="disph"
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo
            echo "Options:"
            echo "  --step1-only    Run only Step 1 (relaxation)"
            echo "  --step2-only    Run only Step 2 (flyby), requires existing IC"
            echo "  --disph         Use DISPH for Step 2 (default)"
            echo "  --gdisph        Use GDISPH for Step 2"
            echo "  --skip-build    Skip plugin rebuild if already built"
            echo "  --help          Show this help message"
            echo
            echo "Examples:"
            echo "  $0                    # Run full workflow with DISPH"
            echo "  $0 --gdisph           # Run full workflow with GDISPH"
            echo "  $0 --step1-only       # Only generate relaxed disk"
            echo "  $0 --step2-only       # Only run flyby (requires existing IC)"
            exit 0
            ;;
        *)
            echo -e "${RED}ERROR: Unknown option $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Verify SPH3D executable exists
if [ ! -f "$SPH3D" ]; then
    echo -e "${RED}ERROR: SPH3D executable not found at $SPH3D${NC}"
    echo "Please build the main project first:"
    echo "  cd /Users/guo/OSS/sphcode/build"
    echo "  cmake .. -DDIM=3"
    echo "  make"
    exit 1
fi

# ============================================
# STEP 1: Disk Relaxation
# ============================================
if [ "$RUN_STEP1" = true ]; then
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}  STEP 1: Disk Relaxation${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo
    
    # Build relaxation plugin (but stay in project root for running)
    if [ "$SKIP_BUILD" = false ] || [ ! -f "$WORKFLOW_DIR/01_relaxation/build/libdisk_relaxation_plugin.dylib" ]; then
        echo -e "${GREEN}Building relaxation plugin...${NC}"
        cd "$WORKFLOW_DIR/01_relaxation"
        ./build.sh
        cd "$PROJECT_ROOT"
        echo
    else
        echo -e "${BLUE}Skipping build (plugin already exists)${NC}"
        echo
    fi
    
    # Run relaxation simulation (from project root so paths work)
    echo -e "${GREEN}Running disk relaxation simulation...${NC}"
    echo -e "This may take several minutes depending on endTime and particle count"
    echo
    
    $SPH3D "$WORKFLOW_DIR/01_relaxation/build/libdisk_relaxation_plugin.dylib" "$WORKFLOW_DIR/01_relaxation/config.json"
    
    # Find latest output directory
    RELAX_OUTPUT=$(ls -td "$WORKFLOW_DIR/01_relaxation/output/disk_relaxation/run_"* 2>/dev/null | head -1)
    
    if [ -z "$RELAX_OUTPUT" ]; then
        echo -e "${RED}ERROR: No output directory found${NC}"
        exit 1
    fi
    
    # Find final CSV snapshot
    FINAL_CSV=$(ls -t "$RELAX_OUTPUT"/*.csv 2>/dev/null | head -1)
    
    if [ -z "$FINAL_CSV" ]; then
        echo -e "${RED}ERROR: No CSV output found in $RELAX_OUTPUT${NC}"
        exit 1
    fi
    
    # Copy to shared IC directory
    mkdir -p "$IC_DIR"
    cp "$FINAL_CSV" "$IC_DIR/relaxed_disk.csv"
    
    echo
    echo -e "${GREEN}✓ Step 1 Complete!${NC}"
    echo -e "Relaxed disk saved to: ${GREEN}$IC_DIR/relaxed_disk.csv${NC}"
    echo -e "Particle count: $(tail -n +2 "$IC_DIR/relaxed_disk.csv" | wc -l)"
    echo
fi

# ============================================
# STEP 2: IMBH Flyby
# ============================================
if [ "$RUN_STEP2" = true ]; then
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}  STEP 2: IMBH Flyby Simulation${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo
    
    # Check if initial conditions exist
    if [ ! -f "$IC_DIR/relaxed_disk.csv" ]; then
        echo -e "${RED}ERROR: Initial conditions not found at $IC_DIR/relaxed_disk.csv${NC}"
        echo "Please run Step 1 first or provide initial conditions manually"
        exit 1
    fi
    
    cd "$WORKFLOW_DIR/02_flyby"
    
    # Use GDISPH plugin from local 02_flyby directory
    PLUGIN_PATH="$WORKFLOW_DIR/02_flyby/build/librazor_thin_hvcc_gdisph_plugin.dylib"
    CONFIG_FILE="config_gdisph.json"
    echo -e "${BLUE}Using GDISPH method (better shock capturing)${NC}"
    
    # Build flyby plugin if needed
    if [ "$SKIP_BUILD" = false ] || [ ! -f "$PLUGIN_PATH" ]; then
        echo -e "${GREEN}Building flyby plugin...${NC}"
        cd "$WORKFLOW_DIR/02_flyby"
        ./build.sh
        echo
    else
        echo -e "${BLUE}Skipping build (plugin already exists)${NC}"
        echo
    fi
    
    # Run flyby simulation
    echo -e "${GREEN}Running IMBH flyby simulation...${NC}"
    echo -e "Initial conditions: $IC_DIR/relaxed_disk.csv"
    echo -e "SPH method: $SPH_METHOD"
    echo
    
    $SPH3D "$PLUGIN_PATH" "$CONFIG_FILE"
    
    echo
    echo -e "${GREEN}✓ Step 2 Complete!${NC}"
    
    # Find output directory
    OUTPUT_DIR=$(ls -td "$WORKFLOW_DIR/02_flyby/output/"run_* 2>/dev/null | head -1)
    
    if [ -n "$OUTPUT_DIR" ]; then
        echo -e "Results in: ${GREEN}$OUTPUT_DIR${NC}"
    fi
    echo
fi

# ============================================
# Workflow Complete
# ============================================
echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}  Workflow Complete!${NC}"
echo -e "${BLUE}============================================${NC}"
echo
echo -e "Summary:"
if [ "$RUN_STEP1" = true ]; then
    echo -e "  ${GREEN}✓${NC} Step 1: Relaxed disk generated"
    echo -e "      Location: $IC_DIR/relaxed_disk.csv"
fi
if [ "$RUN_STEP2" = true ]; then
    echo -e "  ${GREEN}✓${NC} Step 2: Flyby simulation completed ($SPH_METHOD)"
    if [ -n "$OUTPUT_DIR" ]; then
        echo -e "      Results: $OUTPUT_DIR"
    fi
fi
echo
echo -e "For analysis, see: simulations/workflows/razor_thin_hvcc_workflow/README.md"
echo
