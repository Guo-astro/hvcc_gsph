# DISPH Formulation Analysis

## Paper Equation (38) - DISPH Momentum

```
dv_i/dt = -(γ-1) Σ_j U_i*U_j * [f_i^grad * ∇W_ij(h_i)/q_i + f_j^grad * ∇W_ij(h_j)/q_j]
```

Where:
- `U_i = m_i * u_i` (total internal energy of particle i)
- `q_i = ρ_i * u_i` (internal energy DENSITY at particle i)
- `P_i = (γ-1) * q_i` (pressure at particle i)
- `f_i^grad` is the gradh correction factor

## Expand the formula

Term with h_i smoothing:
```
(γ-1) * U_i*U_j/q_i = (γ-1) * (m_i*u_i) * (m_j*u_j) / (ρ_i*u_i)
                     = (γ-1) * m_i * m_j * u_j / ρ_i
                     = m_i * m_j * P_j/(γ-1) / ρ_i
```

Hmm, that gives `m_i * m_j * P_j / ((γ-1)*ρ_i)`, not `m_j * P_j/ρ_i`.

Let me recalculate. If `P_j = (γ-1)*ρ_j*u_j`, then:
```
(γ-1)*m_j*u_j = (γ-1)*m_j*u_j * ρ_j/ρ_j 
               = m_j * P_j / ρ_j
```

So:
```
(γ-1) * U_i*U_j/q_i = (γ-1) * m_i*u_i * m_j*u_j / (ρ_i*u_i)
                     = m_i * (γ-1)*m_j*u_j / ρ_i
                     = m_i * m_j*P_j/ρ_j / ρ_i
```

**This is still not matching!**

Let me try a different approach. What if the formula uses `q` not `ρ*u`? Let me check if `q_i` is calculated differently...

Actually, from the paper equation (42):
```
q_i = Σ_j U_j * W_ij(h_i)
```

So `q_i` is NOT simply `ρ_i * u_i`, it's a **smoothed internal energy density** from neighbors!

This changes everything! The DISPH formulation computes `q` via SPH summation, not as `ρ*u`.

## Correct Understanding

DISPH uses:
1. Smooth density: `ρ_i = Σ_j m_j * W_ij`
2. **Smooth internal energy density**: `q_i = Σ_j U_j * W_ij = Σ_j (m_j*u_j) * W_ij`
3. Specific internal energy: `u_i = q_i / ρ_i` (derived from smoothed quantities)
4. Pressure: `P_i = (γ-1) * q_i`
5. Volume: `V_i = U_i/q_i = m_i*u_i / q_i`

The key insight: `q_i` is smoothed independently, so `u_i = q_i/ρ_i` is NOT the same as the particle's actual `u_i`!

## Implementation Strategy

I need to add a `q` field (internal energy density) to particles and calculate it in PreInteraction:
```cpp
p_i.q = Σ_j (m_j * u_j) * W_ij(h_i)
p_i.ene = p_i.q / p_i.dens  // Update specific internal energy from smoothed q
p_i.pres = (gamma - 1) * p_i.q
p_i.volume = p_i.mass * p_i.ene / p_i.q
```

Then in FluidForce:
```cpp
const real term_i = (gamma-1) * p_i.mass * p_j.mass * p_j.ene / p_i.q * gradh_i;
const real term_j = (gamma-1) * p_i.mass * p_j.mass * p_i.ene / p_j.q * gradh_j;
acc -= dw_i * term_i + dw_j * term_j;
```

This is fundamentally different from my current implementation!
