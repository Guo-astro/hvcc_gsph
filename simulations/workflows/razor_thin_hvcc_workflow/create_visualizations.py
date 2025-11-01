#!/usr/bin/env python3
"""
Create all visualizations (PNG images and movies) for the HVCC workflow.

This script creates:
1. Individual snapshot visualizations (PNG)
2. Evolution plots showing all snapshots
3. Animated movies (MP4) of the relaxation process

All outputs are saved in: workflow_root/visualizations/
"""

import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import subprocess

# Parse CLI args early so WORKFLOW_ROOT and VIZ_DIR can be overridden
parser = argparse.ArgumentParser(description='Generate visualizations for a workflow or a given output directory.')
parser.add_argument('--root', '-r', help='Workflow root directory (where visualizations/ will be created).', default=None)
parser.add_argument('--output-dir', '-o', help='Optional explicit simulation output directory (absolute path) to read CSV snapshots from).', default=None)
args = parser.parse_known_args()[0]

# Workflow root directory (can be overridden)
if args.root:
    WORKFLOW_ROOT = Path(args.root).resolve()
else:
    WORKFLOW_ROOT = Path(__file__).parent.resolve()

VIZ_DIR = WORKFLOW_ROOT / "visualizations"
VIZ_DIR.mkdir(exist_ok=True)

def read_csv_snapshot(filepath):
    """Read SPH snapshot from CSV file."""
    try:
        data = np.loadtxt(filepath, delimiter=',', skiprows=1)
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None
    
    # Column order: x, y, z, vx, vy, vz, rho, p, u, h, mass, ax, ay, az
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

def create_snapshot_plot(data, snapshot_num, output_path):
    """Create a detailed plot for a single snapshot."""
    
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    fig.suptitle(f'Disk Relaxation - Snapshot {snapshot_num:05d}', fontsize=16)
    
    r = np.sqrt(data['x']**2 + data['y']**2)
    
    # XY projection (face-on view) - Density
    ax = axes[0, 0]
    sc = ax.scatter(data['x'], data['y'], c=data['rho'], s=2, cmap='viridis', alpha=0.6)
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Face-on (XY) - Density')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Density')
    
    # XZ projection (edge-on view) - Density
    ax = axes[0, 1]
    sc = ax.scatter(data['x'], data['z'], c=data['rho'], s=2, cmap='viridis', alpha=0.6)
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('z (pc)')
    ax.set_title('Edge-on (XZ) - Density')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Density')
    
    # Radial density profile
    ax = axes[0, 2]
    ax.scatter(r, data['rho'], s=2, alpha=0.3, c='blue')
    ax.set_xlabel('Radius (pc)')
    ax.set_ylabel('Density')
    ax.set_title('Radial Density Profile')
    if data['rho'].max() > 0:
        ax.set_yscale('log')
    
    # Pressure distribution
    ax = axes[1, 0]
    sc = ax.scatter(data['x'], data['y'], c=data['p'], s=2, cmap='plasma', alpha=0.6)
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Face-on - Pressure')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='Pressure')
    
    # Velocity magnitude
    ax = axes[1, 1]
    v_mag = np.sqrt(data['vx']**2 + data['vy']**2 + data['vz']**2)
    sc = ax.scatter(data['x'], data['y'], c=v_mag, s=2, cmap='hot', alpha=0.6)
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title('Velocity Magnitude')
    ax.set_aspect('equal')
    plt.colorbar(sc, ax=ax, label='|v|')
    
    # Statistics
    ax = axes[1, 2]
    ax.axis('off')
    stats_text = f"""
Snapshot {snapshot_num:05d}
{'='*20}
Particles: {len(data['x'])}

Total mass: {data['mass'].sum():.1f} M☉
Mass/particle: {data['mass'].mean():.3f} M☉

Radius (95%): {np.percentile(r, 95):.3f} pc
Height (RMS): {np.std(data['z']):.4f} pc

Density:
  Mean: {data['rho'].mean():.2e}
  Max:  {data['rho'].max():.2e}
  Min:  {data['rho'].min():.2e}

Velocity (RMS):
  vx: {np.sqrt(np.mean(data['vx']**2)):.2e}
  vy: {np.sqrt(np.mean(data['vy']**2)):.2e}
  vz: {np.sqrt(np.mean(data['vz']**2)):.2e}
  |v|: {np.sqrt(np.mean(v_mag**2)):.2e}
"""
    ax.text(0.05, 0.5, stats_text, family='monospace', fontsize=9,
            verticalalignment='center')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {output_path.name}")

def create_evolution_comparison(snapshot_files, output_path):
    """Create side-by-side comparison of all snapshots."""
    
    n_snapshots = len(snapshot_files)
    fig, axes = plt.subplots(2, n_snapshots, figsize=(4*n_snapshots, 8))
    fig.suptitle('Disk Relaxation Evolution', fontsize=16)
    
    # Find global density range for consistent coloring
    all_rho = []
    for csv_file in snapshot_files:
        data = read_csv_snapshot(csv_file)
        if data:
            all_rho.extend(data['rho'])
    vmax = np.percentile(all_rho, 99) if all_rho else 1.0
    
    for i, csv_file in enumerate(snapshot_files):
        data = read_csv_snapshot(csv_file)
        if not data:
            continue
            
        snapshot_num = int(csv_file.stem)
        
        # Face-on view
        ax = axes[0, i] if n_snapshots > 1 else axes[0]
        sc = ax.scatter(data['x'], data['y'], c=data['rho'], s=1, cmap='viridis',
                       vmin=0, vmax=vmax, alpha=0.6)
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('y (pc)')
        ax.set_title(f'T={snapshot_num:05d}')
        ax.set_aspect('equal')
        if i == n_snapshots - 1:
            plt.colorbar(sc, ax=ax, label='Density')
        
        # Edge-on view
        ax = axes[1, i] if n_snapshots > 1 else axes[1]
        sc = ax.scatter(data['x'], data['z'], c=data['rho'], s=1, cmap='viridis',
                       vmin=0, vmax=vmax, alpha=0.6)
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('z (pc)')
        ax.set_title('Edge-on')
        ax.set_aspect('equal')
        if i == n_snapshots - 1:
            plt.colorbar(sc, ax=ax, label='Density')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {output_path.name}")

def create_movie_frames(snapshot_files, output_dir, run_name):
    """Create individual movie frames with consistent axes and coloring"""
    print(f"\n3. Creating movie frames...")
    
    frames_dir = output_dir
    frames_dir.mkdir(exist_ok=True)
    
    # Read all snapshots first
    snapshots = []
    for csv_file in snapshot_files:
        data = read_csv_snapshot(csv_file)
        if data:
            snapshots.append(data)
    
    if not snapshots:
        print("  No valid snapshots to process")
        return
    
    # Get global ranges for consistent coloring
    all_rho = np.concatenate([s['rho'] for s in snapshots])
    rho_min, rho_max = np.percentile(all_rho, [1, 99])
    
    # Get global spatial extent
    all_x = np.concatenate([s['x'] for s in snapshots])
    all_y = np.concatenate([s['y'] for s in snapshots])
    all_z = np.concatenate([s['z'] for s in snapshots])
    xy_max = max(np.abs(all_x).max(), np.abs(all_y).max()) * 1.1
    xz_max = max(np.abs(all_x).max(), np.abs(all_z).max()) * 1.1
    
    for i, snapshot in enumerate(snapshots):
        # Use even dimensions for h264 compatibility (12*100=1200, 6*100=600)
        fig, axes = plt.subplots(1, 2, figsize=(12, 6), dpi=100)
        
        x, y, z = snapshot['x'], snapshot['y'], snapshot['z']
        rho = snapshot['rho']
        
        # Face-on view (XY)
        ax = axes[0]
        sc = ax.scatter(x, y, c=rho, s=1, cmap='viridis', vmin=rho_min, vmax=rho_max)
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('y (pc)')
        ax.set_title(f'Face-on View - Frame {i}')
        ax.set_xlim(-xy_max, xy_max)
        ax.set_ylim(-xy_max, xy_max)
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3)
        
        # Edge-on view (XZ)
        ax = axes[1]
        sc = ax.scatter(x, z, c=rho, s=1, cmap='viridis', vmin=rho_min, vmax=rho_max)
        ax.set_xlabel('x (pc)')
        ax.set_ylabel('z (pc)')
        ax.set_title(f'Edge-on View - Frame {i}')
        ax.set_xlim(-xz_max, xz_max)
        ax.set_ylim(-xz_max, xz_max)
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3)
        
        # Colorbar
        plt.colorbar(sc, ax=axes, label='Density (M☉/pc³)')
        
        plt.tight_layout()
        frame_path = frames_dir / f"frame_{i:04d}.png"
        # Don't use bbox_inches='tight' to maintain exact dimensions
        plt.savefig(frame_path, dpi=100)
        plt.close()
    
    print(f"  Created {len(snapshots)} frames in {frames_dir}")

def create_movie(frames_dir, output_movie, fps=5):
    """Create MP4 movie from frames using ffmpeg."""
    
    if not frames_dir.exists() or not list(frames_dir.glob("frame_*.png")):
        print(f"No frames found in {frames_dir}")
        return False
    
    # Check if ffmpeg is available
    try:
        subprocess.run(['ffmpeg', '-version'], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("WARNING: ffmpeg not found. Cannot create movie.")
        print("Install ffmpeg with: brew install ffmpeg")
        return False
    
    print(f"\n4. Creating MP4 movie: {output_movie.name}")
    
    cmd = [
        'ffmpeg', '-y',  # Overwrite output
        '-framerate', str(fps),
        '-pattern_type', 'glob',
        '-i', str(frames_dir / 'frame_*.png'),
        '-c:v', 'libx264',
        '-pix_fmt', 'yuv420p',
        '-crf', '23',  # Quality (lower = better)
        str(output_movie)
    ]
    
    try:
        subprocess.run(cmd, check=True, capture_output=True)
        print(f"  ✅ Saved: {output_movie.name}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"  ❌ Error creating MP4: {e}")
        return False

def create_gif(frames_dir, output_gif, fps=2):
    """Create GIF animation from frames using PIL or ffmpeg."""
    
    if not frames_dir.exists() or not list(frames_dir.glob("frame_*.png")):
        print(f"No frames found in {frames_dir}")
        return False
    
    # Try with PIL first (no external dependencies)
    try:
        from PIL import Image
        
        frame_files = sorted(frames_dir.glob("frame_*.png"))
        frames = [Image.open(f) for f in frame_files]
        
        # Save as GIF
        frames[0].save(
            output_gif,
            save_all=True,
            append_images=frames[1:],
            duration=int(1000/fps),  # milliseconds per frame
            loop=0
        )
        print(f"  ✅ Saved GIF: {output_gif.name}")
        return True
        
    except ImportError:
        # Fall back to ffmpeg if PIL not available
        try:
            subprocess.run(['ffmpeg', '-version'], capture_output=True, check=True)
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("  ❌ Neither PIL nor ffmpeg available for GIF creation")
            return False
        
        cmd = [
            'ffmpeg', '-y',
            '-framerate', str(fps),
            '-pattern_type', 'glob',
            '-i', str(frames_dir / 'frame_*.png'),
            '-vf', 'scale=800:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse',
            str(output_gif)
        ]
        
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"  ✅ Saved GIF (ffmpeg): {output_gif.name}")
            return True
        except subprocess.CalledProcessError:
            print(f"  ❌ Error creating GIF with ffmpeg")
            return False


def process_run_directory(run_dir):
    """Process all snapshots from a run directory."""
    
    csv_dir = run_dir / "outputs" / "csv"
    if not csv_dir.exists():
        print(f"No CSV directory found in {run_dir}")
        return
    
    snapshot_files = sorted(csv_dir.glob("*.csv"))
    if not snapshot_files:
        print(f"No CSV snapshots found in {csv_dir}")
        return
    
    run_name = run_dir.name
    print(f"\n{'='*60}")
    print(f"Processing: {run_name}")
    print(f"Snapshots: {len(snapshot_files)}")
    print(f"{'='*60}")
    
    # Create subdirectory for this run
    run_viz_dir = VIZ_DIR / run_name
    run_viz_dir.mkdir(exist_ok=True)
    
    # 1. Create individual snapshot plots
    print("\n1. Creating individual snapshot plots...")
    for csv_file in snapshot_files:
        data = read_csv_snapshot(csv_file)
        if not data:
            continue
        snapshot_num = int(csv_file.stem)
        output_path = run_viz_dir / f"snapshot_{snapshot_num:05d}.png"
        create_snapshot_plot(data, snapshot_num, output_path)
    
    # 2. Create evolution comparison
    print("\n2. Creating evolution comparison plot...")
    evolution_path = run_viz_dir / "evolution_comparison.png"
    create_evolution_comparison(snapshot_files, evolution_path)
    
    # 3. Create movie frames
    frames_dir = run_viz_dir / "frames"
    create_movie_frames(snapshot_files, frames_dir, run_name)
    
    # 4. Create movie (MP4)
    movie_path = run_viz_dir / f"{run_name}_relaxation.mp4"
    success = create_movie(frames_dir, movie_path, fps=2)
    
    # If MP4 failed, try creating GIF as fallback
    if not success:
        print("  Trying GIF animation instead...")
        gif_path = run_viz_dir / f"{run_name}_relaxation.gif"
        create_gif(frames_dir, gif_path, fps=2)
    
    print(f"\n✅ Completed: {run_name}")
    print(f"   Output directory: {run_viz_dir}")

def main():
    print("="*60)
    print("HVCC Workflow Visualization Generator")
    print("="*60)
    print(f"Workflow root: {WORKFLOW_ROOT}")
    print(f"Output directory: {VIZ_DIR}")
    
    # Find all run directories
    output_dir = WORKFLOW_ROOT / "01_relaxation" / "output" / "disk_relaxation"
    
    if not output_dir.exists():
        print(f"\nError: Output directory not found: {output_dir}")
        sys.exit(1)
    
    run_dirs = sorted([d for d in output_dir.iterdir() 
                      if d.is_dir() and d.name.startswith("run_")])
    
    if not run_dirs:
        print(f"\nNo run directories found in {output_dir}")
        sys.exit(1)
    
    print(f"\nFound {len(run_dirs)} run(s)")
    
    # Process each run
    for run_dir in run_dirs:
        process_run_directory(run_dir)
    
    # Also process initial conditions if available
    ic_dir = WORKFLOW_ROOT / "initial_conditions"
    ic_file = ic_dir / "relaxed_disk_test.csv"
    if ic_file.exists():
        print(f"\n{'='*60}")
        print("Processing initial conditions file")
        print(f"{'='*60}")
        
        data = read_csv_snapshot(ic_file)
        if data:
            output_path = VIZ_DIR / "initial_conditions_overview.png"
            create_snapshot_plot(data, 0, output_path)
    
    print("\n" + "="*60)
    print("✅ ALL VISUALIZATIONS COMPLETE!")
    print("="*60)
    print(f"\nAll outputs saved to: {VIZ_DIR}/")
    print("\nGenerated files:")
    print("  • Individual snapshot PNGs")
    print("  • Evolution comparison plots")
    print("  • Movie frames")
    print("  • MP4 movies (if ffmpeg available)")
    print("\n")

if __name__ == '__main__':
    main()
