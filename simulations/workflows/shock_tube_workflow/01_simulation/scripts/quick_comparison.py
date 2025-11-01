#!/usr/bin/env python3
"""
Quick comparison plot of DISPH vs analytical solution.

Usage:
    python quick_disph_comparison.py [snapshot_file]
"""

import sys
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).parent))

import readers
import theoretical
import matplotlib.pyplot as plt


def plot_comparison(output_dir: str, snapshot_index: int = -1, gamma: float = 1.4):
    """Create comparison plot for a single snapshot."""
    
    # Read simulation
    reader = readers.SimulationReader(output_dir)
    
    if snapshot_index < 0:
        snapshot_index = reader.num_snapshots + snapshot_index  # Use -1 for last
    
    snap = reader.read_snapshot(snapshot_index)
    print(f"Loaded snapshot {snapshot_index}: t = {snap.time:.4f}, N = {snap.num_particles}")
    
    if snap.dim != 1:
        print("ERROR: Only 1D simulations supported")
        sys.exit(1)
    
    # Get analytical solution
    theory, error = theoretical.TheoreticalComparison.compare_shock_tube(snap, gamma=gamma)
    print(f"L2 density error: {error:.6e}")
    
    # Create plot
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    axes = axes.flatten()
    
    x = snap.pos[:, 0]
    
    quantities = [
        ('dens', snap.dens, theory.rho, r'$\rho$', 'Density'),
        ('vel', snap.vel[:, 0], theory.vel, r'$v$', 'Velocity'),
        ('pres', snap.pres, theory.pres, r'$P$', 'Pressure'),
        ('ene', snap.ene, theory.ene, r'$u$', 'Internal Energy'),
    ]
    
    for i, (name, sim_y, thy_y, ylabel, title) in enumerate(quantities):
        ax = axes[i]
        
        # Plot simulation
        ax.scatter(x, sim_y, alpha=0.6, s=30, label='DISPH', color='blue', zorder=2)
        
        # Plot analytical
        ax.plot(theory.x, thy_y, 'r-', linewidth=2.5, label='Analytical', alpha=0.8, zorder=1)
        
        ax.set_xlabel('Position', fontsize=11)
        ax.set_ylabel(ylabel, fontsize=12)
        ax.set_title(title, fontsize=12, fontweight='bold')
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.legend(loc='best', fontsize=10)
    
    fig.suptitle(f'DISPH vs Analytical Sod Shock Tube Solution\n' +
                 f't = {snap.time:.4f} s  |  L2 Density Error = {error:.6f}',
                 fontsize=14, fontweight='bold')
    
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    
    # Save to visualizations directory
    viz_dir = Path(__file__).parent.parent / 'visualizations' / 'shock_tube'
    viz_dir.mkdir(parents=True, exist_ok=True)
    
    output_file = viz_dir / f'snapshot_{snapshot_index:05d}_comparison.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_file}")
    
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        # Default to final snapshot
        output_dir = "../build/results/DISPH/shock_tube/1D"
        snapshot_index = -1  # Last snapshot
        print(f"Using default directory: {output_dir}")
    else:
        output_dir = sys.argv[1]
        snapshot_index = int(sys.argv[2]) if len(sys.argv) > 2 else -1
    
    if not Path(output_dir).exists():
        print(f"ERROR: Directory not found: {output_dir}")
        sys.exit(1)
    
    plot_comparison(output_dir, snapshot_index)
