#!/usr/bin/env python3
"""
Shock tube analysis with theoretical comparison.

Usage:
    python shock_tube_analysis.py <output_directory> [gamma]
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from analysis import (
    SimulationReader,
    TheoreticalComparison,
    ParticlePlotter
)


def analyze_shock_tube(output_dir: str, gamma: float = 1.4):
    """Analyze shock tube simulation and compare with theory."""
    
    print("=" * 70)
    print("SHOCK TUBE ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {output_dir}")
    print(f"Gamma: {gamma}\n")
    
    # Read data
    reader = SimulationReader(output_dir)
    if reader.dim != 1:
        print("ERROR: Shock tube analysis requires 1D simulation")
        sys.exit(1)
    
    snapshots = reader.read_all_snapshots()
    print(f"Number of snapshots: {len(snapshots)}\n")
    
    # Create comparison plots for multiple times
    n_plots = min(4, len(snapshots))
    indices = np.linspace(0, len(snapshots)-1, n_plots, dtype=int)
    
    fig, axes = plt.subplots(n_plots, 4, figsize=(20, 4*n_plots))
    if n_plots == 1:
        axes = axes.reshape(1, -1)
    
    plotter = ParticlePlotter()
    
    for i, idx in enumerate(indices):
        snap = snapshots[idx]
        print(f"Analyzing snapshot {idx} at t = {snap.time:.4f}")
        
        # Get theoretical solution
        solution, error = TheoreticalComparison.compare_shock_tube(snap, gamma=gamma)
        print(f"  L2 density error: {error:.6e}")
        
        # Plot comparison for each quantity
        quantities = ['dens', 'vel', 'pres', 'ene']
        for j, qty in enumerate(quantities):
            plotter.plot_1d(snap, qty, theory=solution, ax=axes[i, j])
    
    plt.tight_layout()
    plt.savefig('shock_tube_comparison.png', dpi=150)
    print(f"\nSaved: shock_tube_comparison.png")
    
    # Plot error evolution
    times = []
    errors = []
    for snap in snapshots:
        solution, error = TheoreticalComparison.compare_shock_tube(snap, gamma=gamma)
        times.append(snap.time)
        errors.append(error)
    
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(times, errors, 'b-', linewidth=2, marker='o')
    ax.set_xlabel('Time')
    ax.set_ylabel('L2 Density Error')
    ax.set_title('Error vs Analytical Solution')
    ax.grid(True, alpha=0.3)
    plt.savefig('shock_tube_error.png', dpi=150)
    print(f"Saved: shock_tube_error.png")
    
    print("\nAnalysis complete!")
    print("=" * 70)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python shock_tube_analysis.py <output_directory> [gamma]")
        print("\nExample:")
        print("  python shock_tube_analysis.py results/shock_tube 1.4")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    gamma = float(sys.argv[2]) if len(sys.argv) > 2 else 1.4
    
    analyze_shock_tube(output_dir, gamma)
