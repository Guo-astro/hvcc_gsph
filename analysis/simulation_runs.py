"""
Simulation run management tools for SPH code.

Load, query, and analyze simulation runs from the new organized structure:
simulations/{sample_name}/{run_id}/
"""

from pathlib import Path
import json
from typing import List, Optional, Dict
from dataclasses import dataclass
from datetime import datetime


@dataclass
class SimulationRunInfo:
    """Information about a simulation run"""
    run_id: str
    sample_name: str
    description: str
    created_at: str
    sph_type: str
    dimension: int
    particle_count: int
    git_hash: str
    snapshot_count: int


class SimulationRunReader:
    """Read a single simulation run"""
    
    def __init__(self, run_directory: str):
        """
        Initialize reader for a simulation run directory.
        
        Args:
            run_directory: Path to run directory (e.g., simulations/shock_tube/run_2025-11-01_143052_disph_1d)
        """
        self.run_dir = Path(run_directory)
        if not self.run_dir.exists():
            raise FileNotFoundError(f"Run directory not found: {run_directory}")
        
        # Load metadata
        metadata_file = self.run_dir / "metadata.json"
        if metadata_file.exists():
            with open(metadata_file) as f:
                self.metadata = json.load(f)
        else:
            self.metadata = {}
        
        # Load config
        config_file = self.run_dir / "config.json"
        if config_file.exists():
            with open(config_file) as f:
                self.config = json.load(f)
        else:
            self.config = {}
    
    def get_info(self) -> SimulationRunInfo:
        """Get summary information about this run"""
        run_info = self.metadata.get('run_info', {})
        code_version = self.metadata.get('code_version', {})
        sim_params = self.metadata.get('simulation_params', {})
        output_info = self.metadata.get('output', {})
        
        return SimulationRunInfo(
            run_id=run_info.get('run_id', 'unknown'),
            sample_name=run_info.get('sample_name', 'unknown'),
            description=run_info.get('description', ''),
            created_at=run_info.get('created_at', 'unknown'),
            sph_type=sim_params.get('sph_type', 'unknown'),
            dimension=sim_params.get('dimension', 0),
            particle_count=sim_params.get('particle_count', 0),
            git_hash=code_version.get('git_hash', 'unknown'),
            snapshot_count=output_info.get('snapshot_count', 0)
        )
    
    def get_output_reader(self, format: str = "binary"):
        """
        Get a reader for output snapshots.
        
        Args:
            format: Output format ("csv", "binary", "numpy")
            
        Returns:
            Reader object (BinarySimulationReader or SimulationReader)
        """
        output_dir = self.run_dir / "outputs" / format
        
        if not output_dir.exists():
            raise FileNotFoundError(f"Output directory not found: {output_dir}")
        
        if format == "binary":
            from analysis.binary_reader import BinarySimulationReader
            return BinarySimulationReader(str(output_dir))
        else:
            from analysis.readers import SimulationReader
            return SimulationReader(str(output_dir))
    
    def get_initial_conditions(self):
        """Load initial conditions as pandas DataFrame"""
        import pandas as pd
        ic_file = self.run_dir / "initial_conditions.csv"
        if not ic_file.exists():
            return None
        return pd.read_csv(ic_file)
    
    def get_visualizations_dir(self) -> Path:
        """Get visualizations directory path"""
        return self.run_dir / "visualizations"
    
    def get_logs_dir(self) -> Path:
        """Get logs directory path"""
        return self.run_dir / "logs"
    
    def get_analysis_dir(self) -> Path:
        """Get analysis directory path"""
        return self.run_dir / "analysis"
    
    def __repr__(self):
        info = self.get_info()
        return (f"SimulationRun(run_id='{info.run_id}', "
                f"sample='{info.sample_name}', "
                f"created='{info.created_at}', "
                f"method={info.sph_type}, "
                f"dim={info.dimension}D, "
                f"particles={info.particle_count})")


class SimulationRunFinder:
    """Find and query simulation runs"""
    
    def __init__(self, base_dir: str = "simulations"):
        """
        Initialize run finder.
        
        Args:
            base_dir: Base simulations directory
        """
        self.base_dir = Path(base_dir)
        if not self.base_dir.exists():
            raise FileNotFoundError(f"Simulations directory not found: {base_dir}")
    
    def find_latest(self, sample_name: str) -> Optional[SimulationRunReader]:
        """
        Find latest run for a sample.
        
        Args:
            sample_name: Sample name (e.g., "shock_tube")
            
        Returns:
            SimulationRunReader or None if not found
        """
        latest_link = self.base_dir / sample_name / "latest"
        if latest_link.exists() and latest_link.is_symlink():
            return SimulationRunReader(latest_link)
        
        # If no symlink, find most recent by directory name
        runs = self.find_all(sample_name)
        if runs:
            return runs[-1]  # Last one (most recent)
        return None
    
    def find_all(self, sample_name: str) -> List[SimulationRunReader]:
        """
        Find all runs for a sample.
        
        Args:
            sample_name: Sample name
            
        Returns:
            List of SimulationRunReader objects, sorted by creation time
        """
        sample_dir = self.base_dir / sample_name
        if not sample_dir.exists():
            return []
        
        runs = []
        for run_dir in sorted(sample_dir.glob("run_*")):
            if run_dir.is_dir():
                try:
                    runs.append(SimulationRunReader(run_dir))
                except Exception as e:
                    print(f"Warning: Could not load run {run_dir}: {e}")
        
        return runs
    
    def find_by_date(self, sample_name: str, date: str) -> List[SimulationRunReader]:
        """
        Find runs on a specific date.
        
        Args:
            sample_name: Sample name
            date: Date string in format YYYY-MM-DD
            
        Returns:
            List of SimulationRunReader objects
        """
        sample_dir = self.base_dir / sample_name
        if not sample_dir.exists():
            return []
        
        runs = []
        for run_dir in sample_dir.glob(f"run_{date}*"):
            if run_dir.is_dir():
                try:
                    runs.append(SimulationRunReader(run_dir))
                except Exception as e:
                    print(f"Warning: Could not load run {run_dir}: {e}")
        
        return sorted(runs, key=lambda r: r.metadata.get('run_info', {}).get('created_at', ''))
    
    def find_by_git_hash(self, git_hash: str) -> List[SimulationRunReader]:
        """
        Find all runs with a specific git hash.
        
        Args:
            git_hash: Git commit hash (short or full)
            
        Returns:
            List of SimulationRunReader objects
        """
        runs = []
        for sample_dir in self.base_dir.glob("*/"):
            if not sample_dir.is_dir():
                continue
            for run_dir in sample_dir.glob("run_*"):
                if not run_dir.is_dir():
                    continue
                try:
                    reader = SimulationRunReader(run_dir)
                    run_hash = reader.metadata.get('code_version', {}).get('git_hash', '')
                    if run_hash.startswith(git_hash) or git_hash.startswith(run_hash):
                        runs.append(reader)
                except Exception:
                    pass
        
        return runs
    
    def list_samples(self) -> List[str]:
        """Get list of all sample names with runs"""
        samples = []
        for sample_dir in self.base_dir.glob("*/"):
            if sample_dir.is_dir() and list(sample_dir.glob("run_*")):
                samples.append(sample_dir.name)
        return sorted(samples)
    
    def get_summary(self) -> Dict[str, int]:
        """Get summary of all runs"""
        summary = {}
        for sample in self.list_samples():
            runs = self.find_all(sample)
            summary[sample] = len(runs)
        return summary


def compare_runs(run1: SimulationRunReader, run2: SimulationRunReader) -> Dict:
    """
    Compare two simulation runs.
    
    Returns:
        Dictionary with differences
    """
    info1 = run1.get_info()
    info2 = run2.get_info()
    
    differences = {}
    
    # Compare parameters
    if info1.sph_type != info2.sph_type:
        differences['sph_type'] = (info1.sph_type, info2.sph_type)
    
    if info1.particle_count != info2.particle_count:
        differences['particle_count'] = (info1.particle_count, info2.particle_count)
    
    if info1.git_hash != info2.git_hash:
        differences['git_hash'] = (info1.git_hash, info2.git_hash)
    
    # Compare full simulation parameters
    params1 = run1.metadata.get('simulation_params', {})
    params2 = run2.metadata.get('simulation_params', {})
    
    for key in set(params1.keys()) | set(params2.keys()):
        val1 = params1.get(key)
        val2 = params2.get(key)
        if val1 != val2:
            differences[f'param_{key}'] = (val1, val2)
    
    return differences


# Convenience functions
def load_latest(sample_name: str, base_dir: str = "simulations") -> Optional[SimulationRunReader]:
    """Load the latest run for a sample"""
    finder = SimulationRunFinder(base_dir)
    return finder.find_latest(sample_name)


def load_run(run_directory: str) -> SimulationRunReader:
    """Load a specific run by directory path"""
    return SimulationRunReader(run_directory)


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python simulation_runs.py <sample_name>          # Show latest run")
        print("  python simulation_runs.py <run_directory>        # Show specific run")
        print("  python simulation_runs.py --list                 # List all samples")
        print("  python simulation_runs.py --summary              # Show run counts")
        sys.exit(1)
    
    if sys.argv[1] == "--list":
        finder = SimulationRunFinder()
        samples = finder.list_samples()
        print(f"Available samples ({len(samples)}):")
        for sample in samples:
            runs = finder.find_all(sample)
            print(f"  {sample}: {len(runs)} run(s)")
    
    elif sys.argv[1] == "--summary":
        finder = SimulationRunFinder()
        summary = finder.get_summary()
        print("Simulation Runs Summary:")
        print(f"  Total samples: {len(summary)}")
        print(f"  Total runs: {sum(summary.values())}")
        print("\nPer sample:")
        for sample, count in sorted(summary.items()):
            print(f"  {sample}: {count} run(s)")
    
    elif Path(sys.argv[1]).is_dir():
        # Load specific run directory
        run = load_run(sys.argv[1])
        print(run)
        print("\nMetadata:")
        print(json.dumps(run.metadata, indent=2))
    
    else:
        # Load latest run for sample
        run = load_latest(sys.argv[1])
        if run:
            print(run)
            print("\nRun Info:")
            info = run.get_info()
            print(f"  Sample: {info.sample_name}")
            print(f"  Created: {info.created_at}")
            print(f"  Method: {info.sph_type} ({info.dimension}D)")
            print(f"  Particles: {info.particle_count}")
            print(f"  Snapshots: {info.snapshot_count}")
            print(f"  Git Hash: {info.git_hash}")
            print(f"\nDirectory: {run.run_dir}")
        else:
            print(f"No runs found for sample: {sys.argv[1]}")
