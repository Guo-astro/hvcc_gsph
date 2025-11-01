#!/usr/bin/env python3
"""
Animation script for disk relaxation visualization
Creates animations showing the evolution of the disk during relaxation
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter
import pandas as pd
import glob
import os
import sys

def load_snapshot(filename):
    """Load a CSV snapshot"""
    try:
        df = pd.read_csv(filename)
        return df
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def create_2d_density_plot(df, ax, title=""):
    """Create a 2D density plot (x-y plane)"""
    ax.clear()
    
    # Get particle positions and densities
    x = df['x'].values
    y = df['y'].values
    density = df['dens'].values
    
    # Create scatter plot colored by density
    scatter = ax.scatter(x, y, c=density, s=1, cmap='viridis', 
                        vmin=0, vmax=density.max()*0.8, alpha=0.6)
    
    ax.set_xlabel('x (pc)')
    ax.set_ylabel('y (pc)')
    ax.set_title(title)
    ax.set_aspect('equal')
    ax.grid(True, alpha=0.3)
    
    return scatter

def create_relaxation_animation(output_dir, output_file="relaxation.mp4"):
    """Create animation from relaxation snapshots"""
    
    # Find all CSV files
    csv_pattern = os.path.join(output_dir, "outputs/csv/*.csv")
    csv_files = sorted(glob.glob(csv_pattern))
    
    if len(csv_files) == 0:
        print(f"No CSV files found in {csv_pattern}")
        return
    
    print(f"Found {len(csv_files)} snapshots")
    
    # Load first snapshot to get ranges
    df0 = load_snapshot(csv_files[0])
    if df0 is None:
        return
    
    # Create figure with subplots
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('Disk Relaxation Evolution', fontsize=14)
    
    def update(frame):
        if frame >= len(csv_files):
            return
        
        df = load_snapshot(csv_files[frame])
        if df is None:
            return
        
        # Plot 1: X-Y density plot
        create_2d_density_plot(df, axes[0], 
                              f"Density (x-y plane) - Snapshot {frame:05d}")
        
        # Plot 2: Radial density profile
        axes[1].clear()
        x = df['x'].values
        y = df['y'].values
        r = np.sqrt(x**2 + y**2)
        density = df['dens'].values
        
        # Bin the data
        r_bins = np.linspace(0, r.max(), 50)
        density_profile = []
        r_centers = []
        
        for i in range(len(r_bins)-1):
            mask = (r >= r_bins[i]) & (r < r_bins[i+1])
            if np.sum(mask) > 0:
                density_profile.append(np.mean(density[mask]))
                r_centers.append((r_bins[i] + r_bins[i+1])/2)
        
        axes[1].plot(r_centers, density_profile, 'b-', linewidth=2)
        axes[1].set_xlabel('Radius (pc)')
        axes[1].set_ylabel('Density')
        axes[1].set_title(f'Radial Density Profile - Snapshot {frame:05d}')
        axes[1].grid(True, alpha=0.3)
        axes[1].set_ylim(0, max(density_profile)*1.1 if density_profile else 1)
        
        plt.tight_layout()
    
    # Create animation
    anim = FuncAnimation(fig, update, frames=len(csv_files), 
                        interval=200, repeat=True)
    
    # Save animation
    print(f"Saving animation to {output_file}...")
    writer = FFMpegWriter(fps=5, bitrate=1800)
    anim.save(output_file, writer=writer)
    print(f"Animation saved successfully!")
    
    plt.close()

def create_static_comparison(output_dir, output_file="relaxation_comparison.png"):
    """Create a static comparison showing initial and final states"""
    
    csv_pattern = os.path.join(output_dir, "outputs/csv/*.csv")
    csv_files = sorted(glob.glob(csv_pattern))
    
    if len(csv_files) < 2:
        print("Need at least 2 snapshots for comparison")
        return
    
    df_initial = load_snapshot(csv_files[0])
    df_final = load_snapshot(csv_files[-1])
    
    if df_initial is None or df_final is None:
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    fig.suptitle('Disk Relaxation: Initial vs Final State', fontsize=16)
    
    # Initial state - density plot
    create_2d_density_plot(df_initial, axes[0,0], "Initial Density (x-y)")
    
    # Final state - density plot  
    create_2d_density_plot(df_final, axes[0,1], "Final Density (x-y)")
    
    # Initial state - radial profile
    x = df_initial['x'].values
    y = df_initial['y'].values
    r_init = np.sqrt(x**2 + y**2)
    dens_init = df_initial['dens'].values
    
    r_bins = np.linspace(0, r_init.max(), 50)
    profile_init = []
    r_centers = []
    
    for i in range(len(r_bins)-1):
        mask = (r_init >= r_bins[i]) & (r_init < r_bins[i+1])
        if np.sum(mask) > 0:
            profile_init.append(np.mean(dens_init[mask]))
            r_centers.append((r_bins[i] + r_bins[i+1])/2)
    
    axes[1,0].plot(r_centers, profile_init, 'b-', linewidth=2, label='Initial')
    axes[1,0].set_xlabel('Radius (pc)')
    axes[1,0].set_ylabel('Density')
    axes[1,0].set_title('Initial Radial Profile')
    axes[1,0].grid(True, alpha=0.3)
    axes[1,0].legend()
    
    # Final state - radial profile
    x = df_final['x'].values
    y = df_final['y'].values
    r_final = np.sqrt(x**2 + y**2)
    dens_final = df_final['dens'].values
    
    profile_final = []
    r_centers_final = []
    
    for i in range(len(r_bins)-1):
        mask = (r_final >= r_bins[i]) & (r_final < r_bins[i+1])
        if np.sum(mask) > 0:
            profile_final.append(np.mean(dens_final[mask]))
            r_centers_final.append((r_bins[i] + r_bins[i+1])/2)
    
    axes[1,1].plot(r_centers_final, profile_final, 'r-', linewidth=2, label='Final')
    axes[1,1].set_xlabel('Radius (pc)')
    axes[1,1].set_ylabel('Density')
    axes[1,1].set_title('Final Radial Profile')
    axes[1,1].grid(True, alpha=0.3)
    axes[1,1].legend()
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"Comparison plot saved to {output_file}")
    plt.close()

if __name__ == "__main__":
    # Get the latest run directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_base = os.path.join(script_dir, "output/disk_relaxation")
    
    # Find the latest run
    runs = glob.glob(os.path.join(output_base, "run_*"))
    if not runs:
        print("No simulation runs found!")
        sys.exit(1)
    
    latest_run = max(runs, key=os.path.getmtime)
    print(f"Using run directory: {latest_run}")
    
    # Create static comparison
    print("\nCreating static comparison plot...")
    create_static_comparison(latest_run, 
                            os.path.join(script_dir, "relaxation_comparison.png"))
    
    # Create animation if ffmpeg is available
    print("\nCreating animation...")
    try:
        create_relaxation_animation(latest_run,
                                   os.path.join(script_dir, "relaxation_animation.mp4"))
    except Exception as e:
        print(f"Could not create animation: {e}")
        print("Make sure ffmpeg is installed: brew install ffmpeg")
