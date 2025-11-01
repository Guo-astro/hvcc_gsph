#!/usr/bin/env python3
"""
Visualize the relaxed disk from the HVCC workflow.

Usage:
    python visualize_disk.py [snapshot_file]
    
If no file specified, uses the latest test IC file.
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

def read_csv_snapshot(filepath):
    """Read SPH snapshot from CSV file."""
    data = np.loadtxt(filepath, delimiter=',', skiprows=1)
    
    # Column order from framework:
    # x, y, z, vx, vy, vz, rho, p, u, h, mass, ax, ay, az
    cols = {
        'x': data[:, 0],
        'y': data[:, 1],
        'z': data[:, 2],
        'vx': data[:, 3],
        'vy': data[:, 4],
        'vz': data[:, 5],
        'rho': data[:, 6],
        'p': data[:, 7],
        'u': data[:, 8],
        'h': data[:, 9],
        'mass': data[:, 10],
        'ax': data[:, 11],
        'ay': data[:, 12],
        'az': data[:, 13],
    }
    
    return cols

def plot_disk_overview(data, output_path=None):
    """Create overview plots of the disk."""
    
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    fig.suptitle('Razor-Thin Disk Relaxation - Overview', fontsize=16)
    
    # XY projection (face-on view)
    ax = axes[0, 0]
    sc = ax.scatter(data['x'], data['y'], c=data['rho'], s=1, cmap='viridis')
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Face-on (XY) - Density')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Density')
    
    # XZ projection (edge-on view)
    ax = axes[0, 1]
    sc = ax.scatter(data['x'], data['z'], c=data['rho'], s=1, cmap='viridis')
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('z (pc)')
    ax.set_title('Edge-on (XZ) - Density')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Density')
    
    # Radial density profile
    ax = axes[0, 2]
    r = np.sqrt(data['x']**2 + data['y']**2)
    ax.scatter(r, data['rho'], s=1, alpha=0.3)
    ax.set_xlabel('Radius (pc)')
    ax.set_ylabel('Density')
    ax.set_title('Radial Density Profile')
    ax.set_yscale('log')
    
    # Velocity field (face-on)
    ax = axes[1, 0]
    # Subsample for clarity
    step = max(1, len(data['x']) // 1000)
    ax.quiver(data['x'][::step], data['y'][::step], 
              data['vx'][::step], data['vy'][::step],
              alpha=0.6)
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Velocity Field (XY)')
    ax.set_aspect('equal')
    
    # Pressure distribution
    ax = axes[1, 1]
    sc = ax.scatter(data['x'], data['y'], c=data['p'], s=1, cmap='plasma')
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Face-on - Pressure')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Pressure')
    
    # Statistics
    ax = axes[1, 2]
    ax.axis('off')
    stats_text = f"""
    Disk Statistics:
    ----------------
    Particles: {len(data['x'])}
    
    Total mass: {data['mass'].sum():.1f} M☉
    Mass per particle: {data['mass'].mean():.3f} M☉
    
    Radius (95%): {np.percentile(r, 95):.2f} pc
    Height (RMS): {np.std(data['z']):.3f} pc
    
    Density:
      Mean: {data['rho'].mean():.2e}
      Max: {data['rho'].max():.2e}
      Min: {data['rho'].min():.2e}
    
    Velocity:
      Max |v|: {np.sqrt(data['vx']**2 + data['vy']**2 + data['vz']**2).max():.2e}
      RMS: {np.sqrt(np.mean(data['vx']**2 + data['vy']**2 + data['vz']**2)):.2e}
    """
    ax.text(0.1, 0.5, stats_text, family='monospace', fontsize=10,
            verticalalignment='center')
    
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Saved plot to: {output_path}")
    else:
        plt.show()

def plot_relaxation_evolution(snapshot_dir):
    """Plot how the disk evolves during relaxation."""
    
    # Find all CSV snapshots
    csv_files = sorted(Path(snapshot_dir).glob('*.csv'))
    
    if len(csv_files) < 2:
        print(f"Need at least 2 snapshots. Found: {len(csv_files)}")
        return
    
    fig, axes = plt.subplots(2, len(csv_files), figsize=(4*len(csv_files), 8))
    fig.suptitle('Disk Relaxation Evolution', fontsize=16)
    
    for i, csv_file in enumerate(csv_files):
        data = read_csv_snapshot(csv_file)
        snapshot_num = csv_file.stem
        
        # Face-on view
        ax = axes[0, i] if len(csv_files) > 1 else axes[0]
        sc = ax.scatter(data['x'], data['y'], c=data['rho'], s=1, cmap='viridis',
                       vmin=0, vmax=np.percentile(data['rho'], 99))
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('y (pc)')
        ax.set_title(f'Snapshot {snapshot_num}')
        ax.set_aspect('equal')
        if i == len(csv_files) - 1:
            plt.colorbar(sc, ax=ax, label='Density')
        
        # Edge-on view
        ax = axes[1, i] if len(csv_files) > 1 else axes[1]
        sc = ax.scatter(data['x'], data['z'], c=data['rho'], s=1, cmap='viridis',
                       vmin=0, vmax=np.percentile(data['rho'], 99))
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('z (pc)')
        ax.set_title(f'Edge-on')
        ax.set_aspect('equal')
        if i == len(csv_files) - 1:
            plt.colorbar(sc, ax=ax, label='Density')
    
    plt.tight_layout()
    
    output_path = Path(snapshot_dir).parent / 'relaxation_evolution.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved evolution plot to: {output_path}")
    plt.show()

if __name__ == '__main__':
    # Default to the test IC file
    default_file = Path(__file__).parent / 'initial_conditions' / 'relaxed_disk_test.csv'
    
    if len(sys.argv) > 1:
        filepath = Path(sys.argv[1])
    else:
        filepath = default_file
    
    if not filepath.exists():
        print(f"Error: File not found: {filepath}")
        print(f"\nUsage: python {sys.argv[0]} [snapshot_file.csv]")
        sys.exit(1)
    
    print(f"Reading: {filepath}")
    data = read_csv_snapshot(filepath)
    
    # Create overview plot
    output_path = filepath.parent / f'{filepath.stem}_visualization.png'
    plot_disk_overview(data, output_path)
    
    # If this is a single snapshot, also check for evolution plots
    if 'initial_conditions' not in str(filepath):
        # This is from an output directory, try to plot evolution
        snapshot_dir = filepath.parent
        if len(list(snapshot_dir.glob('*.csv'))) > 1:
            print("\nCreating evolution plot...")
            plot_relaxation_evolution(snapshot_dir)
    
    print("\nDone!")
