#!/usr/bin/env python3
"""
Create comparison animations for all benchmark simulations.
Automatically validates simulation results against analytical solutions.
"""

import sys
import argparse
from pathlib import Path
import subprocess

try:
    from readers import SimulationReader
    from theoretical import TheoreticalComparison
    from plotting import ParticlePlotter, AnimationMaker
except ImportError:
    from analysis.readers import SimulationReader
    from analysis.theoretical import TheoreticalComparison
    from analysis.plotting import ParticlePlotter, AnimationMaker


BENCHMARKS = {
    'shock_tube': {
        'name': 'Sod Shock Tube',
        'script': 'disph_shock_tube_animation.py',
        'analytical': 'sod_shock_tube',
        'dimension': '1D',
        'description': 'Standard Riemann problem for shock capturing validation'
    },
    'sedov_taylor': {
        'name': 'Sedov-Taylor Blast Wave',
        'script': None,  # TODO: Create script
        'analytical': 'sedov_taylor',
        'dimension': '3D',
        'description': 'Self-similar blast wave expansion'
    },
    'gresho_chan_vortex': {
        'name': 'Gresho-Chan Vortex',
        'script': None,  # TODO: Create script
        'analytical': None,  # Stability test - no analytical solution
        'dimension': '2D',
        'description': 'Vortex stability and pressure gradient handling'
    },
    'evrard': {
        'name': 'Evrard Collapse',
        'script': None,  # TODO: Create script
        'analytical': None,  # No simple analytical solution
        'dimension': '3D',
        'description': 'Spherical collapse with gravity'
    }
}


def find_simulation_output(benchmark_name, base_dir='../build/results'):
    """Find simulation output directory for a benchmark."""
    base_path = Path(base_dir)
    
    # Search pattern: results/{METHOD}/{benchmark}/{DIM}
    candidates = []
    if base_path.exists():
        for method_dir in base_path.iterdir():
            if not method_dir.is_dir():
                continue
            benchmark_dir = method_dir / benchmark_name
            if benchmark_dir.exists():
                for dim_dir in benchmark_dir.iterdir():
                    if dim_dir.is_dir():
                        candidates.append(dim_dir)
    
    return candidates


def create_benchmark_animation(benchmark_name, output_dir, force=False):
    """Create animation for a specific benchmark."""
    
    if benchmark_name not in BENCHMARKS:
        print(f"ERROR: Unknown benchmark '{benchmark_name}'")
        print(f"Available benchmarks: {', '.join(BENCHMARKS.keys())}")
        return False
    
    benchmark = BENCHMARKS[benchmark_name]
    print(f"\n{'='*80}")
    print(f"BENCHMARK: {benchmark['name']}")
    print(f"{'='*80}")
    print(f"Description: {benchmark['description']}")
    
    # Check if script exists
    if benchmark['script'] is None:
        print(f"⚠ Warning: Animation script not yet implemented for {benchmark_name}")
        print(f"  You can create a custom script following the pattern in disph_shock_tube_animation.py")
        return False
    
    script_path = Path(__file__).parent / benchmark['script']
    if not script_path.exists():
        print(f"ERROR: Script not found: {script_path}")
        return False
    
    # Find output directory
    if output_dir is None:
        candidates = find_simulation_output(benchmark_name)
        if not candidates:
            print(f"ERROR: No simulation output found for {benchmark_name}")
            print(f"  Run the simulation first: ./sph{benchmark['dimension'].lower()} {benchmark_name} ...")
            return False
        
        # Use most recent
        output_dir = str(candidates[-1])
        print(f"Found output: {output_dir}")
    
    # Check if output exists
    viz_dir = Path(__file__).parent.parent / 'visualizations' / benchmark_name
    output_file = viz_dir / f'{benchmark_name}_comparison.mp4'
    
    if output_file.exists() and not force:
        print(f"✓ Animation already exists: {output_file}")
        print(f"  Use --force to regenerate")
        return True
    
    # Run animation script
    print(f"\nGenerating animation...")
    cmd = [sys.executable, str(script_path), output_dir]
    
    try:
        subprocess.run(cmd, check=True)
        print(f"✓ Animation created: {output_file}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Animation creation failed: {e}")
        return False


def create_all_animations(force=False):
    """Create animations for all benchmarks that have scripts."""
    
    print("\n" + "="*80)
    print("CREATING ALL BENCHMARK ANIMATIONS")
    print("="*80)
    
    success_count = 0
    skip_count = 0
    fail_count = 0
    
    for benchmark_name in BENCHMARKS:
        result = create_benchmark_animation(benchmark_name, None, force)
        if result:
            success_count += 1
        elif BENCHMARKS[benchmark_name]['script'] is None:
            skip_count += 1
        else:
            fail_count += 1
    
    print("\n" + "="*80)
    print("SUMMARY")
    print("="*80)
    print(f"✓ Success: {success_count}")
    print(f"⚠ Skipped: {skip_count} (no script)")
    print(f"✗ Failed:  {fail_count}")
    
    return fail_count == 0


def list_benchmarks():
    """List all available benchmarks."""
    print("\n" + "="*80)
    print("AVAILABLE BENCHMARKS")
    print("="*80)
    
    for name, info in BENCHMARKS.items():
        status = "✓" if info['script'] else "⚠"
        print(f"\n{status} {name}")
        print(f"  Name: {info['name']}")
        print(f"  Dimension: {info['dimension']}")
        print(f"  Analytical: {info['analytical'] or 'None'}")
        print(f"  Description: {info['description']}")
        
        # Check if simulation output exists
        outputs = find_simulation_output(name)
        if outputs:
            print(f"  Output found: {len(outputs)} run(s)")
            for out in outputs:
                print(f"    → {out}")
        else:
            print(f"  Output: Not yet run")


def main():
    parser = argparse.ArgumentParser(
        description='Create benchmark validation animations',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all benchmarks
  python3 create_all_benchmark_animations.py --list
  
  # Create animation for shock tube
  python3 create_all_benchmark_animations.py shock_tube
  
  # Create all animations
  python3 create_all_benchmark_animations.py --all
  
  # Force regeneration
  python3 create_all_benchmark_animations.py shock_tube --force
        """
    )
    
    parser.add_argument(
        'benchmark',
        nargs='?',
        help='Benchmark name to process'
    )
    parser.add_argument(
        '--list', '-l',
        action='store_true',
        help='List all available benchmarks'
    )
    parser.add_argument(
        '--all', '-a',
        action='store_true',
        help='Process all benchmarks'
    )
    parser.add_argument(
        '--force', '-f',
        action='store_true',
        help='Force regeneration even if animation exists'
    )
    parser.add_argument(
        '--output-dir', '-o',
        help='Override simulation output directory'
    )
    
    args = parser.parse_args()
    
    if args.list:
        list_benchmarks()
    elif args.all:
        create_all_animations(args.force)
    elif args.benchmark:
        create_benchmark_animation(args.benchmark, args.output_dir, args.force)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
