"""
Conservation analysis tools.

Check conservation of:
- Total mass
- Linear momentum
- Angular momentum
- Total energy
"""

import numpy as np
from typing import List, Dict, Optional
from dataclasses import dataclass

# Support both package and script imports
try:
    from .readers import ParticleSnapshot, EnergyHistory
except ImportError:
    from readers import ParticleSnapshot, EnergyHistory


@dataclass
class ConservationReport:
    """Report of conservation quantities over time."""
    time: np.ndarray
    
    # Mass conservation
    total_mass: np.ndarray
    mass_error: np.ndarray  # Relative to initial
    
    # Momentum conservation
    momentum: np.ndarray  # Shape: (N_times, DIM)
    momentum_magnitude: np.ndarray
    momentum_error: np.ndarray  # Relative to initial magnitude
    
    # Energy conservation
    kinetic_energy: np.ndarray
    thermal_energy: np.ndarray
    total_energy: np.ndarray
    energy_error: np.ndarray  # Relative to initial
    
    # Angular momentum conservation (2D/3D only)
    angular_momentum: Optional[np.ndarray] = None  # Shape: (N_times,) for 2D or (N_times, 3) for 3D
    angular_momentum_error: Optional[np.ndarray] = None
    
    def summary(self) -> Dict[str, float]:
        """
        Get summary statistics of conservation.
        
        Returns:
            Dictionary with max absolute errors
        """
        return {
            'max_mass_error': np.max(np.abs(self.mass_error)),
            'max_momentum_error': np.max(np.abs(self.momentum_error)),
            'max_angular_momentum_error': np.max(np.abs(self.angular_momentum_error)) if self.angular_momentum_error is not None else 0.0,
            'max_energy_error': np.max(np.abs(self.energy_error)),
            'final_mass_error': self.mass_error[-1],
            'final_momentum_error': self.momentum_error[-1],
            'final_energy_error': self.energy_error[-1],
        }
    
    def print_summary(self):
        """Print conservation summary."""
        summary = self.summary()
        print("=" * 60)
        print("CONSERVATION ANALYSIS SUMMARY")
        print("=" * 60)
        print(f"Max mass error:              {summary['max_mass_error']:.6e}")
        print(f"Max momentum error:          {summary['max_momentum_error']:.6e}")
        if self.angular_momentum_error is not None:
            print(f"Max angular momentum error:  {summary['max_angular_momentum_error']:.6e}")
        print(f"Max energy error:            {summary['max_energy_error']:.6e}")
        print()
        print(f"Final mass error:            {summary['final_mass_error']:.6e}")
        print(f"Final momentum error:        {summary['final_momentum_error']:.6e}")
        print(f"Final energy error:          {summary['final_energy_error']:.6e}")
        print("=" * 60)


class ConservationAnalyzer:
    """Analyze conservation properties of SPH simulations."""
    
    @staticmethod
    def compute_angular_momentum_2d(snapshot: ParticleSnapshot, 
                                     com_pos: Optional[np.ndarray] = None,
                                     com_vel: Optional[np.ndarray] = None) -> float:
        """
        Compute angular momentum in 2D (scalar, z-component).
        
        Args:
            snapshot: Particle snapshot
            com_pos: Center of mass position (if None, computed)
            com_vel: Center of mass velocity (if None, computed)
        
        Returns:
            Angular momentum (z-component)
        """
        if snapshot.dim != 2:
            raise ValueError("This method is for 2D simulations only")
        
        if com_pos is None:
            com_pos = snapshot.center_of_mass()
        if com_vel is None:
            com_vel = snapshot.center_of_mass_velocity()
        
        r = snapshot.pos - com_pos
        v = snapshot.vel - com_vel
        
        # L_z = sum_i m_i (x_i * v_y_i - y_i * v_x_i)
        L_z = np.sum(snapshot.mass * (r[:, 0] * v[:, 1] - r[:, 1] * v[:, 0]))
        
        return L_z
    
    @staticmethod
    def compute_angular_momentum_3d(snapshot: ParticleSnapshot,
                                     com_pos: Optional[np.ndarray] = None,
                                     com_vel: Optional[np.ndarray] = None) -> np.ndarray:
        """
        Compute angular momentum in 3D (vector).
        
        Args:
            snapshot: Particle snapshot
            com_pos: Center of mass position (if None, computed)
            com_vel: Center of mass velocity (if None, computed)
        
        Returns:
            Angular momentum vector
        """
        if snapshot.dim != 3:
            raise ValueError("This method is for 3D simulations only")
        
        if com_pos is None:
            com_pos = snapshot.center_of_mass()
        if com_vel is None:
            com_vel = snapshot.center_of_mass_velocity()
        
        r = snapshot.pos - com_pos
        v = snapshot.vel - com_vel
        
        # L = sum_i m_i (r_i x v_i)
        L = np.sum(snapshot.mass[:, np.newaxis] * np.cross(r, v), axis=0)
        
        return L
    
    @staticmethod
    def analyze_snapshots(snapshots: List[ParticleSnapshot]) -> ConservationReport:
        """
        Analyze conservation over all snapshots.
        
        Args:
            snapshots: List of particle snapshots
        
        Returns:
            ConservationReport with time evolution of conserved quantities
        """
        n_times = len(snapshots)
        dim = snapshots[0].dim
        
        # Initialize arrays
        time = np.zeros(n_times)
        total_mass = np.zeros(n_times)
        momentum = np.zeros((n_times, dim))
        kinetic = np.zeros(n_times)
        thermal = np.zeros(n_times)
        total_energy = np.zeros(n_times)
        
        # Angular momentum (dimension-dependent)
        if dim == 1:
            angular_momentum = None
        elif dim == 2:
            angular_momentum = np.zeros(n_times)
        else:  # dim == 3
            angular_momentum = np.zeros((n_times, 3))
        
        # Compute quantities for each snapshot
        for i, snap in enumerate(snapshots):
            time[i] = snap.time
            total_mass[i] = snap.total_mass()
            momentum[i] = snap.total_momentum()
            kinetic[i] = snap.total_kinetic_energy()
            thermal[i] = snap.total_thermal_energy()
            total_energy[i] = kinetic[i] + thermal[i]
            
            if dim == 2:
                angular_momentum[i] = ConservationAnalyzer.compute_angular_momentum_2d(snap)
            elif dim == 3:
                angular_momentum[i] = ConservationAnalyzer.compute_angular_momentum_3d(snap)
        
        # Compute errors relative to initial values
        mass_error = (total_mass - total_mass[0]) / total_mass[0]
        
        momentum_mag = np.linalg.norm(momentum, axis=1)
        momentum_mag_0 = momentum_mag[0]
        if momentum_mag_0 > 1e-14:  # Only compute if initial momentum is non-zero
            momentum_error = (momentum_mag - momentum_mag_0) / momentum_mag_0
        else:
            momentum_error = momentum_mag  # Absolute error if initially at rest
        
        if angular_momentum is not None:
            if dim == 2:
                L_0 = angular_momentum[0]
                if abs(L_0) > 1e-14:
                    angular_momentum_error = (angular_momentum - L_0) / abs(L_0)
                else:
                    angular_momentum_error = angular_momentum
            else:  # dim == 3
                L_mag = np.linalg.norm(angular_momentum, axis=1)
                L_mag_0 = L_mag[0]
                if L_mag_0 > 1e-14:
                    angular_momentum_error = (L_mag - L_mag_0) / L_mag_0
                else:
                    angular_momentum_error = L_mag
        else:
            angular_momentum_error = None
        
        energy_error = (total_energy - total_energy[0]) / total_energy[0]
        
        return ConservationReport(
            time=time,
            total_mass=total_mass,
            mass_error=mass_error,
            momentum=momentum,
            momentum_magnitude=momentum_mag,
            momentum_error=momentum_error,
            angular_momentum=angular_momentum,
            angular_momentum_error=angular_momentum_error,
            kinetic_energy=kinetic,
            thermal_energy=thermal,
            total_energy=total_energy,
            energy_error=energy_error
        )
    
    @staticmethod
    def check_energy_from_file(energy_history: EnergyHistory, 
                               tolerance: float = 1e-3,
                               verbose: bool = True) -> bool:
        """
        Check energy conservation from energy.txt file.
        
        Args:
            energy_history: Energy history data
            tolerance: Maximum acceptable relative error
            verbose: Print results
        
        Returns:
            True if energy is conserved within tolerance
        """
        error = energy_history.relative_error()
        max_error = np.max(np.abs(error))
        
        conserved = max_error < tolerance
        
        if verbose:
            print("=" * 60)
            print("ENERGY CONSERVATION CHECK")
            print("=" * 60)
            print(f"Maximum relative error: {max_error:.6e}")
            print(f"Tolerance:              {tolerance:.6e}")
            print(f"Status:                 {'PASS ✓' if conserved else 'FAIL ✗'}")
            print("=" * 60)
        
        return conserved
