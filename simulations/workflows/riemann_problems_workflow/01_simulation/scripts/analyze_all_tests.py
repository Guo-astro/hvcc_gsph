#!/usr/bin/env python3
"""
Compare SPH simulations with analytical solutions for 1D Riemann problems.

This script analyzes multiple Riemann problem test cases and compares
the numerical results with exact Riemann solver solutions.
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
import sys
import json
from typing import Dict, List, Tuple

# Add analysis module to path
sys.path.append(str(Path(__file__).parent.parent.parent.parent.parent / 'analysis'))

from theoretical import TheoreticalComparison, ShockTubeSolution
from readers import ParticleSnapshot

# Test case configurations
TEST_CASES = {
    1: {
        'name': 'Sod Shock Tube',
        'rho_L': 1.0, 'P_L': 1.0, 'u_L': 0.0,
        'rho_R': 0.125, 'P_R': 0.1, 'u_R': 0.0,
        'result_dir': 'results_test1_sod',
        't_sample': [0.0, 0.03, 0.06, 0.09, 0.12],
    },
    2: {
        'name': 'Double Rarefaction',
        'rho_L': 1.0, 'P_L': 0.4, 'u_L': -2.0,
        'rho_R': 1.0, 'P_R': 0.4, 'u_R': 2.0,
        'result_dir': 'results_test2_rarefaction',
        't_sample': [0.0, 0.04, 0.08, 0.12, 0.15],
    },
    3: {
        'name': 'Strong Shock',
        'rho_L': 1.0, 'P_L': 1000.0, 'u_L': 0.0,
        'rho_R': 1.0, 'P_R': 0.01, 'u_R': 0.0,
        'result_dir': 'results_test3_strong',
        't_sample': [0.0, 0.003, 0.006, 0.009, 0.012],
    },
    5: {
        'name': 'Vacuum Generation',
        'rho_L': 1.0, 'P_L': 1.0, 'u_L': -1.0,
        'rho_R': 1.0, 'P_R': 1.0, 'u_R': 1.0,
        'result_dir': 'results_test5_vacuum',
        't_sample': [0.0, 0.02, 0.04, 0.06, 0.08],
    }
}


def read_snapshot_csv(csv_file: Path, dim: int = 1) -> ParticleSnapshot:
    """Read snapshot from CSV with metadata."""
    df = pd.read_csv(csv_file)
    
    # Read metadata
    meta_file = csv_file.parent / (csv_file.stem + '.meta.json')
    time = 0.0
    if meta_file.exists():
        with open(meta_file, 'r') as f:
            metadata = json.load(f)
            time = metadata['simulation'].get('time', 0.0)
    
    # Build snapshot
    n = len(df)
    pos = np.zeros((n, 3))
    vel = np.zeros((n, 3))
    acc = np.zeros((n, 3))
    
    if 'pos_x' in df.columns: pos[:, 0] = df['pos_x'].values
    if 'vel_x' in df.columns: vel[:, 0] = df['vel_x'].values
    
    snapshot = ParticleSnapshot(
        time=time,
        num_particles=n,
        pos=pos,
        vel=vel,
        acc=acc,
        dens=df['dens'].values if 'dens' in df.columns else np.ones(n),
        pres=df['pres'].values if 'pres' in df.columns else np.ones(n),
        ene=df['ene'].values if 'ene' in df.columns else np.ones(n),
        mass=df['mass'].values if 'mass' in df.columns else np.ones(n),
        sml=df['sml'].values if 'sml' in df.columns else np.ones(n),
        particle_id=np.arange(n)
    )
    
    return snapshot


def compute_l2_error(sim_vals: np.ndarray, ana_vals: np.ndarray) -> float:
    """Compute L2 relative error."""
    diff = sim_vals - ana_vals
    norm_diff = np.linalg.norm(diff)
    norm_ana = np.linalg.norm(ana_vals)
    return norm_diff / norm_ana if norm_ana > 0 else 0.0


def analyze_test_case(test_id: int, gamma: float = 1.4) -> Dict:
    """Analyze a single Riemann problem test case."""
    config = TEST_CASES[test_id]
    print(f"\n{'='*70}")
    print(f"TEST {test_id}: {config['name']}")
    print(f"{'='*70}")
    
    # Find snapshots
    result_dir = Path(config['result_dir']) / "riemann_problems" / "latest" / "outputs" / "csv"
    
    if not result_dir.exists():
        print(f"Warning: Result directory not found: {result_dir}")
        return None
    
    csv_files = sorted(result_dir.glob("*.csv"))
    print(f"Found {len(csv_files)} snapshots")
    
    if len(csv_files) == 0:
        print("No snapshots found!")
        return None
    
    # Select snapshots at desired times
    times_to_plot = config['t_sample']
    snapshot_indices = []
    
    for target_t in times_to_plot:
        # Find snapshot closest to target time
        best_idx = 0
        best_diff = float('inf')
        
        for idx, csv_file in enumerate(csv_files):
            meta_file = csv_file.parent / (csv_file.stem + '.meta.json')
            if meta_file.exists():
                with open(meta_file) as f:
                    meta = json.load(f)
                    t = meta['simulation']['time']
                    diff = abs(t - target_t)
                    if diff < best_diff:
                        best_diff = diff
                        best_idx = idx
        
        snapshot_indices.append(best_idx)
    
    # Analyze selected snapshots
    errors = {'density': [], 'velocity': [], 'pressure': [], 'energy': []}
    
    fig, axes = plt.subplots(4, 1, figsize=(12, 16))
    colors = plt.cm.viridis(np.linspace(0, 0.9, len(snapshot_indices)))
    
    for plot_idx, snap_idx in enumerate(snapshot_indices):
        csv_file = csv_files[snap_idx]
        snap = read_snapshot_csv(csv_file)
        
        x = snap.pos[:, 0]
        rho_sim = snap.dens
        vel_sim = snap.vel[:, 0]
        pres_sim = snap.pres
        ene_sim = snap.ene
        t = snap.time
        
        # Compute analytical solution
        sol = TheoreticalComparison.riemann_problem(
            x, t,
            config['rho_L'], config['P_L'], config['u_L'],
            config['rho_R'], config['P_R'], config['u_R'],
            gamma=gamma, x_interface=0.0
        )
        
        # Compute errors
        err_rho = compute_l2_error(rho_sim, sol.rho)
        err_vel = compute_l2_error(vel_sim, sol.vel)
        err_pres = compute_l2_error(pres_sim, sol.pres)
        err_ene = compute_l2_error(ene_sim, sol.ene)
        
        errors['density'].append(err_rho)
        errors['velocity'].append(err_vel)
        errors['pressure'].append(err_pres)
        errors['energy'].append(err_ene)
        
        print(f"\nTime: {t:.4f}")
        print(f"  L2 errors: ρ={err_rho:.4f}, v={err_vel:.4f}, P={err_pres:.4f}, e={err_ene:.4f}")
        
        # Plot
        color = colors[plot_idx]
        label = f"t={t:.3f}"
        
        axes[0].plot(x, rho_sim, 'o', color=color, alpha=0.6, markersize=2, label=f"SPH {label}")
        axes[0].plot(sol.x, sol.rho, '-', color=color, linewidth=2, label=f"Exact {label}")
        
        axes[1].plot(x, vel_sim, 'o', color=color, alpha=0.6, markersize=2)
        axes[1].plot(sol.x, sol.vel, '-', color=color, linewidth=2)
        
        axes[2].plot(x, pres_sim, 'o', color=color, alpha=0.6, markersize=2)
        axes[2].plot(sol.x, sol.pres, '-', color=color, linewidth=2)
        
        axes[3].plot(x, ene_sim, 'o', color=color, alpha=0.6, markersize=2)
        axes[3].plot(sol.x, sol.ene, '-', color=color, linewidth=2)
    
    # Format plots
    axes[0].set_ylabel('Density', fontsize=12)
    axes[0].legend(fontsize=8, ncol=2)
    axes[0].grid(True, alpha=0.3)
    
    axes[1].set_ylabel('Velocity', fontsize=12)
    axes[1].grid(True, alpha=0.3)
    
    axes[2].set_ylabel('Pressure', fontsize=12)
    axes[2].grid(True, alpha=0.3)
    
    axes[3].set_ylabel('Specific Energy', fontsize=12)
    axes[3].set_xlabel('Position', fontsize=12)
    axes[3].grid(True, alpha=0.3)
    
    fig.suptitle(f"Test {test_id}: {config['name']}", fontsize=14, fontweight='bold')
    plt.tight_layout()
    
    # Save figure
    output_dir = Path('comparison_results')
    output_dir.mkdir(exist_ok=True)
    fig_path = output_dir / f"test{test_id}_{config['result_dir'].split('_')[-1]}_comparison.png"
    plt.savefig(fig_path, dpi=150, bbox_inches='tight')
    print(f"\nSaved: {fig_path}")
    plt.close()
    
    # Print summary
    print(f"\n{'='*70}")
    print(f"SUMMARY - Test {test_id}: {config['name']}")
    print(f"{'='*70}")
    print(f"Average L2 errors:")
    print(f"  Density:  {np.mean(errors['density']):.4f}")
    print(f"  Velocity: {np.mean(errors['velocity']):.4f}")
    print(f"  Pressure: {np.mean(errors['pressure']):.4f}")
    print(f"  Energy:   {np.mean(errors['energy']):.4f}")
    
    return errors


def main():
    """Main analysis function."""
    print("="*70)
    print("1D RIEMANN PROBLEMS BENCHMARK SUITE")
    print("="*70)
    
    all_results = {}
    
    for test_id in TEST_CASES.keys():
        try:
            errors = analyze_test_case(test_id)
            if errors:
                all_results[test_id] = errors
        except Exception as e:
            print(f"\nError analyzing test {test_id}: {e}")
            import traceback
            traceback.print_exc()
    
    print("\n" + "="*70)
    print("BENCHMARK SUMMARY")
    print("="*70)
    
    for test_id, errors in all_results.items():
        config = TEST_CASES[test_id]
        print(f"\nTest {test_id} - {config['name']}:")
        print(f"  Avg L2 density error:  {np.mean(errors['density']):.4f}")
        print(f"  Avg L2 velocity error: {np.mean(errors['velocity']):.4f}")
        print(f"  Avg L2 pressure error: {np.mean(errors['pressure']):.4f}")
    
    print("\n" + "="*70)
    print("ANALYSIS COMPLETE")
    print("="*70)


if __name__ == '__main__':
    main()
