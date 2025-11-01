#!/usr/bin/env python3
"""
Create DISPH shock tube animation with analytical solution overlay.

Usage:
    python disph_shock_tube_animation.py [output_directory] [options]
"""

import sys
import argparse
from pathlib import Path
import numpy as np

# Add analysis directory to path
sys.path.insert(0, str(Path(__file__).parent))

# Import analysis tools
import readers
import theoretical
import plotting
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

# Use module references
SimulationReader = readers.SimulationReader
TheoreticalComparison = theoretical.TheoreticalComparison
ParticlePlotter = plotting.ParticlePlotter


def create_disph_animation(output_dir: str, 
                           gamma: float = 1.4,
                           fps: int = 10,
                           interval: int = 1):
    """
    Create animation comparing DISPH simulation with analytical Sod solution.
    
    Args:
        output_dir: Simulation output directory
        gamma: Adiabatic index
        fps: Frames per second
        interval: Use every Nth snapshot
    """
    # Setup output paths
    viz_dir = Path(__file__).parent.parent / 'visualizations' / 'shock_tube'
    viz_dir.mkdir(parents=True, exist_ok=True)
    output_file = viz_dir / 'disph_shock_tube_comparison.mp4'
    final_png = viz_dir / 'disph_shock_tube_comparison_final.png'
    
    print("=" * 80)
    print("DISPH SHOCK TUBE ANIMATION WITH ANALYTICAL OVERLAY")
    print("=" * 80)
    print(f"Output directory: {output_dir}")
    print(f"Gamma: {gamma}")
    print(f"Output file: {output_file}")
    print(f"FPS: {fps}")
    print(f"Snapshot interval: {interval}\n")
    
    # Read simulation data
    reader = SimulationReader(output_dir)
    if reader.dim != 1:
        print("ERROR: This script requires 1D simulation")
        sys.exit(1)
    
    snapshots = reader.read_all_snapshots()[::interval]
    print(f"Loaded {len(snapshots)} snapshots\n")
    
    # Create figure with 2x2 subplots
    fig, axes = plt.subplots(2, 2, figsize=(16, 10))
    axes = axes.flatten()
    
    plotter = ParticlePlotter()
    quantities = ['dens', 'vel', 'pres', 'ene']
    labels = ['Density', 'Velocity', 'Pressure', 'Internal Energy']
    
    # Determine axis limits from all snapshots
    limits = {}
    for qty in quantities:
        y_min_list = []
        y_max_list = []
        for snap in snapshots:
            # Get analytical solution
            theory, _ = TheoreticalComparison.compare_shock_tube(snap, gamma=gamma)
            
            # Get simulation values
            if qty == 'dens':
                sim_y = snap.dens
                thy_y = theory.rho
            elif qty == 'vel':
                sim_y = snap.vel[:, 0]
                thy_y = theory.vel
            elif qty == 'pres':
                sim_y = snap.pres
                thy_y = theory.pres
            elif qty == 'ene':
                sim_y = snap.ene
                thy_y = theory.ene
            
            y_min_list.append(min(sim_y.min(), thy_y.min()))
            y_max_list.append(max(sim_y.max(), thy_y.max()))
        
        y_min = min(y_min_list)
        y_max = max(y_max_list)
        y_range = y_max - y_min
        limits[qty] = (y_min - 0.05 * y_range, y_max + 0.05 * y_range)
    
    # Animation function
    def animate(frame):
        snap = snapshots[frame]
        
        # Get analytical solution
        theory, error = TheoreticalComparison.compare_shock_tube(snap, gamma=gamma)
        
        # Clear all axes
        for ax in axes:
            ax.clear()
        
        # Plot each quantity
        for i, (qty, label) in enumerate(zip(quantities, labels)):
            ax = axes[i]
            
            # Get data
            x = snap.pos[:, 0]
            if qty == 'dens':
                sim_y = snap.dens
                thy_y = theory.rho
                ylabel = r'$\rho$'
            elif qty == 'vel':
                sim_y = snap.vel[:, 0]
                thy_y = theory.vel
                ylabel = r'$v$'
            elif qty == 'pres':
                sim_y = snap.pres
                thy_y = theory.pres
                ylabel = r'$P$'
            elif qty == 'ene':
                sim_y = snap.ene
                thy_y = theory.ene
                ylabel = r'$u$'
            
            # Plot simulation (scatter)
            ax.scatter(x, sim_y, alpha=0.6, s=25, label='DISPH', color='blue', zorder=2)
            
            # Plot analytical (line)
            ax.plot(theory.x, thy_y, 'r-', linewidth=2.5, label='Analytical', alpha=0.8, zorder=1)
            
            # Configure axes
            ax.set_xlabel('Position', fontsize=11)
            ax.set_ylabel(ylabel, fontsize=12)
            ax.set_title(f'{label}', fontsize=12, fontweight='bold')
            ax.set_ylim(limits[qty])
            ax.grid(True, alpha=0.3, linestyle='--')
            ax.legend(loc='best', fontsize=10)
        
        # Overall title with time and error
        fig.suptitle(f'DISPH vs Analytical Sod Shock Tube Solution\n' + 
                     f't = {snap.time:.4f} s  |  L2 Density Error = {error:.6f}',
                     fontsize=14, fontweight='bold')
        
        plt.tight_layout(rect=[0, 0, 1, 0.96])
        
        return axes
    
    # Create animation
    print("Creating animation (this may take a while)...")
    anim = FuncAnimation(fig, animate, frames=len(snapshots), interval=1000//fps, blit=False)
    
    # Save animation
    print(f"Saving animation to {output_file}...")
    writer = FFMpegWriter(fps=fps, bitrate=2400, codec='libx264')
    anim.save(output_file, writer=writer, dpi=120)
    
    print(f"\n✓ Animation saved successfully!")
    print(f"  File: {output_file}")
    print(f"  Duration: {len(snapshots) / fps:.1f} seconds")
    print(f"  Resolution: 1920x1200")
    print("=" * 80)
    
    # Also create a static comparison plot at final time
    final_snap = snapshots[-1]
    final_theory, final_error = TheoreticalComparison.compare_shock_tube(final_snap, gamma=gamma)
    
    fig2, axes2 = plt.subplots(2, 2, figsize=(16, 10))
    axes2 = axes2.flatten()
    
    for i, (qty, label) in enumerate(zip(quantities, labels)):
        ax = axes2[i]
        x = final_snap.pos[:, 0]
        
        if qty == 'dens':
            sim_y = final_snap.dens
            thy_y = final_theory.rho
            ylabel = r'$\rho$'
        elif qty == 'vel':
            sim_y = final_snap.vel[:, 0]
            thy_y = final_theory.vel
            ylabel = r'$v$'
        elif qty == 'pres':
            sim_y = final_snap.pres
            thy_y = final_theory.pres
            ylabel = r'$P$'
        elif qty == 'ene':
            sim_y = final_snap.ene
            thy_y = final_theory.ene
            ylabel = r'$u$'
        
        ax.scatter(x, sim_y, alpha=0.6, s=30, label='DISPH', color='blue', zorder=2)
        ax.plot(final_theory.x, thy_y, 'r-', linewidth=2.5, label='Analytical', alpha=0.8, zorder=1)
        
        ax.set_xlabel('Position', fontsize=11)
        ax.set_ylabel(ylabel, fontsize=12)
        ax.set_title(f'{label}', fontsize=12, fontweight='bold')
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.legend(loc='best', fontsize=10)
    
    fig2.suptitle(f'DISPH vs Analytical Solution at Final Time\n' +
                  f't = {final_snap.time:.4f} s  |  L2 Density Error = {final_error:.6f}',
                  fontsize=14, fontweight='bold')
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    
    plt.savefig(str(final_png), dpi=150, bbox_inches='tight')
    print(f"✓ Final snapshot saved: {final_png}")
    
    return anim


def main():
    parser = argparse.ArgumentParser(
        description='Create DISPH shock tube animation with analytical overlay'
    )
    parser.add_argument(
        'output_dir',
        nargs='?',
        default='../build/results/DISPH/shock_tube/1D',
        help='Simulation output directory (default: ../build/results/DISPH/shock_tube/1D)'
    )
    parser.add_argument(
        '-g', '--gamma',
        type=float,
        default=1.4,
        help='Adiabatic index (default: 1.4)'
    )
    parser.add_argument(
        '--fps',
        type=int,
        default=10,
        help='Frames per second (default: 10)'
    )
    parser.add_argument(
        '--interval',
        type=int,
        default=1,
        help='Use every Nth snapshot (default: 1 - use all)'
    )
    
    args = parser.parse_args()
    
    # Check if output directory exists
    output_path = Path(args.output_dir)
    if not output_path.exists():
        print(f"ERROR: Output directory not found: {args.output_dir}")
        print("\nAvailable results directories:")
        results_base = Path('../build/results')
        if results_base.exists():
            for method_dir in results_base.iterdir():
                if method_dir.is_dir():
                    print(f"  {method_dir}")
                    for sim_dir in method_dir.iterdir():
                        if sim_dir.is_dir():
                            for dim_dir in sim_dir.iterdir():
                                if dim_dir.is_dir():
                                    print(f"    → {dim_dir}")
        sys.exit(1)
    
    create_disph_animation(
        args.output_dir,
        args.gamma,
        args.fps,
        args.interval
    )


if __name__ == "__main__":
    main()
