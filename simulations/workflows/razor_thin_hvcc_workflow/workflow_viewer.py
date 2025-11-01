#!/usr/bin/env python3
"""
Comprehensive Workflow Viewer
Shows all raw data and animations for each workflow step
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import json
from datetime import datetime

WORKFLOW_ROOT = Path(__file__).parent.resolve()

def print_section(title):
    """Print a formatted section header"""
    print("\n" + "="*80)
    print(f"  {title}")
    print("="*80)

def format_size(bytes_val):
    """Format file size in human-readable format"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_val < 1024.0:
            return f"{bytes_val:.1f} {unit}"
        bytes_val /= 1024.0
    return f"{bytes_val:.1f} TB"

def count_particles(csv_file):
    """Count particles in a CSV file"""
    try:
        with open(csv_file, 'r') as f:
            # Skip comment lines
            lines = [line for line in f if not line.startswith('#')]
            return len(lines) - 1  # Subtract header
    except:
        return 0

def get_time_from_csv(csv_file):
    """Extract time from CSV header"""
    try:
        with open(csv_file, 'r') as f:
            for line in f:
                if line.startswith('# time:'):
                    return float(line.split(':')[1].strip())
        return 0.0
    except:
        return 0.0

def analyze_step1():
    """Analyze Step 1: Disk Relaxation"""
    print_section("STEP 1: DISK RELAXATION")
    
    step1_dir = WORKFLOW_ROOT / "01_relaxation"
    output_dir = step1_dir / "output" / "disk_relaxation"
    
    if not output_dir.exists():
        print("  ❌ No Step 1 output found")
        return
    
    # Find all runs
    runs = sorted([d for d in output_dir.iterdir() if d.is_dir() and d.name.startswith('run_')])
    
    print(f"\n  Found {len(runs)} simulation runs:")
    
    for run_dir in runs:
        print(f"\n  📁 {run_dir.name}")
        
        # Check for metadata
        metadata_file = run_dir / "metadata.json"
        if metadata_file.exists():
            with open(metadata_file) as f:
                metadata = json.load(f)
                print(f"     Started: {metadata.get('start_time', 'unknown')}")
                print(f"     Duration: {metadata.get('wall_time', 'unknown')}")
        
        # Check CSV outputs
        csv_dir = run_dir / "outputs" / "csv"
        if csv_dir.exists():
            csv_files = sorted(csv_dir.glob("*.csv"))
            print(f"\n     CSV Snapshots: {len(csv_files)}")
            
            total_size = sum(f.stat().st_size for f in csv_files)
            print(f"     Total CSV size: {format_size(total_size)}")
            
            if csv_files:
                # Analyze first and last snapshots
                first_file = csv_files[0]
                last_file = csv_files[-1]
                
                n_particles = count_particles(first_file)
                t_start = get_time_from_csv(first_file)
                t_end = get_time_from_csv(last_file)
                
                print(f"     Particles: {n_particles:,}")
                print(f"     Time range: {t_start:.4f} → {t_end:.4f}")
                
                # List individual snapshots
                print(f"\n     Individual snapshots:")
                for csv_file in csv_files:
                    size = format_size(csv_file.stat().st_size)
                    time = get_time_from_csv(csv_file)
                    print(f"       • {csv_file.name}: {size}, t={time:.4f}")
        
        # Check binary outputs
        binary_dir = run_dir / "outputs" / "binary"
        if binary_dir.exists():
            binary_files = sorted(binary_dir.glob("*.sph"))
            if binary_files:
                total_size = sum(f.stat().st_size for f in binary_files)
                print(f"\n     Binary Snapshots: {len(binary_files)}")
                print(f"     Total binary size: {format_size(total_size)}")
        
        # Check visualizations
        viz_dir = WORKFLOW_ROOT / "visualizations" / run_dir.name
        if viz_dir.exists():
            png_files = list(viz_dir.glob("*.png"))
            frames_dir = viz_dir / "frames"
            
            print(f"\n     📊 Visualizations:")
            print(f"       • Snapshot plots: {len([p for p in png_files if 'snapshot' in p.name])}")
            print(f"       • Evolution plots: {len([p for p in png_files if 'evolution' in p.name])}")
            
            if frames_dir.exists():
                frame_files = list(frames_dir.glob("frame_*.png"))
                print(f"       • Movie frames: {len(frame_files)}")
            
            # Check for movies
            mp4_files = list(viz_dir.glob("*.mp4"))
            gif_files = list(viz_dir.glob("*.gif"))
            if mp4_files:
                print(f"       • Movies (MP4): {len(mp4_files)}")
            if gif_files:
                print(f"       • Animations (GIF): {len(gif_files)}")

def analyze_step2():
    """Analyze Step 2: Flyby Encounter"""
    print_section("STEP 2: FLYBY ENCOUNTER")
    
    step2_dir = WORKFLOW_ROOT / "02_flyby"
    output_dir = step2_dir / "output"
    
    if not output_dir.exists():
        print("  ⚠️  Step 2 has not been run yet")
        print("\n  Step 2 Configuration:")
        
        # Show available configs
        configs = sorted(step2_dir.glob("config_*.json"))
        if configs:
            print(f"    Available configs: {len(configs)}")
            for config in configs:
                print(f"      • {config.name}")
        
        # Show initial conditions
        ic_dir = WORKFLOW_ROOT / "initial_conditions"
        ic_files = sorted(ic_dir.glob("*.csv"))
        if ic_files:
            print(f"\n  Initial Conditions ready:")
            for ic_file in ic_files:
                size = format_size(ic_file.stat().st_size)
                n_particles = count_particles(ic_file)
                print(f"      • {ic_file.name}: {n_particles:,} particles, {size}")
        
        return
    
    # Similar analysis as Step 1
    runs = sorted([d for d in output_dir.iterdir() if d.is_dir() and d.name.startswith('run_')])
    print(f"\n  Found {len(runs)} simulation runs:")
    
    # ... (same structure as analyze_step1)

def show_visualization_summary():
    """Show summary of all visualizations"""
    print_section("VISUALIZATION SUMMARY")
    
    viz_dir = WORKFLOW_ROOT / "visualizations"
    
    if not viz_dir.exists():
        print("  ❌ No visualizations directory found")
        return
    
    # Count all visualization files
    all_png = list(viz_dir.glob("**/*.png"))
    all_mp4 = list(viz_dir.glob("**/*.mp4"))
    all_gif = list(viz_dir.glob("**/*.gif"))
    
    print(f"\n  Total Visualization Files:")
    print(f"    • PNG plots: {len(all_png)}")
    print(f"    • MP4 movies: {len(all_mp4)}")
    print(f"    • GIF animations: {len(all_gif)}")
    
    total_size = sum(f.stat().st_size for f in all_png + all_mp4 + all_gif)
    print(f"    • Total size: {format_size(total_size)}")
    
    print(f"\n  📁 Visualization Directory:")
    print(f"    {viz_dir}")
    
    # List run directories
    run_dirs = sorted([d for d in viz_dir.iterdir() if d.is_dir() and d.name.startswith('run_')])
    print(f"\n  Run Visualizations: {len(run_dirs)}")
    for run_dir in run_dirs:
        png_count = len(list(run_dir.glob("*.png")))
        mp4_count = len(list(run_dir.glob("*.mp4")))
        gif_count = len(list(run_dir.glob("*.gif")))
        
        print(f"    • {run_dir.name}:")
        print(f"      - PNGs: {png_count}, MP4s: {mp4_count}, GIFs: {gif_count}")

def show_raw_data_summary():
    """Show summary of all raw data"""
    print_section("RAW DATA SUMMARY")
    
    # Step 1 data
    step1_output = WORKFLOW_ROOT / "01_relaxation" / "output" / "disk_relaxation"
    if step1_output.exists():
        runs = [d for d in step1_output.iterdir() if d.is_dir() and d.name.startswith('run_')]
        
        total_csv = 0
        total_binary = 0
        total_csv_size = 0
        total_binary_size = 0
        
        for run_dir in runs:
            csv_dir = run_dir / "outputs" / "csv"
            if csv_dir.exists():
                csv_files = list(csv_dir.glob("*.csv"))
                total_csv += len(csv_files)
                total_csv_size += sum(f.stat().st_size for f in csv_files)
            
            binary_dir = run_dir / "outputs" / "binary"
            if binary_dir.exists():
                binary_files = list(binary_dir.glob("*.sph"))
                total_binary += len(binary_files)
                total_binary_size += sum(f.stat().st_size for f in binary_files)
        
        print(f"\n  Step 1 (Relaxation) Data:")
        print(f"    • CSV snapshots: {total_csv} files, {format_size(total_csv_size)}")
        print(f"    • Binary snapshots: {total_binary} files, {format_size(total_binary_size)}")
        print(f"    • Total: {format_size(total_csv_size + total_binary_size)}")
    
    # Step 2 data (if exists)
    step2_output = WORKFLOW_ROOT / "02_flyby" / "output"
    if step2_output.exists():
        # Similar analysis
        print(f"\n  Step 2 (Flyby) Data:")
        print(f"    ⚠️  Not yet implemented")
    else:
        print(f"\n  Step 2 (Flyby) Data:")
        print(f"    ⚠️  Not run yet")
    
    # Initial conditions
    ic_dir = WORKFLOW_ROOT / "initial_conditions"
    if ic_dir.exists():
        ic_files = list(ic_dir.glob("*.csv"))
        if ic_files:
            total_ic_size = sum(f.stat().st_size for f in ic_files)
            print(f"\n  Initial Conditions:")
            print(f"    • Files: {len(ic_files)}")
            print(f"    • Total size: {format_size(total_ic_size)}")
            for ic_file in ic_files:
                print(f"      - {ic_file.name}: {format_size(ic_file.stat().st_size)}")

def show_quick_access_guide():
    """Show quick access guide for users"""
    print_section("QUICK ACCESS GUIDE")
    
    print("""
  📊 VIEW VISUALIZATIONS:
    Open any PNG file in the visualizations/ directory:
    
    • Latest run overview:
      visualizations/run_*/evolution_comparison.png
    
    • Individual snapshots:
      visualizations/run_*/snapshot_*.png
    
    • Movies (if generated):
      visualizations/run_*/*.mp4
      visualizations/run_*/*.gif

  📁 ACCESS RAW DATA:
    CSV files (human-readable):
      01_relaxation/output/disk_relaxation/run_*/outputs/csv/*.csv
    
    Binary files (compressed):
      01_relaxation/output/disk_relaxation/run_*/outputs/binary/*.sph

  🔄 REGENERATE VISUALIZATIONS:
    cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow
    python create_visualizations.py

  🎬 CREATE MOVIES MANUALLY:
    cd visualizations/run_*/frames
    
    # MP4 (requires ffmpeg):
    ffmpeg -framerate 2 -pattern_type glob -i 'frame_*.png' \\
      -c:v libx264 -pix_fmt yuv420p -crf 23 ../movie.mp4
    
    # GIF (requires ffmpeg):
    ffmpeg -framerate 2 -pattern_type glob -i 'frame_*.png' \\
      -vf "scale=800:-1" ../animation.gif

  📖 DOCUMENTATION:
    • OUTPUT_GUIDE.md - Detailed output documentation
    • README.md - Workflow overview
    • 01_relaxation/README.md - Physics documentation
    """)

def main():
    """Main workflow viewer"""
    print("\n" + "█"*80)
    print("█" + " "*78 + "█")
    print("█" + "  HVCC WORKFLOW VIEWER".center(78) + "█")
    print("█" + f"  {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}".center(78) + "█")
    print("█" + " "*78 + "█")
    print("█"*80)
    
    # Analyze all steps
    analyze_step1()
    analyze_step2()
    
    # Show summaries
    show_visualization_summary()
    show_raw_data_summary()
    
    # Show quick access guide
    show_quick_access_guide()
    
    print("\n" + "█"*80)
    print("█" + "  END OF REPORT".center(78) + "█")
    print("█"*80 + "\n")

if __name__ == "__main__":
    main()
