#!/usr/bin/env python3
"""
Automatic visualization generation for Riemann problems.

Generates:
1. Initial conditions plot (all physics variables with units)
2. Final timestep plot (all physics variables with units)
3. Animation of evolution
4. Screenshot at key timesteps

Usage:
    python generate_visualizations.py <csv_directory> <output_directory> [test_name] [gamma]
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
import json
from typing import Dict, List, Tuple, Optional

# Add analysis package to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent.parent / 'analysis'))

from readers import ParticleSnapshot
from theoretical import riemann_problem


def load_metadata(csv_dir: Path) -> Dict:
    """Load metadata.json from CSV directory."""
    metadata_file = csv_dir / "metadata.json"
    if metadata_file.exists():
        with open(metadata_file, 'r') as f:
            return json.load(f)
    return {}


def read_csv_snapshot(csv_file: Path, metadata: Dict) -> ParticleSnapshot:
    """Read a single CSV snapshot with metadata."""
    import pandas as pd
    
    df = pd.read_csv(csv_file)
    n = len(df)
    
    # Get time from metadata if available
    time = df['time'].iloc[0] if 'time' in df.columns else 0.0
    
    # Build snapshot
    pos = np.zeros((n, 3))
    vel = np.zeros((n, 3))
    acc = np.zeros((n, 3))
    
    if 'pos_x' in df.columns: pos[:, 0] = df['pos_x'].values
    if 'pos_y' in df.columns: pos[:, 1] = df['pos_y'].values
    if 'pos_z' in df.columns: pos[:, 2] = df['pos_z'].values
    
    if 'vel_x' in df.columns: vel[:, 0] = df['vel_x'].values
    if 'vel_y' in df.columns: vel[:, 1] = df['vel_y'].values
    if 'vel_z' in df.columns: vel[:, 2] = df['vel_z'].values
    
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
        particle_id=df['id'].values if 'id' in df.columns else np.arange(n)
    )
    
    return snapshot


def get_units_from_metadata(metadata: Dict) -> Dict[str, str]:
    """Extract unit labels from metadata."""
    units = {}
    if 'units' in metadata:
        u = metadata['units']
        units['length'] = u.get('length', '')
        units['time'] = u.get('time', '')
        units['velocity'] = f"{u.get('length', '')}/{u.get('time', '')}"
        units['density'] = u.get('density', '')
        units['pressure'] = u.get('pressure', '')
        units['energy'] = u.get('energy', '')
    else:
        # Default dimensionless
        units = {k: '' for k in ['length', 'time', 'velocity', 'density', 'pressure', 'energy']}
    
    return units


def plot_physics_state(snapshot: ParticleSnapshot, 
                       analytical_sol: Optional[Dict] = None,
                       units: Optional[Dict] = None,
                       title_suffix: str = "") -> plt.Figure:
    """
    Create 4-panel plot of all physics variables.
    
    Args:
        snapshot: Particle snapshot
        analytical_sol: Optional dictionary with analytical solution
        units: Dictionary of unit labels
        title_suffix: Additional text for plot title
    """
    if units is None:
        units = {k: '' for k in ['length', 'velocity', 'density', 'pressure', 'energy']}
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'Riemann Problem - t = {snapshot.time:.4f} {units.get("time", "")} {title_suffix}', 
                 fontsize=14, fontweight='bold')
    
    x = snapshot.pos[:, 0]
    
    # Sort by position for cleaner plots
    sort_idx = np.argsort(x)
    x_sorted = x[sort_idx]
    
    # Density
    ax = axes[0, 0]
    ax.plot(x_sorted, snapshot.dens[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
    if analytical_sol:
        ax.plot(analytical_sol['x'], analytical_sol['rho'], 'r-', linewidth=2, label='Analytical')
    ax.set_xlabel(f'Position [{units["length"]}]')
    ax.set_ylabel(f'Density [{units["density"]}]')
    ax.set_title('Density')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Velocity
    ax = axes[0, 1]
    ax.plot(x_sorted, snapshot.vel[sort_idx, 0], 'bo', markersize=3, label='SPH', alpha=0.6)
    if analytical_sol:
        ax.plot(analytical_sol['x'], analytical_sol['vel'], 'r-', linewidth=2, label='Analytical')
    ax.set_xlabel(f'Position [{units["length"]}]')
    ax.set_ylabel(f'Velocity [{units["velocity"]}]')
    ax.set_title('Velocity')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Pressure
    ax = axes[1, 0]
    ax.plot(x_sorted, snapshot.pres[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
    if analytical_sol:
        ax.plot(analytical_sol['x'], analytical_sol['pres'], 'r-', linewidth=2, label='Analytical')
    ax.set_xlabel(f'Position [{units["length"]}]')
    ax.set_ylabel(f'Pressure [{units["pressure"]}]')
    ax.set_title('Pressure')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Specific Internal Energy
    ax = axes[1, 1]
    ax.plot(x_sorted, snapshot.ene[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
    if analytical_sol:
        ax.plot(analytical_sol['x'], analytical_sol['ene'], 'r-', linewidth=2, label='Analytical')
    ax.set_xlabel(f'Position [{units["length"]}]')
    ax.set_ylabel(f'Specific Energy [{units["energy"]}]')
    ax.set_title('Specific Internal Energy')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig


def get_riemann_config(test_name: str) -> Dict:
    """Get Riemann problem configuration for analytical solution."""
    configs = {
        'test1_sod': {
            'rho_L': 1.0, 'P_L': 1.0, 'u_L': 0.0,
            'rho_R': 0.125, 'P_R': 0.1, 'u_R': 0.0,
            'name': 'Sod Shock Tube'
        },
        'test2_rarefaction': {
            'rho_L': 1.0, 'P_L': 0.4, 'u_L': -2.0,
            'rho_R': 1.0, 'P_R': 0.4, 'u_R': 2.0,
            'name': 'Double Rarefaction'
        },
        'test3_strong': {
            'rho_L': 1.0, 'P_L': 1000.0, 'u_L': 0.0,
            'rho_R': 1.0, 'P_R': 0.01, 'u_R': 0.0,
            'name': 'Strong Shock'
        },
        'test5_vacuum': {
            'rho_L': 1.0, 'P_L': 1.0, 'u_L': -1.0,
            'rho_R': 1.0, 'P_R': 1.0, 'u_R': 1.0,
            'name': 'Vacuum Generation'
        },
    }
    return configs.get(test_name, configs['test1_sod'])


def compute_analytical_solution(x: np.ndarray, t: float, config: Dict, gamma: float = 1.4) -> Dict:
    """Compute analytical Riemann solution."""
    try:
        sol = riemann_problem(
            x=x, t=t,
            rho_L=config['rho_L'], P_L=config['P_L'], u_L=config['u_L'],
            rho_R=config['rho_R'], P_R=config['P_R'], u_R=config['u_R'],
            x0=0.0, gamma=gamma
        )
        return {
            'x': x,
            'rho': sol.rho,
            'vel': sol.vel,
            'pres': sol.pres,
            'ene': sol.ene
        }
    except Exception as e:
        print(f"Warning: Could not compute analytical solution: {e}")
        return None


def create_animation_video(snapshots: List[ParticleSnapshot], 
                          output_file: Path,
                          config: Dict,
                          units: Dict,
                          gamma: float = 1.4):
    """Create animation video of simulation evolution."""
    print(f"Creating animation with {len(snapshots)} frames...")
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    def animate(frame_idx):
        snap = snapshots[frame_idx]
        x = snap.pos[:, 0]
        sort_idx = np.argsort(x)
        x_sorted = x[sort_idx]
        
        # Compute analytical solution for this timestep
        x_ana = np.linspace(x.min(), x.max(), 200)
        ana_sol = compute_analytical_solution(x_ana, snap.time, config, gamma)
        
        for ax in axes.flat:
            ax.clear()
        
        fig.suptitle(f'{config["name"]} - t = {snap.time:.4f} {units.get("time", "")}', 
                     fontsize=14, fontweight='bold')
        
        # Density
        ax = axes[0, 0]
        ax.plot(x_sorted, snap.dens[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
        if ana_sol:
            ax.plot(ana_sol['x'], ana_sol['rho'], 'r-', linewidth=2, label='Analytical')
        ax.set_xlabel(f'Position [{units["length"]}]')
        ax.set_ylabel(f'Density [{units["density"]}]')
        ax.set_title('Density')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Velocity
        ax = axes[0, 1]
        ax.plot(x_sorted, snap.vel[sort_idx, 0], 'bo', markersize=3, label='SPH', alpha=0.6)
        if ana_sol:
            ax.plot(ana_sol['x'], ana_sol['vel'], 'r-', linewidth=2, label='Analytical')
        ax.set_xlabel(f'Position [{units["length"]}]')
        ax.set_ylabel(f'Velocity [{units["velocity"]}]')
        ax.set_title('Velocity')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Pressure
        ax = axes[1, 0]
        ax.plot(x_sorted, snap.pres[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
        if ana_sol:
            ax.plot(ana_sol['x'], ana_sol['pres'], 'r-', linewidth=2, label='Analytical')
        ax.set_xlabel(f'Position [{units["length"]}]')
        ax.set_ylabel(f'Pressure [{units["pressure"]}]')
        ax.set_title('Pressure')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Energy
        ax = axes[1, 1]
        ax.plot(x_sorted, snap.ene[sort_idx], 'bo', markersize=3, label='SPH', alpha=0.6)
        if ana_sol:
            ax.plot(ana_sol['x'], ana_sol['ene'], 'r-', linewidth=2, label='Analytical')
        ax.set_xlabel(f'Position [{units["length"]}]')
        ax.set_ylabel(f'Specific Energy [{units["energy"]}]')
        ax.set_title('Specific Internal Energy')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
    
    anim = animation.FuncAnimation(fig, animate, frames=len(snapshots), 
                                   interval=100, repeat=True)
    
    # Save as MP4 (requires ffmpeg) or GIF
    try:
        anim.save(str(output_file), writer='ffmpeg', fps=10, dpi=100)
        print(f"Animation saved: {output_file}")
    except Exception as e:
        # Fallback to GIF
        gif_file = output_file.with_suffix('.gif')
        anim.save(str(gif_file), writer='pillow', fps=5, dpi=80)
        print(f"Animation saved as GIF: {gif_file}")
        print(f"(MP4 failed: {e})")
    
    plt.close(fig)


def main():
    if len(sys.argv) < 3:
        print("Usage: python generate_visualizations.py <csv_directory> <output_directory> [test_name] [gamma]")
        print("Example: python generate_visualizations.py results_test1_sod/riemann_problems/latest/outputs/csv \\")
        print("         results_test1_sod/riemann_problems/latest/visualizations test1_sod 1.4")
        sys.exit(1)
    
    csv_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    test_name = sys.argv[3] if len(sys.argv) > 3 else 'test1_sod'
    gamma = float(sys.argv[4]) if len(sys.argv) > 4 else 1.4
    
    print("=" * 70)
    print("AUTOMATIC VISUALIZATION GENERATION")
    print("=" * 70)
    print(f"CSV directory: {csv_dir}")
    print(f"Output directory: {output_dir}")
    print(f"Test: {test_name}")
    print(f"Gamma: {gamma}\n")
    
    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load metadata
    metadata = load_metadata(csv_dir)
    units = get_units_from_metadata(metadata)
    
    # Load Riemann configuration
    config = get_riemann_config(test_name)
    
    # Find all CSV files
    csv_files = sorted(csv_dir.glob("*.csv"))
    if not csv_files:
        print(f"ERROR: No CSV files found in {csv_dir}")
        sys.exit(1)
    
    print(f"Found {len(csv_files)} snapshots\n")
    
    # Read all snapshots
    print("Loading snapshots...")
    snapshots = []
    for csv_file in csv_files:
        snap = read_csv_snapshot(csv_file, metadata)
        snapshots.append(snap)
    
    # Generate visualizations
    print("\n" + "=" * 70)
    print("GENERATING VISUALIZATIONS")
    print("=" * 70)
    
    # 1. Initial conditions
    print("\n1. Initial conditions plot...")
    initial_snap = snapshots[0]
    x_ana = np.linspace(-0.5, 0.5, 200)
    ana_sol_initial = compute_analytical_solution(x_ana, initial_snap.time, config, gamma)
    
    fig = plot_physics_state(initial_snap, ana_sol_initial, units, "(Initial Conditions)")
    initial_file = output_dir / "initial_conditions.png"
    fig.savefig(initial_file, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"   Saved: {initial_file}")
    
    # 2. Final timestep
    print("\n2. Final timestep plot...")
    final_snap = snapshots[-1]
    ana_sol_final = compute_analytical_solution(x_ana, final_snap.time, config, gamma)
    
    fig = plot_physics_state(final_snap, ana_sol_final, units, "(Final State)")
    final_file = output_dir / "final_state.png"
    fig.savefig(final_file, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"   Saved: {final_file}")
    
    # 3. Key timesteps (25%, 50%, 75%)
    print("\n3. Key timestep screenshots...")
    key_indices = [
        len(snapshots) // 4,
        len(snapshots) // 2,
        3 * len(snapshots) // 4
    ]
    
    for idx in key_indices:
        snap = snapshots[idx]
        ana_sol = compute_analytical_solution(x_ana, snap.time, config, gamma)
        
        fig = plot_physics_state(snap, ana_sol, units, f"(t = {snap.time:.4f})")
        screenshot_file = output_dir / f"screenshot_t{snap.time:.4f}.png"
        fig.savefig(screenshot_file, dpi=150, bbox_inches='tight')
        plt.close(fig)
        print(f"   Saved: {screenshot_file}")
    
    # 4. Animation
    print("\n4. Creating animation...")
    # Sample every Nth frame to keep animation reasonable
    step = max(1, len(snapshots) // 50)  # Max 50 frames
    sampled_snapshots = snapshots[::step]
    
    animation_file = output_dir / "evolution_animation.mp4"
    create_animation_video(sampled_snapshots, animation_file, config, units, gamma)
    
    print("\n" + "=" * 70)
    print("VISUALIZATION GENERATION COMPLETE!")
    print("=" * 70)
    print(f"Output directory: {output_dir}")
    print(f"\nGenerated files:")
    print(f"  - initial_conditions.png")
    print(f"  - final_state.png")
    print(f"  - screenshot_t*.png (3 files)")
    print(f"  - evolution_animation.mp4 (or .gif)")
    print("=" * 70)


if __name__ == "__main__":
    main()
