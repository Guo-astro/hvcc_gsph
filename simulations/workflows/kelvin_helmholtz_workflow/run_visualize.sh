#!/usr/bin/env bash
# Run visualization for this migrated workflow
BASE_DIR=$(cd "$(dirname "$0")" && pwd)
/usr/bin/env python3 "/Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow/create_visualizations.py" --root "$BASE_DIR"
echo "Visualizations generated in $BASE_DIR/visualizations"
