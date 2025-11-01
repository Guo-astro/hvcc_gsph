"""
Binary SPH output reader for Python.

Reads .sph binary files produced by the SPH simulation code.
Binary format is ~5-10x more compact than CSV and much faster to load.

File format:
- Header (80 bytes):
  - magic_number (uint32): 0x53504801 ("SPH\x01")
  - version (uint32): format version (currently 1)
  - dimension (uint32): spatial dimension (1, 2, or 3)
  - particle_count (uint32): number of particles
  - time (float64): simulation time
  - length_factor (float64): unit conversion for length
  - time_factor (float64): unit conversion for time
  - mass_factor (float64): unit conversion for mass
  - length_unit (16 bytes): unit name string (e.g., "m", "kpc")
  - time_unit (16 bytes): unit name string (e.g., "s", "Myr")
  - mass_unit (16 bytes): unit name string (e.g., "kg", "M_sun")
- Particle data (per particle):
  - position: DIM × float64
  - velocity: DIM × float64
  - acceleration: DIM × float64
  - mass: float64
  - density: float64
  - pressure: float64
  - energy: float64
  - smoothing_length: float64
  - alpha: float64
  - gradh: float64
  - shock_sensor: float64
  - id: int32
  - neighbor_count: int32
  - energy_floored: int32
"""

import numpy as np
import struct
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass

try:
    from .units import UnitSystem, UnitFactory
except ImportError:
    from units import UnitSystem, UnitFactory


@dataclass
class BinarySnapshotHeader:
    """Header information from binary snapshot file"""
    magic_number: int
    version: int
    dimension: int
    particle_count: int
    time: float
    length_factor: float
    time_factor: float
    mass_factor: float
    length_unit: str
    time_unit: str
    mass_unit: str


class BinarySnapshotReader:
    """Read binary SPH snapshot files (.sph format)"""
    
    MAGIC_NUMBER = 0x53504801  # "SPH\x01"
    HEADER_SIZE = 80  # bytes
    
    def __init__(self, filename: str):
        """
        Initialize reader for a binary snapshot file.
        
        Args:
            filename: Path to .sph binary file
        """
        self.filename = Path(filename)
        if not self.filename.exists():
            raise FileNotFoundError(f"Binary snapshot not found: {filename}")
    
    def read(self) -> Tuple[BinarySnapshotHeader, Dict[str, np.ndarray]]:
        """
        Read binary snapshot and return header + particle data.
        
        Returns:
            Tuple of (header, data_dict) where data_dict contains:
            - 'time': simulation time
            - 'pos': positions (N, DIM)
            - 'vel': velocities (N, DIM)
            - 'acc': accelerations (N, DIM)
            - 'mass': masses (N,)
            - 'dens': densities (N,)
            - 'pres': pressures (N,)
            - 'ene': specific energies (N,)
            - 'sml': smoothing lengths (N,)
            - 'alpha': artificial viscosity (N,)
            - 'gradh': grad-h term (N,)
            - 'shock_sensor': shock sensor (N,)
            - 'id': particle IDs (N,)
            - 'neighbor': neighbor counts (N,)
            - 'ene_floored': energy floor flag (N,)
        """
        with open(self.filename, 'rb') as f:
            # Read header
            header = self._read_header(f)
            
            # Validate header
            if header.magic_number != self.MAGIC_NUMBER:
                raise ValueError(f"Invalid magic number: {hex(header.magic_number)}")
            if header.version != 1:
                raise ValueError(f"Unsupported version: {header.version}")
            
            # Read particle data
            data = self._read_particles(f, header)
            
        return header, data
    
    def _read_header(self, f) -> BinarySnapshotHeader:
        """Read header from binary file"""
        # Read fixed-size header fields
        magic = struct.unpack('I', f.read(4))[0]
        version = struct.unpack('I', f.read(4))[0]
        dimension = struct.unpack('I', f.read(4))[0]
        particle_count = struct.unpack('I', f.read(4))[0]
        time = struct.unpack('d', f.read(8))[0]
        
        # Read unit factors
        length_factor = struct.unpack('d', f.read(8))[0]
        time_factor = struct.unpack('d', f.read(8))[0]
        mass_factor = struct.unpack('d', f.read(8))[0]
        
        # Read unit name strings (16 bytes each, null-terminated)
        length_unit = f.read(16).rstrip(b'\x00').decode('ascii')
        time_unit = f.read(16).rstrip(b'\x00').decode('ascii')
        mass_unit = f.read(16).rstrip(b'\x00').decode('ascii')
        
        return BinarySnapshotHeader(
            magic_number=magic,
            version=version,
            dimension=dimension,
            particle_count=particle_count,
            time=time,
            length_factor=length_factor,
            time_factor=time_factor,
            mass_factor=mass_factor,
            length_unit=length_unit,
            time_unit=time_unit,
            mass_unit=mass_unit
        )
    
    def _read_particles(self, f, header: BinarySnapshotHeader) -> Dict[str, np.ndarray]:
        """Read particle data from binary file"""
        n = header.particle_count
        dim = header.dimension
        
        # Calculate bytes per particle
        # pos (dim*8) + vel (dim*8) + acc (dim*8) + 8 scalars (8*8) + 3 ints (3*4)
        bytes_per_particle = dim * 8 * 3 + 8 * 8 + 3 * 4
        
        # Read all particle data at once
        particle_bytes = f.read(bytes_per_particle * n)
        
        # Parse using numpy (much faster than struct)
        offset = 0
        data = {}
        
        # Helper to read array
        def read_array(dtype, shape):
            nonlocal offset
            size = np.prod(shape) * np.dtype(dtype).itemsize
            arr = np.frombuffer(particle_bytes[offset:offset+size], dtype=dtype).reshape(shape)
            offset += size
            return arr.copy()  # Copy to own memory
        
        # Read position, velocity, acceleration
        data['pos'] = read_array('f8', (n, dim))
        data['vel'] = read_array('f8', (n, dim))
        data['acc'] = read_array('f8', (n, dim))
        
        # Read scalar fields
        data['mass'] = read_array('f8', (n,))
        data['dens'] = read_array('f8', (n,))
        data['pres'] = read_array('f8', (n,))
        data['ene'] = read_array('f8', (n,))
        data['sml'] = read_array('f8', (n,))
        data['alpha'] = read_array('f8', (n,))
        data['gradh'] = read_array('f8', (n,))
        data['shock_sensor'] = read_array('f8', (n,))
        
        # Read integer fields
        data['id'] = read_array('i4', (n,))
        data['neighbor'] = read_array('i4', (n,))
        data['ene_floored'] = read_array('i4', (n,))
        
        # Add time
        data['time'] = header.time
        
        return data
    
    def get_unit_system(self) -> UnitSystem:
        """Get unit system from binary snapshot header"""
        header, _ = self.read()
        
        # Construct unit system from header
        return UnitFactory.create_custom(
            length_unit=header.length_unit,
            time_unit=header.time_unit,
            mass_unit=header.mass_unit,
            length_factor=header.length_factor,
            time_factor=header.time_factor,
            mass_factor=header.mass_factor
        )


class BinarySimulationReader:
    """
    Read a series of binary snapshots from a directory.
    
    Similar interface to SimulationReader but for binary files.
    """
    
    def __init__(self, directory: str):
        """
        Initialize reader for a directory of binary snapshots.
        
        Args:
            directory: Path to directory containing .sph files
        """
        self.directory = Path(directory)
        if not self.directory.exists():
            raise FileNotFoundError(f"Directory not found: {directory}")
        
        # Find all .sph files
        self.snapshot_files = sorted(self.directory.glob("*.sph"))
        if not self.snapshot_files:
            raise ValueError(f"No .sph files found in {directory}")
        
        # Read first snapshot to get metadata
        reader = BinarySnapshotReader(self.snapshot_files[0])
        header, _ = reader.read()
        
        self.dimension = header.dimension
        self.units = reader.get_unit_system()
        self.num_snapshots = len(self.snapshot_files)
    
    def read_snapshot(self, index: int) -> Dict[str, np.ndarray]:
        """
        Read snapshot by index.
        
        Args:
            index: Snapshot index (0 to num_snapshots-1)
            
        Returns:
            Dictionary of particle data arrays
        """
        if index < 0 or index >= self.num_snapshots:
            raise IndexError(f"Snapshot index {index} out of range [0, {self.num_snapshots})")
        
        reader = BinarySnapshotReader(self.snapshot_files[index])
        header, data = reader.read()
        return data
    
    def read_all(self) -> List[Dict[str, np.ndarray]]:
        """Read all snapshots"""
        return [self.read_snapshot(i) for i in range(self.num_snapshots)]
    
    def get_times(self) -> np.ndarray:
        """Get array of all snapshot times"""
        times = []
        for snapshot_file in self.snapshot_files:
            reader = BinarySnapshotReader(snapshot_file)
            header, _ = reader.read()
            times.append(header.time)
        return np.array(times)
    
    def __len__(self):
        return self.num_snapshots
    
    def __repr__(self):
        return (f"BinarySimulationReader(directory='{self.directory}', "
                f"num_snapshots={self.num_snapshots}, "
                f"dimension={self.dimension}D, "
                f"units={self.units})")


# Convenience function
def read_binary_snapshot(filename: str) -> Tuple[BinarySnapshotHeader, Dict[str, np.ndarray]]:
    """
    Convenience function to read a single binary snapshot.
    
    Args:
        filename: Path to .sph file
        
    Returns:
        Tuple of (header, data)
        
    Example:
        >>> header, data = read_binary_snapshot("00000.sph")
        >>> print(f"Time: {header.time} {header.time_unit}")
        >>> print(f"Positions shape: {data['pos'].shape}")
        >>> print(f"Density range: [{data['dens'].min():.3f}, {data['dens'].max():.3f}]")
    """
    reader = BinarySnapshotReader(filename)
    return reader.read()


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python binary_reader.py <snapshot.sph>")
        print("   or: python binary_reader.py <directory>")
        sys.exit(1)
    
    path = Path(sys.argv[1])
    
    if path.is_file():
        # Read single file
        print(f"Reading binary snapshot: {path}")
        header, data = read_binary_snapshot(path)
        
        print(f"\nHeader:")
        print(f"  Version: {header.version}")
        print(f"  Dimension: {header.dimension}D")
        print(f"  Particles: {header.particle_count}")
        print(f"  Time: {header.time:.6f} {header.time_unit}")
        print(f"  Units: [{header.length_unit}, {header.time_unit}, {header.mass_unit}]")
        
        print(f"\nData arrays:")
        for key, arr in data.items():
            if isinstance(arr, np.ndarray):
                print(f"  {key:15s}: shape={str(arr.shape):12s} dtype={arr.dtype}")
            else:
                print(f"  {key:15s}: {arr}")
    
    elif path.is_dir():
        # Read directory of snapshots
        print(f"Reading binary simulation: {path}")
        reader = BinarySimulationReader(path)
        
        print(f"\nSimulation info:")
        print(f"  Snapshots: {len(reader)}")
        print(f"  Dimension: {reader.dimension}D")
        print(f"  Units: {reader.units}")
        
        times = reader.get_times()
        print(f"\nTime range: {times[0]:.6f} to {times[-1]:.6f} {reader.units.time_unit}")
        
        # Read first and last snapshot
        print(f"\nFirst snapshot:")
        data0 = reader.read_snapshot(0)
        print(f"  Particles: {len(data0['mass'])}")
        print(f"  Density range: [{data0['dens'].min():.3e}, {data0['dens'].max():.3e}]")
        
        print(f"\nLast snapshot:")
        data_last = reader.read_snapshot(len(reader) - 1)
        print(f"  Particles: {len(data_last['mass'])}")
        print(f"  Density range: [{data_last['dens'].min():.3e}, {data_last['dens'].max():.3e}]")
    
    else:
        print(f"ERROR: Not a file or directory: {path}")
        sys.exit(1)
