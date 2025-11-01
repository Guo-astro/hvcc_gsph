#!/usr/bin/env python3
"""
Create animation from simulation output.

Usage:
    python make_animation.py <output_directory> [options]
"""

import sys
import argparse
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from analysis import SimulationReader, AnimationMaker


def main():
    parser = argparse.ArgumentParser(description='Create animation from SPH simulation')
    parser.add_argument('output_dir', help='Simulation output directory')
    parser.add_argument('-q', '--quantity', default='dens',
                       choices=['dens', 'pres', 'vel', 'ene'],
                       help='Quantity to visualize (default: dens)')
    parser.add_argument('-o', '--output', default='animation.mp4',
                       help='Output filename (default: animation.mp4)')
    parser.add_argument('--fps', type=int, default=10,
                       help='Frames per second (default: 10)')
    parser.add_argument('--interval', type=int, default=1,
                       help='Use every Nth snapshot (default: 1)')
    parser.add_argument('--mode', default='scatter', choices=['scatter', 'grid'],
                       help='2D plot mode (default: scatter)')
    
    args = parser.parse_args()
    
    print("=" * 70)
    print("CREATING ANIMATION")
    print("=" * 70)
    print(f"Output directory: {args.output_dir}")
    print(f"Quantity: {args.quantity}")
    print(f"Output file: {args.output}")
    print(f"FPS: {args.fps}")
    print(f"Snapshot interval: {args.interval}\n")
    
    # Read simulation
    reader = SimulationReader(args.output_dir)
    print(f"Dimension: {reader.dim}D")
    print(f"Number of snapshots: {reader.num_snapshots}\n")
    
    # Create animation
    maker = AnimationMaker(reader)
    
    print("Creating animation (this may take a while)...")
    if reader.dim == 1:
        anim = maker.animate_1d(
            quantity=args.quantity,
            output_file=args.output,
            fps=args.fps,
            interval=args.interval
        )
    elif reader.dim == 2:
        anim = maker.animate_2d(
            quantity=args.quantity,
            output_file=args.output,
            fps=args.fps,
            interval=args.interval,
            mode=args.mode
        )
    else:
        print("ERROR: 3D animation not yet implemented")
        print("Use plot_snapshots.py to create individual frames")
        sys.exit(1)
    
    print("\nAnimation complete!")
    print("=" * 70)


if __name__ == "__main__":
    main()
