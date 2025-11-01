"""
Data readers for GSPH simulation output files.
"""

import os
import glob
import numpy as np
import pandas as pd
from pathlib import Path
from typing import Optional, List, Dict
from dataclasses import dataclass

try:
    from units import UnitFactory, UnitSystem
except ImportError:
    try:
        from analysis.units import UnitFactory, UnitSystem
    except ImportError:
        # Fallback if units module not available
        UnitFactory = None
        UnitSystem = None


@dataclass
class ParticleSnapshot:
    """Single particle snapshot at a given time."""
    time: float
    num_particles: int
    
    # Particle data
    pos: np.ndarray  # Shape: (N, DIM)
    vel: np.ndarray  # Shape: (N, DIM)
    acc: np.ndarray  # Shape: (N, DIM)
    mass: np.ndarray  # Shape: (N,)
    dens: np.ndarray  # Shape: (N,)
    pres: np.ndarray  # Shape: (N,)
    ene: np.ndarray   # Shape: (N,)
    sml: np.ndarray   # Shape: (N,)
    particle_id: np.ndarray  # Shape: (N,)
    
    # Optional fields
    neighbor_count: Optional[np.ndarray] = None
    alpha: Optional[np.ndarray] = None
    shock_sensor: Optional[np.ndarray] = None
    
    # Unit system information
    units: Optional[object] = None  # UnitSystem object
    
    # Additional scalar/vector arrays
    extra_scalars: Optional[Dict[str, np.ndarray]] = None
    extra_vectors: Optional[Dict[str, np.ndarray]] = None
    
    @property
    def dim(self) -> int:
        """Spatial dimension of simulation."""
        return self.pos.shape[1]
    
    def total_mass(self) -> float:
        """Total mass in simulation."""
        return float(np.sum(self.mass))
    
    def total_momentum(self) -> np.ndarray:
        """Total momentum vector."""
        momentum: np.ndarray = np.sum(self.mass[:, np.newaxis] * self.vel, axis=0)
        return momentum
    
    def total_kinetic_energy(self) -> float:
        """Total kinetic energy."""
        return float(0.5 * np.sum(self.mass * np.sum(self.vel**2, axis=1)))
    
    def total_thermal_energy(self) -> float:
        """Total thermal energy."""
        return float(np.sum(self.mass * self.ene))
    
    def center_of_mass(self) -> np.ndarray:
        """Center of mass position."""
        com: np.ndarray = np.sum(self.mass[:, np.newaxis] * self.pos, axis=0) / self.total_mass()
        return com
    
    def center_of_mass_velocity(self) -> np.ndarray:
        """Center of mass velocity."""
        return self.total_momentum() / self.total_mass()


@dataclass
class EnergyHistory:
    """Energy history over simulation."""
    time: np.ndarray
    kinetic: np.ndarray
    thermal: np.ndarray
    potential: np.ndarray
    total: np.ndarray
    
    def relative_error(self, reference_time: float = 0.0) -> np.ndarray:
        """
        Relative energy error compared to reference time.
        
        Args:
            reference_time: Time to use as reference (default: t=0)
        
        Returns:
            Relative error: (E(t) - E(t0)) / E(t0)
        """
        idx = np.argmin(np.abs(self.time - reference_time))
        E0 = self.total[idx]
        error: np.ndarray = (self.total - E0) / E0
        return error


class SimulationReader:
    """
    Read GSPH simulation output files.
    
    Supports:
    - CSV particle snapshots (00000.csv, 00001.csv, ...)
    - Energy history (energy.txt)
    """
    
    def __init__(self, output_dir: str):
        """
        Initialize reader.
        
        Args:
            output_dir: Path to simulation output directory
        """
        self.output_dir = Path(output_dir)
        if not self.output_dir.exists():
            raise FileNotFoundError(f"Output directory not found: {output_dir}")
        
        self._snapshot_files = sorted(glob.glob(str(self.output_dir / "*.csv")))
        
        # Check for energy file (try both .txt and .dat extensions)
        energy_txt = self.output_dir / "energy.txt"
        energy_dat = self.output_dir / "energy.dat"
        self._energy_file: Optional[Path] = None
        if energy_txt.exists():
            self._energy_file = energy_txt
        elif energy_dat.exists():
            self._energy_file = energy_dat
        else:
            # Check parent directories (energy file might be at parent level)
            parent_energy_txt = self.output_dir.parent / "energy.txt"
            parent_energy_dat = self.output_dir.parent / "energy.dat"
            if parent_energy_txt.exists():
                self._energy_file = parent_energy_txt
            elif parent_energy_dat.exists():
                self._energy_file = parent_energy_dat
        
        # Auto-detect dimension and units from first snapshot
        self._dim: Optional[int] = None
        self._units: Optional[object] = None  # UnitSystem object
        if self._snapshot_files:
            with open(self._snapshot_files[0], 'r') as f:
                header = f.readline().strip()
            
            # Detect dimension
            if 'pos_z [' in header:
                self._dim = 3
            elif 'pos_y [' in header:
                self._dim = 2
            else:
                self._dim = 1
            
            # Detect unit system
            if UnitFactory is not None:
                self._units = UnitFactory.detect_from_csv_header(header)
            else:
                self._units = None
    
    @property
    def num_snapshots(self) -> int:
        """Number of particle snapshots available."""
        return len(self._snapshot_files)
    
    @property
    def dim(self) -> int:
        """Spatial dimension of simulation."""
        if self._dim is None:
            raise ValueError("Dimension not set - no snapshots available")
        return self._dim
    
    @property
    def units(self) -> Optional[object]:
        """Unit system detected from simulation output."""
        return self._units
    
    def snapshot_times(self) -> np.ndarray:
        """Get times of all snapshots."""
        times = []
        for f in self._snapshot_files:
            df = pd.read_csv(f, nrows=1)
            col = [c for c in df.columns if 'time' in c.lower()][0]
            times.append(df[col].iloc[0])
        return np.array(times)
    
    def read_snapshot(self, index: int) -> ParticleSnapshot:
        """
        Read particle snapshot by index.
        
        Args:
            index: Snapshot index (0 = first snapshot)
        
        Returns:
            ParticleSnapshot object
        """
        if index < 0 or index >= len(self._snapshot_files):
            raise IndexError(f"Snapshot index {index} out of range [0, {len(self._snapshot_files)})")
        
        df = pd.read_csv(self._snapshot_files[index])
        
        # Extract time from first row
        time_col = [c for c in df.columns if 'time' in c.lower()][0]
        time = df[time_col].iloc[0]
        
        # Extract dimension-specific columns
        if self._dim == 1:
            pos_cols = [c for c in df.columns if 'pos_x [' in c]
            vel_cols = [c for c in df.columns if 'vel_x [' in c]
            acc_cols = [c for c in df.columns if 'acc_x [' in c]
            pos = df[pos_cols].values
            vel = df[vel_cols].values
            acc = df[acc_cols].values
        elif self._dim == 2:
            pos_cols = [c for c in df.columns if 'pos_x [' in c or 'pos_y [' in c]
            vel_cols = [c for c in df.columns if 'vel_x [' in c or 'vel_y [' in c]
            acc_cols = [c for c in df.columns if 'acc_x [' in c or 'acc_y [' in c]
            pos = df[pos_cols].values
            vel = df[vel_cols].values
            acc = df[acc_cols].values
        else:  # dim == 3
            pos_cols = [c for c in df.columns if 'pos_' in c and '[' in c][:3]
            vel_cols = [c for c in df.columns if 'vel_' in c and '[' in c][:3]
            acc_cols = [c for c in df.columns if 'acc_' in c and '[' in c][:3]
            pos = df[pos_cols].values
            vel = df[vel_cols].values
            acc = df[acc_cols].values
        
        # Extract standard fields
        mass_cols = [c for c in df.columns if 'mass [' in c]
        dens_cols = [c for c in df.columns if 'dens [' in c]
        pres_cols = [c for c in df.columns if 'pres [' in c]
        ene_cols = [c for c in df.columns if 'ene [' in c]
        sml_cols = [c for c in df.columns if 'sml [' in c]
        
        mass = df[mass_cols[0]].values if mass_cols else None
        dens = df[dens_cols[0]].values if dens_cols else None
        pres = df[pres_cols[0]].values if pres_cols else None
        ene = df[ene_cols[0]].values if ene_cols else None
        sml = df[sml_cols[0]].values if sml_cols else None
        particle_id = df['id'].values if 'id' in df.columns else np.arange(len(df))
        
        # Optional fields
        neighbor = df['neighbor'].values if 'neighbor' in df.columns else None
        alpha = df['alpha'].values if 'alpha' in df.columns else None
        shock = df['shockSensor'].values if 'shockSensor' in df.columns else None
        
        # Additional arrays (if any)
        standard_cols = set(['time', 'pos_x', 'pos_y', 'pos_z', 'vel_x', 'vel_y', 'vel_z',
                            'acc_x', 'acc_y', 'acc_z', 'mass', 'dens', 'pres', 'ene', 
                            'sml', 'id', 'neighbor', 'alpha', 'gradh', 'shockSensor', 'ene_floored'])
        extra_scalars = {}
        extra_vectors = {}
        for col in df.columns:
            base_name = col.split('[')[0].strip()
            if base_name not in standard_cols and '[' in col:
                if any(suffix in col for suffix in ['_x', '_y', '_z']):
                    # Vector field
                    vector_name = base_name.replace('_x', '').replace('_y', '').replace('_z', '')
                    if vector_name not in extra_vectors:
                        if self._dim == 1:
                            extra_vectors[vector_name] = df[[base_name]].values
                        elif self._dim == 2:
                            cols = [c for c in df.columns if vector_name in c and ('_x' in c or '_y' in c)]
                            extra_vectors[vector_name] = df[cols].values
                        else:
                            cols = [c for c in df.columns if vector_name in c and ('_x' in c or '_y' in c or '_z' in c)]
                            extra_vectors[vector_name] = df[cols].values
                else:
                    # Scalar field
                    extra_scalars[base_name] = df[col].values
        
        return ParticleSnapshot(
            time=time,
            num_particles=len(df),
            pos=pos,
            vel=vel,
            acc=acc,
            mass=mass,
            dens=dens,
            pres=pres,
            ene=ene,
            sml=sml,
            particle_id=particle_id,
            neighbor_count=neighbor,
            alpha=alpha,
            shock_sensor=shock,
            units=self._units,  # Attach unit system
            extra_scalars=extra_scalars if extra_scalars else None,
            extra_vectors=extra_vectors if extra_vectors else None
        )
    
    def read_all_snapshots(self) -> List[ParticleSnapshot]:
        """Read all particle snapshots."""
        return [self.read_snapshot(i) for i in range(self.num_snapshots)]
    
    def read_energy_history(self) -> Optional[EnergyHistory]:
        """
        Read energy history file.
        
        Returns:
            EnergyHistory object or None if file doesn't exist
        """
        if self._energy_file is None or not self._energy_file.exists():
            return None
        
        data = np.loadtxt(self._energy_file)
        
        return EnergyHistory(
            time=data[:, 0],
            kinetic=data[:, 1],
            thermal=data[:, 2],
            potential=data[:, 3],
            total=data[:, 4]
        )
