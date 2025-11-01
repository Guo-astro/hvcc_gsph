# Output Format Quick Reference

## Configuration

```json
{
  "outputFormat": "binary",        // "csv", "binary", "numpy", "hdf5"
  "outputDirectory": "auto",       // Auto-generate organized path
  "sampleName": "shock_tube",      // For auto directory naming
  "SPHType": "disph"               // For auto directory naming
}
```

## Auto Directory Structure

`visualizations/{sample}/{method}/{dim}D/{format}/`

**Examples:**
- `visualizations/shock_tube/DISPH/1D/csv/`
- `visualizations/shock_tube/DISPH/1D/binary/`
- `visualizations/sedov_taylor/GSPH/3D/binary/`

## Format Comparison

| Format | Size | Speed | Use When |
|--------|------|-------|----------|
| CSV | 1x | 1x | Debugging, small datasets |
| **Binary** | **9x smaller** | **20x faster** | **Production, large data** |
| NumPy | 5x smaller | 10x faster | Python-only |

## Python Usage

### CSV
```python
from analysis.readers import SimulationReader

reader = SimulationReader("visualizations/shock_tube/DISPH/1D/csv")
snapshot = reader.read_snapshot(0)
print(snapshot.pos, snapshot.dens)
```

### Binary (Recommended)
```python
from analysis.binary_reader import BinarySimulationReader

reader = BinarySimulationReader("visualizations/shock_tube/DISPH/1D/binary")
data = reader.read_snapshot(0)
print(data['pos'], data['dens'])
print(f"Units: {reader.units}")
```

### Single File
```python
from analysis.binary_reader import read_binary_snapshot

header, data = read_binary_snapshot("00000.sph")
print(f"Time: {header.time} {header.time_unit}")
print(f"Particles: {header.particle_count}")
```

## Common Tasks

### Check Binary File
```bash
python3 analysis/binary_reader.py visualizations/shock_tube/DISPH/1D/binary/00000.sph
```

### Check Directory
```bash
python3 analysis/binary_reader.py visualizations/shock_tube/DISPH/1D/binary/
```

### Estimate Size
```python
# 50k particles, 100 snapshots, 3D
csv_mb = (50000 * 100 * 100) / 1e6      # ~500 MB
binary_mb = (50000 * 100 * 100) / 1e6  # ~50 MB
```

## Recommended Settings

**Small runs (<10k particles):**
```json
{"outputFormat": "csv", "outputDirectory": "auto"}
```

**Production (>10k particles):**
```json
{"outputFormat": "binary", "outputDirectory": "auto"}
```

**Python-only:**
```json
{"outputFormat": "numpy", "outputDirectory": "auto"}
```
