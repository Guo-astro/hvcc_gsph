#!/usr/bin/env python3
"""
Unified CLI for GSPH simulation analysis.

Usage:
    gsph-analyze quick <results_dir>           - Quick conservation analysis
    gsph-analyze shock-tube <results_dir>      - Shock tube theoretical comparison
    gsph-analyze energy <results_dir>          - Energy conservation analysis
    gsph-analyze conservation <results_dir>    - Detailed conservation report
"""

import argparse
import sys
from pathlib import Path


def main():
    """Main entry point for gsph-analyze CLI."""
    parser = argparse.ArgumentParser(
        description="GSPH Simulation Analysis Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Analysis command')
    
    # Quick analysis command
    quick_parser = subparsers.add_parser(
        'quick',
        help='Quick conservation and energy analysis'
    )
    quick_parser.add_argument(
        'results_dir',
        type=Path,
        help='Path to simulation results directory'
    )
    quick_parser.add_argument(
        '-o', '--output',
        type=Path,
        default=None,
        help='Output directory for plots (default: same as results_dir)'
    )
    
    # Shock tube analysis command
    shock_parser = subparsers.add_parser(
        'shock-tube',
        help='Compare 1D shock tube with analytical solution'
    )
    shock_parser.add_argument(
        'results_dir',
        type=Path,
        help='Path to simulation results directory'
    )
    shock_parser.add_argument(
        '--gamma',
        type=float,
        default=1.4,
        help='Adiabatic index (default: 1.4)'
    )
    shock_parser.add_argument(
        '-o', '--output',
        type=Path,
        default=None,
        help='Output directory for plots'
    )
    
    # Energy analysis command
    energy_parser = subparsers.add_parser(
        'energy',
        help='Analyze energy conservation from energy.txt'
    )
    energy_parser.add_argument(
        'results_dir',
        type=Path,
        help='Path to simulation results directory'
    )
    energy_parser.add_argument(
        '--tolerance',
        type=float,
        default=1e-3,
        help='Relative error tolerance (default: 1e-3)'
    )
    
    # Conservation analysis command
    conservation_parser = subparsers.add_parser(
        'conservation',
        help='Detailed conservation analysis (mass, momentum, energy, angular momentum)'
    )
    conservation_parser.add_argument(
        'results_dir',
        type=Path,
        help='Path to simulation results directory'
    )
    conservation_parser.add_argument(
        '--interval',
        type=int,
        default=1,
        help='Analyze every Nth snapshot (default: 1, all snapshots)'
    )
    
    args = parser.parse_args()
    
    if args.command is None:
        parser.print_help()
        return 1
    
    # Import analysis modules only when needed
    try:
        # Import from analysis package (handles relative imports correctly)
        import analysis
        from analysis import (
            SimulationReader,
            ConservationAnalyzer,
            EnergyPlotter,
            ParticlePlotter,
            TheoreticalComparison
        )
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError as e:
        print(f"Error importing analysis modules: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1
    
    # Execute command
    try:
        if args.command == 'quick':
            return quick_analysis(args, SimulationReader, ConservationAnalyzer, 
                                EnergyPlotter, ParticlePlotter, plt)
        elif args.command == 'shock-tube':
            return shock_tube_analysis(args, SimulationReader, TheoreticalComparison, 
                                      ParticlePlotter, plt, np)
        elif args.command == 'energy':
            return energy_analysis(args, SimulationReader, ConservationAnalyzer, 
                                 EnergyPlotter, plt)
        elif args.command == 'conservation':
            return conservation_analysis(args, SimulationReader, ConservationAnalyzer)
    except Exception as e:
        print(f"Error during analysis: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


def quick_analysis(args, SimulationReader, ConservationAnalyzer, EnergyPlotter, 
                   ParticlePlotter, plt):
    """Run quick analysis combining conservation and energy checks."""
    print("=" * 70)
    print("GSPH SIMULATION ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {args.results_dir}\n")
    
    # Read data
    print("Reading simulation data...")
    reader = SimulationReader(str(args.results_dir))
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
    output_dir = args.output if args.output else args.results_dir
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
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
    plot_path = output_dir / 'density_comparison.png'
    plt.savefig(plot_path, dpi=150)
    print(f"  Saved: {plot_path}")
    plt.close()
    
    # Plot conservation
    if energy is not None:
        fig, axes = plt.subplots(2, 1, figsize=(10, 8))
        EnergyPlotter.plot_energy_history(energy, ax=axes[0])
        EnergyPlotter.plot_energy_error(energy, ax=axes[1])
        plt.tight_layout()
        plot_path = output_dir / 'energy_conservation.png'
        plt.savefig(plot_path, dpi=150)
        print(f"  Saved: {plot_path}")
        plt.close()
    
    # Plot full conservation report
    fig = EnergyPlotter.plot_conservation_report(conservation)
    plot_path = output_dir / 'conservation_report.png'
    plt.savefig(plot_path, dpi=150)
    print(f"  Saved: {plot_path}")
    plt.close()
    
    print("\nAnalysis complete!")
    print("=" * 70)
    return 0


def shock_tube_analysis(args, SimulationReader, TheoreticalComparison, ParticlePlotter, plt, np):
    """Run shock tube analysis with theoretical comparison."""
    print("=" * 70)
    print("SHOCK TUBE ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {args.results_dir}")
    print(f"Gamma: {args.gamma}\n")
    
    # Read data
    reader = SimulationReader(str(args.results_dir))
    if reader.dim != 1:
        print("ERROR: Shock tube analysis requires 1D simulation", file=sys.stderr)
        return 1
    
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
        solution, error = TheoreticalComparison.compare_shock_tube(snap, gamma=args.gamma)
        print(f"  L2 density error: {error:.6e}")
        
        # Plot comparison for each quantity
        quantities = ['dens', 'vel', 'pres', 'ene']
        for j, qty in enumerate(quantities):
            plotter.plot_1d(snap, qty, theory=solution, ax=axes[i, j])
    
    output_dir = args.output if args.output else args.results_dir
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    plt.tight_layout()
    plot_path = output_dir / 'shock_tube_comparison.png'
    plt.savefig(plot_path, dpi=150)
    print(f"\nSaved: {plot_path}")
    plt.close()
    
    # Plot error evolution
    times = []
    errors = []
    for snap in snapshots:
        solution, error = TheoreticalComparison.compare_shock_tube(snap, gamma=args.gamma)
        times.append(snap.time)
        errors.append(error)
    
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(times, errors, 'b-', linewidth=2, marker='o')
    ax.set_xlabel('Time')
    ax.set_ylabel('L2 Density Error')
    ax.set_title('Error vs Analytical Solution')
    ax.grid(True, alpha=0.3)
    plot_path = output_dir / 'shock_tube_error.png'
    plt.savefig(plot_path, dpi=150)
    print(f"Saved: {plot_path}")
    plt.close()
    
    print("\nAnalysis complete!")
    print("=" * 70)
    return 0


def energy_analysis(args, SimulationReader, ConservationAnalyzer, EnergyPlotter, plt):
    """Run energy-focused analysis."""
    print("=" * 70)
    print("ENERGY CONSERVATION ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {args.results_dir}\n")
    
    reader = SimulationReader(str(args.results_dir))
    energy = reader.read_energy_history()
    
    if energy is None:
        print("ERROR: No energy.txt file found", file=sys.stderr)
        return 1
    
    print(f"Checking energy conservation (tolerance={args.tolerance})...")
    is_conserved = ConservationAnalyzer.check_energy_from_file(
        energy,
        tolerance=args.tolerance,
        verbose=True
    )
    print()
    
    # Plot
    fig, axes = plt.subplots(2, 1, figsize=(10, 8))
    EnergyPlotter.plot_energy_history(energy, ax=axes[0])
    EnergyPlotter.plot_energy_error(energy, ax=axes[1])
    plt.tight_layout()
    
    plot_path = Path(args.results_dir) / 'energy_analysis.png'
    plt.savefig(plot_path, dpi=150)
    print(f"Saved: {plot_path}")
    plt.close()
    
    print("=" * 70)
    return 0 if is_conserved else 1


def conservation_analysis(args, SimulationReader, ConservationAnalyzer):
    """Run detailed conservation analysis."""
    print("=" * 70)
    print("CONSERVATION ANALYSIS")
    print("=" * 70)
    print(f"Output directory: {args.results_dir}\n")
    
    reader = SimulationReader(str(args.results_dir))
    snapshots = reader.read_all_snapshots()
    
    # Optionally subsample
    if args.interval > 1:
        print(f"Analyzing every {args.interval} snapshots...")
        snapshots = snapshots[::args.interval]
    
    print(f"Analyzing {len(snapshots)} snapshots...\n")
    conservation = ConservationAnalyzer.analyze_snapshots(snapshots)
    conservation.print_summary()
    
    print("\n" + "=" * 70)
    return 0


if __name__ == '__main__':
    sys.exit(main())
