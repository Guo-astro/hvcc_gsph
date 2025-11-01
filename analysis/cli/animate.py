#!/usr/bin/env python3
"""
CLI tool for creating animations from GSPH simulation results.

Usage:
    gsph-animate <results_dir> [options]
"""

import argparse
import sys
from pathlib import Path


def main():
    """Main entry point for gsph-animate CLI."""
    parser = argparse.ArgumentParser(
        description="Create animations from GSPH simulation results",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    parser.add_argument(
        'results_dir',
        type=Path,
        help='Path to simulation results directory'
    )
    parser.add_argument(
        '-q', '--quantity',
        choices=['dens', 'pres', 'vel', 'ene'],
        default='dens',
        help='Quantity to visualize'
    )
    parser.add_argument(
        '-o', '--output',
        type=Path,
        default='animation.mp4',
        help='Output filename'
    )
    parser.add_argument(
        '--fps',
        type=int,
        default=10,
        help='Frames per second'
    )
    parser.add_argument(
        '--interval',
        type=int,
        default=1,
        help='Use every Nth snapshot'
    )
    parser.add_argument(
        '--mode',
        choices=['scatter', 'grid'],
        default='scatter',
        help='2D plotting mode (scatter: particles, grid: interpolated)'
    )
    parser.add_argument(
        '--cmap',
        default='viridis',
        help='Matplotlib colormap name'
    )
    
    args = parser.parse_args()
    
    # Import analysis modules
    try:
        # Import from analysis package (handles relative imports correctly)
        from analysis import SimulationReader, AnimationMaker
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
    except ImportError as e:
        print(f"Error importing analysis modules: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1
    
    print("=" * 70)
    print("CREATING ANIMATION")
    print("=" * 70)
    print(f"Output directory: {args.results_dir}")
    print(f"Quantity: {args.quantity}")
    print(f"Output file: {args.output}")
    print(f"FPS: {args.fps}")
    print(f"Snapshot interval: {args.interval}\n")
    
    try:
        # Read simulation
        reader = SimulationReader(str(args.results_dir))
        print(f"Dimension: {reader.dim}D")
        print(f"Number of snapshots: {reader.num_snapshots}\n")
        
        # Create animation
        maker = AnimationMaker(reader)
        
        print("Creating animation (this may take a while)...")
        if reader.dim == 1:
            anim = maker.animate_1d(
                quantity=args.quantity,
                output_file=str(args.output),
                fps=args.fps,
                interval=args.interval
            )
        elif reader.dim == 2:
            anim = maker.animate_2d(
                quantity=args.quantity,
                output_file=str(args.output),
                fps=args.fps,
                interval=args.interval,
                mode=args.mode
            )
        else:
            print("ERROR: 3D animation not yet implemented", file=sys.stderr)
            print("Use 2D slice projections instead", file=sys.stderr)
            return 1
        
        print("\nAnimation complete!")
        print("=" * 70)
        return 0
        
    except Exception as e:
        print(f"Error creating animation: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
