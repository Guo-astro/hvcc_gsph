#!/usr/bin/env python3
"""
Quick analysis example for GSPH simulations.

Usage:
    python quick_analysis.py <output_directory>
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

# Add analysis package to path
sys.path.insert(0, str(Path(__file__).parent))

from analysis import (
    SimulationReader,
    ConservationAnalyzer,
    ParticlePlotter,
    EnergyPlotter
)


def analyze_simulation(output_dir: str):
    """Run quick analysis of simulation."""
    
    print("=" * 70)
    print("GSPH SIMULATION ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {output_dir}\n")
    
    # Read data
    print("Reading simulation data...")
    reader = SimulationReader(output_dir)
    print(f"  Dimension: {reader.dim}D")
    print(f"  Number of snapshots: {reader.num_snapshots}")
    
    # Read all snapshots
    snapshots = reader.read_all_snapshots()
    print(f"  Time range: {snapshots[0].time:.4f} to {snapshots[-1].time:.4f}")
    print(f"  Number of particles: {snapshots[0].num_particles}\n")
    
    # Conservation analysis
    print("Analyzing conservation properties...")
    conservation = ConservationAnalyzer.analyze_snapshots(snapshots)
    conservation.print_summary()
    print()
    
    # Energy from file
    energy = reader.read_energy_history()
    if energy is not None:
        print("Checking energy conservation from energy.txt...")
        ConservationAnalyzer.check_energy_from_file(energy, tolerance=1e-3)
        print()
    
    # Create plots
    print("Creating plots...")
    
    # Plot initial and final states
    fig, axes = plt.subplots(1, 2, figsize=(15, 5))
    plotter = ParticlePlotter()
    
    if reader.dim == 1:
        plotter.plot_1d(snapshots[0], 'dens', ax=axes[0])
        plotter.plot_1d(snapshots[-1], 'dens', ax=axes[1])
    elif reader.dim == 2:
        plotter.plot_2d_scatter(snapshots[0], 'dens', ax=axes[0])
        plotter.plot_2d_scatter(snapshots[-1], 'dens', ax=axes[1])
    else:  # 3D
        plotter.plot_3d_slice(snapshots[0], 'dens', ax=axes[0])
        plotter.plot_3d_slice(snapshots[-1], 'dens', ax=axes[1])
    
    plt.tight_layout()
    plt.savefig('density_comparison.png', dpi=150)
    print("  Saved: density_comparison.png")
    
    # Plot conservation
    if energy is not None:
        fig, axes = plt.subplots(2, 1, figsize=(10, 8))
        EnergyPlotter.plot_energy_history(energy, ax=axes[0])
        EnergyPlotter.plot_energy_error(energy, ax=axes[1])
        plt.tight_layout()
        plt.savefig('energy_conservation.png', dpi=150)
        print("  Saved: energy_conservation.png")
    
    # Plot full conservation report
    fig = EnergyPlotter.plot_conservation_report(conservation)
    plt.savefig('conservation_report.png', dpi=150)
    print("  Saved: conservation_report.png")
    
    print("\nAnalysis complete!")
    print("=" * 70)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python quick_analysis.py <output_directory>")
        print("\nExample:")
        print("  python quick_analysis.py results/shock_tube")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    analyze_simulation(output_dir)
