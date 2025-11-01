#!/usr/bin/env python3
"""
Compare GDISPH and SSPH shock tube results with analytical solution.

This script compares the two SPH methods timestep-by-timestep against
the Sod shock tube analytical solution.
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import pandas as pd

# Add analysis directory to path
# From: .../simulations/workflows/shock_tube_workflow/01_simulation/scripts/compare_methods.py
# To:   .../analysis/
analysis_path = Path(__file__).resolve().parent.parent.parent.parent.parent.parent / 'analysis'
sys.path.insert(0, str(analysis_path))

# Import from analysis library (not as modules, but as package)
try:
    import readers
    import theoretical
    ParticleSnapshot = readers.ParticleSnapshot
    TheoreticalComparison = theoretical.TheoreticalComparison
except ImportError as e:
    print(f"Error importing analysis modules from {analysis_path}")
    print(f"Import error: {e}")
    print(f"Python path: {sys.path}")
    sys.exit(1)


def read_snapshot_csv(csv_file: Path, dim: int = 1):
    """
    Read a snapshot from CSV file with JSON metadata sidecar.
    
    New format (clean headers + metadata.json):
    - CSV has simple column names: time, pos_x, vel_x, dens, etc.
    - Metadata in *.meta.json file with units and descriptions
    """
    df = pd.read_csv(csv_file)
    
    # Read metadata file
    meta_file = csv_file.parent / (csv_file.stem + '.meta.json')
    metadata = None
    if meta_file.exists():
        try:
            import json
            with open(meta_file, 'r') as f:
                metadata = json.load(f)
        except Exception as e:
            print(f"Warning: Could not read metadata file {meta_file}: {e}")
    
    # Extract time from metadata
    time = 0.0
    if metadata and 'simulation' in metadata:
        time = metadata['simulation'].get('time', 0.0)
    elif 'time' in df.columns:
        # Fallback to CSV time column
        time = df['time'].iloc[0] if len(df) > 0 else 0.0
    
    # Build snapshot
    n = len(df)
    pos = np.zeros((n, 3))
    vel = np.zeros((n, 3))
    acc = np.zeros((n, 3))
    
    # Read position and velocity columns
    if dim >= 1 and 'pos_x' in df.columns and 'vel_x' in df.columns:
        pos[:, 0] = df['pos_x'].values
        vel[:, 0] = df['vel_x'].values
    if dim >= 2 and 'pos_y' in df.columns and 'vel_y' in df.columns:
        pos[:, 1] = df['pos_y'].values
        vel[:, 1] = df['vel_y'].values
    if dim >= 3 and 'pos_z' in df.columns and 'vel_z' in df.columns:
        pos[:, 2] = df['pos_z'].values
        vel[:, 2] = df['vel_z'].values
    
    # Read other fields
    dens = df['dens'].values if 'dens' in df.columns else np.ones(n)
    pres = df['pres'].values if 'pres' in df.columns else np.ones(n)
    ene = df['ene'].values if 'ene' in df.columns else np.ones(n)
    mass = df['mass'].values if 'mass' in df.columns else np.ones(n)
    sml = df['sml'].values if 'sml' in df.columns else np.ones(n)
    
    snapshot = ParticleSnapshot(
        time=time,
        num_particles=n,
        pos=pos,
        vel=vel,
        acc=acc,
        dens=dens,
        pres=pres,
        ene=ene,
        mass=mass,
        sml=sml,
        particle_id=np.arange(n)
    )
    
    return snapshot


def compare_shock_tube_methods(gdisph_dir: Path, ssph_dir: Path, 
                                output_dir: Path, gamma: float = 1.4):
    """
    Compare GDISPH and SSPH methods with analytical solution.
    
    Args:
        gdisph_dir: Directory containing GDISPH CSV outputs
        ssph_dir: Directory containing SSPH CSV outputs  
        output_dir: Directory to save comparison plots
        gamma: Adiabatic index
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Get list of snapshot files (sorted)
    gdisph_files = sorted(gdisph_dir.glob('*.csv'))
    ssph_files = sorted(ssph_dir.glob('*.csv'))
    
    print(f"GDISPH snapshots: {len(gdisph_files)}")
    print(f"SSPH snapshots: {len(ssph_files)}")
    
    # We'll compare at matching timesteps
    n_snapshots = min(len(gdisph_files), len(ssph_files))
    
    # Select snapshots for detailed comparison (beginning, middle, end + a few more)
    if n_snapshots <= 6:
        indices = list(range(n_snapshots))
    else:
        indices = [0, n_snapshots//4, n_snapshots//2, 3*n_snapshots//4, n_snapshots-1]
    
    print(f"\nComparing {len(indices)} timesteps: {indices}")
    
    # Create comparison plots
    n_plots = len(indices)
    fig, axes = plt.subplots(n_plots, 4, figsize=(20, 4*n_plots))
    if n_plots == 1:
        axes = axes.reshape(1, -1)
    
    quantities = ['dens', 'vel', 'pres', 'ene']
    qty_labels = ['Density', 'Velocity', 'Pressure', 'Internal Energy']
    
    gdisph_errors = []
    ssph_errors = []
    times = []
    
    for plot_idx, snap_idx in enumerate(indices):
        print(f"\n--- Snapshot {snap_idx} ---")
        
        # Read snapshots
        gdisph_snap = read_snapshot_csv(gdisph_files[snap_idx])
        ssph_snap = read_snapshot_csv(ssph_files[snap_idx])
        
        # Use GDISPH time as reference
        t = gdisph_snap.time
        times.append(t)
        print(f"Time: {t:.4f}")
        
        # Determine domain for analytical solution based on actual particle positions
        # Add some margin to ensure coverage
        x_min = min(gdisph_snap.pos[:, 0].min(), ssph_snap.pos[:, 0].min()) - 0.1
        x_max = max(gdisph_snap.pos[:, 0].max(), ssph_snap.pos[:, 0].max()) + 0.1
        x_theory = np.linspace(x_min, x_max, 1000)
        theory = TheoreticalComparison.sod_shock_tube(x_theory, t, gamma)
        
        # Compute errors by interpolating analytical solution to particle positions
        rho_theory_gdisph = np.interp(gdisph_snap.pos[:, 0], theory.x, theory.rho)
        gdisph_error = np.sqrt(np.mean((gdisph_snap.dens - rho_theory_gdisph)**2))
        gdisph_errors.append(gdisph_error)
        
        rho_theory_ssph = np.interp(ssph_snap.pos[:, 0], theory.x, theory.rho)
        ssph_error = np.sqrt(np.mean((ssph_snap.dens - rho_theory_ssph)**2))
        ssph_errors.append(ssph_error)
        
        print(f"  GDISPH L2 error: {gdisph_error:.6e}")
        print(f"  SSPH L2 error:   {ssph_error:.6e}")
        
        # Plot each quantity
        for qty_idx, qty in enumerate(quantities):
            ax = axes[plot_idx, qty_idx]
            
            # Get data
            if qty == 'vel':
                gdisph_data = gdisph_snap.vel[:, 0]
                ssph_data = ssph_snap.vel[:, 0]
                theory_data = theory.vel
            elif qty == 'dens':
                gdisph_data = gdisph_snap.dens
                ssph_data = ssph_snap.dens
                theory_data = theory.rho
            elif qty == 'pres':
                gdisph_data = gdisph_snap.pres
                ssph_data = ssph_snap.pres
                theory_data = theory.pres
            elif qty == 'ene':
                gdisph_data = gdisph_snap.ene
                ssph_data = ssph_snap.ene
                theory_data = theory.ene
            
            # Sort by position for cleaner plotting
            gdisph_sort = np.argsort(gdisph_snap.pos[:, 0])
            ssph_sort = np.argsort(ssph_snap.pos[:, 0])
            
            # Plot
            ax.plot(theory.x, theory_data, 'k-', linewidth=2, label='Analytical', alpha=0.8)
            ax.plot(gdisph_snap.pos[gdisph_sort, 0], gdisph_data[gdisph_sort], 
                   'b.-', markersize=3, linewidth=1, label='GDISPH', alpha=0.7)
            ax.plot(ssph_snap.pos[ssph_sort, 0], ssph_data[ssph_sort], 
                   'r.-', markersize=3, linewidth=1, label='SSPH', alpha=0.7)
            
            ax.set_xlabel('Position')
            ax.set_ylabel(qty_labels[qty_idx])
            ax.set_title(f'{qty_labels[qty_idx]} at t={t:.3f}')
            ax.grid(True, alpha=0.3)
            ax.legend(loc='best', fontsize=8)
    
    plt.tight_layout()
    comparison_file = output_dir / 'shock_tube_comparison_timesteps.png'
    plt.savefig(comparison_file, dpi=150, bbox_inches='tight')
    print(f"\nSaved: {comparison_file}")
    plt.close()
    
    # Plot error evolution for all timesteps
    all_times = []
    all_gdisph_errors = []
    all_ssph_errors = []
    
    for i in range(n_snapshots):
        gdisph_snap = read_snapshot_csv(gdisph_files[i])
        ssph_snap = read_snapshot_csv(ssph_files[i])
        
        t_gdisph = gdisph_snap.time
        t_ssph = ssph_snap.time
        
        # Use average time if they differ slightly
        t_avg = (t_gdisph + t_ssph) / 2
        all_times.append(t_avg)
        
        # Get domain for analytical solution
        x_min = min(gdisph_snap.pos[:, 0].min(), ssph_snap.pos[:, 0].min()) - 0.1
        x_max = max(gdisph_snap.pos[:, 0].max(), ssph_snap.pos[:, 0].max()) + 0.1
        
        # Analytical solution for GDISPH
        x_theory_gdisph = np.linspace(x_min, x_max, 1000)
        theory_gdisph = TheoreticalComparison.sod_shock_tube(x_theory_gdisph, t_gdisph, gamma)
        rho_theory_gdisph = np.interp(gdisph_snap.pos[:, 0], theory_gdisph.x, theory_gdisph.rho)
        gdisph_err = np.sqrt(np.mean((gdisph_snap.dens - rho_theory_gdisph)**2))
        all_gdisph_errors.append(gdisph_err)
        
        # Analytical solution for SSPH
        x_theory_ssph = np.linspace(x_min, x_max, 1000)
        theory_ssph = TheoreticalComparison.sod_shock_tube(x_theory_ssph, t_ssph, gamma)
        rho_theory_ssph = np.interp(ssph_snap.pos[:, 0], theory_ssph.x, theory_ssph.rho)
        ssph_err = np.sqrt(np.mean((ssph_snap.dens - rho_theory_ssph)**2))
        all_ssph_errors.append(ssph_err)
    
    # Error evolution plot
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(all_times, all_gdisph_errors, 'b-', linewidth=2, 
            marker='o', markersize=5, label='GDISPH')
    ax.plot(all_times, all_ssph_errors, 'r-', linewidth=2, 
            marker='s', markersize=5, label='SSPH')
    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('L2 Density Error', fontsize=12)
    ax.set_title('Error Evolution: GDISPH vs SSPH vs Analytical Solution', fontsize=14)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=12)
    
    error_file = output_dir / 'shock_tube_error_evolution.png'
    plt.savefig(error_file, dpi=150, bbox_inches='tight')
    print(f"Saved: {error_file}")
    plt.close()
    
    # Summary statistics
    print("\n" + "="*70)
    print("SUMMARY STATISTICS")
    print("="*70)
    print(f"Average L2 error (GDISPH): {np.mean(all_gdisph_errors):.6e}")
    print(f"Average L2 error (SSPH):   {np.mean(all_ssph_errors):.6e}")
    print(f"Max L2 error (GDISPH):     {np.max(all_gdisph_errors):.6e}")
    print(f"Max L2 error (SSPH):       {np.max(all_ssph_errors):.6e}")
    print(f"Min L2 error (GDISPH):     {np.min(all_gdisph_errors):.6e}")
    print(f"Min L2 error (SSPH):       {np.min(all_ssph_errors):.6e}")
    
    # Determine which is better
    gdisph_better = np.sum(np.array(all_gdisph_errors) < np.array(all_ssph_errors))
    ssph_better = np.sum(np.array(all_ssph_errors) < np.array(all_gdisph_errors))
    
    print(f"\nGDISPH had lower error in {gdisph_better}/{n_snapshots} timesteps")
    print(f"SSPH had lower error in {ssph_better}/{n_snapshots} timesteps")
    print("="*70)
    
    # Save summary to file
    summary_file = output_dir / 'comparison_summary.txt'
    with open(summary_file, 'w') as f:
        f.write("SHOCK TUBE COMPARISON: GDISPH vs SSPH vs Analytical\n")
        f.write("="*70 + "\n\n")
        f.write(f"Number of timesteps: {n_snapshots}\n")
        f.write(f"Time range: {all_times[0]:.4f} to {all_times[-1]:.4f}\n")
        f.write(f"Gamma: {gamma}\n\n")
        f.write("Average L2 Density Errors:\n")
        f.write(f"  GDISPH: {np.mean(all_gdisph_errors):.6e}\n")
        f.write(f"  SSPH:   {np.mean(all_ssph_errors):.6e}\n\n")
        f.write("Maximum L2 Density Errors:\n")
        f.write(f"  GDISPH: {np.max(all_gdisph_errors):.6e}\n")
        f.write(f"  SSPH:   {np.max(all_ssph_errors):.6e}\n\n")
        f.write("Minimum L2 Density Errors:\n")
        f.write(f"  GDISPH: {np.min(all_gdisph_errors):.6e}\n")
        f.write(f"  SSPH:   {np.min(all_ssph_errors):.6e}\n\n")
        f.write(f"GDISPH had lower error in {gdisph_better}/{n_snapshots} timesteps\n")
        f.write(f"SSPH had lower error in {ssph_better}/{n_snapshots} timesteps\n")
    
    print(f"\nSaved summary: {summary_file}")


if __name__ == "__main__":
    # Set up paths (relative to script location)
    script_dir = Path(__file__).parent.parent  # Go up to 01_simulation/
    gdisph_dir = script_dir / 'results_gdisph' / 'shock_tube' / 'latest' / 'outputs' / 'csv'
    ssph_dir = script_dir / 'results_ssph' / 'shock_tube' / 'latest' / 'outputs' / 'csv'
    output_dir = script_dir / 'comparison_results'
    
    print("="*70)
    print("SHOCK TUBE METHOD COMPARISON")
    print("="*70)
    print(f"GDISPH directory: {gdisph_dir}")
    print(f"SSPH directory:   {ssph_dir}")
    print(f"Output directory: {output_dir}")
    print()
    
    # Run comparison
    compare_shock_tube_methods(gdisph_dir, ssph_dir, output_dir, gamma=1.4)
    
    print("\n" + "="*70)
    print("COMPARISON COMPLETE!")
    print("="*70)
