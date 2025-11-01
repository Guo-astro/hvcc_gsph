"""
Theoretical comparison tools.

Compare simulation results with known analytical solutions:
- Shock tube (Riemann problem)
- Sedov-Taylor blast wave
- Evrard collapse
- Hydrostatic equilibrium
"""

import numpy as np
from typing import Callable, Optional, Tuple
from dataclasses import dataclass
from scipy.optimize import fsolve

# Support both package and script imports
try:
    from .readers import ParticleSnapshot
except ImportError:
    from readers import ParticleSnapshot


@dataclass
class ShockTubeSolution:
    """Analytical solution for shock tube problem."""
    x: np.ndarray
    rho: np.ndarray
    vel: np.ndarray
    pres: np.ndarray
    ene: np.ndarray


class TheoreticalComparison:
    """Tools for comparing with analytical solutions."""
    
    @staticmethod
    def sod_shock_tube(x: np.ndarray, t: float, gamma: float = 1.4) -> ShockTubeSolution:
        """
        Sod shock tube analytical solution.
        
        Args:
            x: Spatial positions to evaluate
            t: Time
            gamma: Adiabatic index
        
        Returns:
            ShockTubeSolution with analytical values
        """
        # Initial conditions (standard Sod problem)
        rho_L, P_L, u_L = 1.0, 1.0, 0.0
        rho_R, P_R, u_R = 0.125, 0.1, 0.0
        
        # Sound speeds
        c_L = np.sqrt(gamma * P_L / rho_L)
        c_R = np.sqrt(gamma * P_R / rho_R)
        
        # Solve for post-shock state (Newton iteration)
        def equations(p):
            """Pressure ratio equation."""
            # Left rarefaction
            A_L = 2 * c_L / (gamma - 1)
            # Right shock
            B = (gamma - 1) / (gamma + 1)
            A_R = 2 / ((gamma + 1) * rho_R)
            C_R = B * P_R
            
            # Velocity match condition
            u_star = A_L * (1 - (p / P_L)**((gamma - 1) / (2 * gamma))) - \
                     np.sqrt(A_R / (p + C_R)) * (p - P_R)
            
            return u_star
        
        # Solve for pressure in star region
        P_star = fsolve(lambda p: equations(p[0]), [0.3])[0]
        
        # Velocity in star region
        u_star = 2 * c_L / (gamma - 1) * (1 - (P_star / P_L)**((gamma - 1) / (2 * gamma)))
        
        # Density in star region
        rho_star_L = rho_L * (P_star / P_L)**(1 / gamma)
        rho_star_R = rho_R * ((P_star / P_R + (gamma - 1) / (gamma + 1)) / \
                              ((gamma - 1) / (gamma + 1) * P_star / P_R + 1))
        
        # Wave speeds
        # Head of rarefaction
        x_head = -c_L * t
        # Tail of rarefaction
        c_star_L = c_L * (P_star / P_L)**((gamma - 1) / (2 * gamma))
        x_tail = (u_star - c_star_L) * t
        # Contact discontinuity
        x_contact = u_star * t
        # Shock
        v_shock = u_R + np.sqrt(gamma * P_R / (2 * rho_R) * \
                                ((gamma + 1) / gamma * P_star / P_R + (gamma - 1) / gamma))
        x_shock = v_shock * t
        
        # Allocate solution arrays
        rho = np.zeros_like(x)
        vel = np.zeros_like(x)
        pres = np.zeros_like(x)
        
        # Fill regions
        for i, xi in enumerate(x):
            if xi < x_head:
                # Left state
                rho[i] = rho_L
                vel[i] = u_L
                pres[i] = P_L
            elif xi < x_tail:
                # Rarefaction fan - self-similar isentropic expansion
                # Characteristic: xi/t = u + c (for left-moving rarefaction, use u-c)
                # For Sod problem with interface at x=0, left rarefaction moves left
                # In the fan: u and c vary smoothly
                vel[i] = 2 / (gamma + 1) * (c_L + xi / t)
                c = c_L - (gamma - 1) / 2 * vel[i]
                rho[i] = rho_L * (c / c_L)**(2 / (gamma - 1))
                pres[i] = P_L * (c / c_L)**(2 * gamma / (gamma - 1))
            elif xi < x_contact:
                # Left star region
                rho[i] = rho_star_L
                vel[i] = u_star
                pres[i] = P_star
            elif xi < x_shock:
                # Right star region
                rho[i] = rho_star_R
                vel[i] = u_star
                pres[i] = P_star
            else:
                # Right state
                rho[i] = rho_R
                vel[i] = u_R
                pres[i] = P_R
        
        # Compute internal energy
        ene = pres / ((gamma - 1) * rho)
        
        return ShockTubeSolution(x=x, rho=rho, vel=vel, pres=pres, ene=ene)
    
    @staticmethod
    def riemann_problem(x: np.ndarray, t: float, 
                       rho_L: float, P_L: float, u_L: float,
                       rho_R: float, P_R: float, u_R: float,
                       gamma: float = 1.4, x_interface: float = 0.0) -> ShockTubeSolution:
        """
        General 1D Riemann problem analytical solution.
        
        Uses exact Riemann solver for ideal gas EOS.
        
        Args:
            x: Spatial positions to evaluate
            t: Time
            rho_L, P_L, u_L: Left state (density, pressure, velocity)
            rho_R, P_R, u_R: Right state (density, pressure, velocity)
            gamma: Adiabatic index
            x_interface: Position of initial discontinuity
        
        Returns:
            ShockTubeSolution with analytical values
        """
        # Shift coordinates to interface at x=0
        x_shifted = x - x_interface
        
        # Sound speeds
        c_L = np.sqrt(gamma * P_L / rho_L)
        c_R = np.sqrt(gamma * P_R / rho_R)
        
        # Solve for post-shock state (Newton iteration)
        def equations(p):
            """Pressure ratio equation."""
            # Left wave (rarefaction or shock)
            if p > P_L:
                # Left shock
                A_L = 2 / ((gamma + 1) * rho_L)
                B_L = (gamma - 1) / (gamma + 1) * P_L
                f_L = (p - P_L) * np.sqrt(A_L / (p + B_L))
            else:
                # Left rarefaction
                f_L = 2 * c_L / (gamma - 1) * ((p / P_L)**((gamma - 1) / (2 * gamma)) - 1)
            
            # Right wave (rarefaction or shock)
            if p > P_R:
                # Right shock
                A_R = 2 / ((gamma + 1) * rho_R)
                B_R = (gamma - 1) / (gamma + 1) * P_R
                f_R = (p - P_R) * np.sqrt(A_R / (p + B_R))
            else:
                # Right rarefaction
                f_R = 2 * c_R / (gamma - 1) * ((p / P_R)**((gamma - 1) / (2 * gamma)) - 1)
            
            # Velocity match condition
            return u_L - u_R + f_L + f_R
        
        # Initial guess for star pressure
        P_guess = max(0.5 * (P_L + P_R), 1e-10)
        P_star = fsolve(equations, [P_guess])[0]
        P_star = max(P_star, 1e-10)  # Ensure positive pressure
        
        # Star region velocity
        if P_star > P_L:
            # Left shock
            A_L = 2 / ((gamma + 1) * rho_L)
            B_L = (gamma - 1) / (gamma + 1) * P_L
            f_L = (P_star - P_L) * np.sqrt(A_L / (P_star + B_L))
        else:
            # Left rarefaction
            f_L = 2 * c_L / (gamma - 1) * ((P_star / P_L)**((gamma - 1) / (2 * gamma)) - 1)
        
        u_star = u_L + f_L
        
        # Star region densities
        if P_star > P_L:
            # Left shock
            rho_star_L = rho_L * ((P_star / P_L + (gamma - 1) / (gamma + 1)) / \
                                  ((gamma - 1) / (gamma + 1) * P_star / P_L + 1))
        else:
            # Left rarefaction
            rho_star_L = rho_L * (P_star / P_L)**(1 / gamma)
        
        if P_star > P_R:
            # Right shock
            rho_star_R = rho_R * ((P_star / P_R + (gamma - 1) / (gamma + 1)) / \
                                  ((gamma - 1) / (gamma + 1) * P_star / P_R + 1))
        else:
            # Right rarefaction
            rho_star_R = rho_R * (P_star / P_R)**(1 / gamma)
        
        # Wave speeds
        if P_star > P_L:
            # Left shock
            v_shock_L = u_L - c_L * np.sqrt((gamma + 1) / (2 * gamma) * P_star / P_L + \
                                            (gamma - 1) / (2 * gamma))
            x_head_L = v_shock_L * t
            x_tail_L = v_shock_L * t
        else:
            # Left rarefaction
            x_head_L = u_L - c_L
            c_star_L = c_L * (P_star / P_L)**((gamma - 1) / (2 * gamma))
            x_tail_L = u_star - c_star_L
            x_head_L *= t
            x_tail_L *= t
        
        if P_star > P_R:
            # Right shock
            v_shock_R = u_R + c_R * np.sqrt((gamma + 1) / (2 * gamma) * P_star / P_R + \
                                            (gamma - 1) / (2 * gamma))
            x_shock_R = v_shock_R * t
        else:
            # Right rarefaction
            x_head_R = u_R + c_R
            c_star_R = c_R * (P_star / P_R)**((gamma - 1) / (2 * gamma))
            x_tail_R = u_star + c_star_R
            x_head_R *= t
            x_tail_R *= t
        
        # Contact discontinuity
        x_contact = u_star * t
        
        # Allocate solution arrays
        rho = np.zeros_like(x_shifted)
        vel = np.zeros_like(x_shifted)
        pres = np.zeros_like(x_shifted)
        
        # Fill regions
        for i, xi in enumerate(x_shifted):
            if P_star > P_L:
                # Left shock
                if xi < x_head_L:
                    rho[i] = rho_L
                    vel[i] = u_L
                    pres[i] = P_L
                elif xi < x_contact:
                    rho[i] = rho_star_L
                    vel[i] = u_star
                    pres[i] = P_star
                else:
                    # Continue to right side
                    pass
            else:
                # Left rarefaction
                if xi < x_head_L:
                    rho[i] = rho_L
                    vel[i] = u_L
                    pres[i] = P_L
                elif xi < x_tail_L:
                    # Rarefaction fan
                    vel[i] = 2 / (gamma + 1) * (c_L + xi / t + u_L)
                    c = c_L - (gamma - 1) / 2 * (vel[i] - u_L)
                    rho[i] = rho_L * (c / c_L)**(2 / (gamma - 1))
                    pres[i] = P_L * (c / c_L)**(2 * gamma / (gamma - 1))
                elif xi < x_contact:
                    rho[i] = rho_star_L
                    vel[i] = u_star
                    pres[i] = P_star
                else:
                    # Continue to right side
                    pass
            
            # Right side
            if (P_star <= P_L and xi >= x_contact) or (P_star > P_L and xi >= x_contact):
                if P_star > P_R:
                    # Right shock
                    if xi < x_shock_R:
                        rho[i] = rho_star_R
                        vel[i] = u_star
                        pres[i] = P_star
                    else:
                        rho[i] = rho_R
                        vel[i] = u_R
                        pres[i] = P_R
                else:
                    # Right rarefaction
                    if xi < x_tail_R:
                        rho[i] = rho_star_R
                        vel[i] = u_star
                        pres[i] = P_star
                    elif xi < x_head_R:
                        # Rarefaction fan
                        vel[i] = 2 / (gamma + 1) * (-c_R + xi / t + u_R)
                        c = c_R + (gamma - 1) / 2 * (vel[i] - u_R)
                        rho[i] = rho_R * (c / c_R)**(2 / (gamma - 1))
                        pres[i] = P_R * (c / c_R)**(2 * gamma / (gamma - 1))
                    else:
                        rho[i] = rho_R
                        vel[i] = u_R
                        pres[i] = P_R
        
        # Compute internal energy
        ene = pres / ((gamma - 1) * rho)
        
        return ShockTubeSolution(x=x, rho=rho, vel=vel, pres=pres, ene=ene)
    
    @staticmethod
    def sedov_taylor(r: np.ndarray, t: float, E0: float, rho0: float, 
                     gamma: float = 1.4, dim: int = 3) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """
        Sedov-Taylor blast wave similarity solution.
        
        Args:
            r: Radial positions
            t: Time
            E0: Total energy of explosion
            rho0: Background density
            gamma: Adiabatic index
            dim: Spatial dimension (2 or 3)
        
        Returns:
            Tuple of (density, velocity, pressure) arrays
        """
        # Similarity variable
        if dim == 2:
            omega = 0  # Planar/cylindrical
            xi_s = 1.033  # Shock position (approximate for gamma=1.4)
        elif dim == 3:
            omega = 0  # Spherical
            xi_s = 1.152  # Shock position (approximate for gamma=1.4)
        else:
            raise ValueError("dim must be 2 or 3")
        
        # Shock radius
        if dim == 2:
            alpha = (E0 / rho0)**(1/4) * t**(1/2)
        else:  # dim == 3
            alpha = (E0 / rho0)**(1/5) * t**(2/5)
        
        R_s = xi_s * alpha
        
        # Similarity solution (simplified)
        rho = np.zeros_like(r)
        vel = np.zeros_like(r)
        pres = np.zeros_like(r)
        
        # Post-shock values (Rankine-Hugoniot)
        rho_s = rho0 * (gamma + 1) / (gamma - 1)
        
        for i, ri in enumerate(r):
            if ri < R_s:
                xi = ri / alpha
                # Simplified self-similar solution (approximate)
                f = (1 - (xi / xi_s)**2)**2
                rho[i] = rho_s * f
                vel[i] = (2 / (gamma + 1)) * ri / t
                pres[i] = rho[i] * (vel[i]**2) / gamma
            else:
                rho[i] = rho0
                vel[i] = 0.0
                pres[i] = 0.0  # Assuming vacuum
        
        return rho, vel, pres
    
    @staticmethod
    def compare_shock_tube(snapshot: ParticleSnapshot, 
                           gamma: float = 1.4,
                           x0: float = 0.0) -> Tuple[ShockTubeSolution, float]:
        """
        Compare snapshot with Sod shock tube solution.
        
        Args:
            snapshot: Particle snapshot (must be 1D)
            gamma: Adiabatic index
            x0: Position of initial discontinuity
        
        Returns:
            Tuple of (analytical solution, L2 error in density)
        """
        if snapshot.dim != 1:
            raise ValueError("Shock tube comparison requires 1D simulation")
        
        # Get particle positions (shifted to have discontinuity at x0)
        x = snapshot.pos[:, 0]
        
        # Compute analytical solution at particle positions
        solution = TheoreticalComparison.sod_shock_tube(x - x0, snapshot.time, gamma)
        
        # Compute L2 error in density
        error = np.sqrt(np.mean((snapshot.dens - solution.rho)**2))
        
        return solution, error
    
    @staticmethod
    def compare_sedov(snapshot: ParticleSnapshot,
                      E0: float,
                      rho0: float,
                      gamma: float = 1.4) -> Tuple[np.ndarray, np.ndarray, np.ndarray, float]:
        """
        Compare snapshot with Sedov-Taylor solution.
        
        Args:
            snapshot: Particle snapshot
            E0: Total explosion energy
            rho0: Background density
            gamma: Adiabatic index
        
        Returns:
            Tuple of (r_theory, rho_theory, sorted simulation, L2 error)
        """
        # Compute radial distance from origin
        if snapshot.dim == 2:
            r_sim = np.sqrt(snapshot.pos[:, 0]**2 + snapshot.pos[:, 1]**2)
        elif snapshot.dim == 3:
            r_sim = np.sqrt(np.sum(snapshot.pos**2, axis=1))
        else:
            raise ValueError("Sedov comparison requires 2D or 3D simulation")
        
        # Create radial bins for comparison
        r_theory = np.linspace(0, r_sim.max(), 200)
        rho_theory, vel_theory, pres_theory = TheoreticalComparison.sedov_taylor(
            r_theory, snapshot.time, E0, rho0, gamma, snapshot.dim
        )
        
        # Sort simulation data by radius for comparison
        idx = np.argsort(r_sim)
        r_sim_sorted = r_sim[idx]
        rho_sim_sorted = snapshot.dens[idx]
        
        # Interpolate theory to simulation points for error calculation
        rho_theory_interp = np.interp(r_sim_sorted, r_theory, rho_theory)
        error = np.sqrt(np.mean((rho_sim_sorted - rho_theory_interp)**2))
        
        return r_theory, rho_theory, (r_sim_sorted, rho_sim_sorted), error
    
    @staticmethod
    def lane_emden_sphere(r: np.ndarray, n: float = 1.5, 
                          rho_c: float = 1.0) -> Tuple[np.ndarray, np.ndarray]:
        """
        Lane-Emden sphere solution (polytropic equilibrium).
        
        Args:
            r: Radial positions
            n: Polytropic index
            rho_c: Central density
        
        Returns:
            Tuple of (density, pressure) arrays
        """
        # This is a simplified version - full solution requires numerical integration
        # of Lane-Emden equation
        
        # Rough approximation for n=1.5
        if abs(n - 1.5) < 0.01:
            # Analytical solution exists for n=1.5
            xi_1 = 3.65375  # First zero of theta(xi)
            R = xi_1  # Stellar radius in dimensionless units
            
            rho = np.zeros_like(r)
            pres = np.zeros_like(r)
            
            for i, ri in enumerate(r):
                if ri < R:
                    xi = ri
                    # Approximate solution
                    theta = np.sin(xi) / xi if xi > 0 else 1.0
                    rho[i] = rho_c * theta**(1/n)
                    K = 1.0  # Polytropic constant (needs proper scaling)
                    pres[i] = K * rho[i]**(1 + 1/n)
                else:
                    rho[i] = 0.0
                    pres[i] = 0.0
            
            return rho, pres
        else:
            raise NotImplementedError(f"Lane-Emden solution for n={n} requires numerical integration")
