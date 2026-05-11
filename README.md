# VolSurf: Voxel Surface Reconstruction Library

VolSurf is a C++ library for efficient voxel-based surface reconstruction. It provides tools for reading voxel data from CSV files, building hierarchical voxel structures, and generating quad mesh surfaces.

## Features

- Efficient reading and processing of variable-sized voxel blocks from CSV files
- Hierarchical voxel tree construction with pruning optimization
- Quad mesh surface generation from voxel data
- Support for VTK visualization output
- Multi-threaded processing with TBB

## Requirements

### Dependencies

- **VTK (Visualization Toolkit)** - Required for visualization output
- **TBB (Threading Building Blocks)** - Required for multi-threaded operations
- **BLOSC** - Compression library
- **LZ4** - Compression library
- **ZLIB** - Compression library
- **ZSTD** - Compression library

### System Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2019 or later
- C++17 compatible compiler

## Installation

### Building from Source

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd VolSurf
   ```

2. Open the solution file in Visual Studio:
   - `VolSurf.sln` (if exists) or create a new project

3. Build the project:
   - Select the desired configuration (Debug/Release)
   - Build the solution

### Pre-built Binaries

Pre-built binaries are available in the `bin/` and `x64/` directories:
- `bin/Debug/x64/` - Debug binaries
- `bin/Release/x64/` - Release binaries
- `x64/Debug/` and `x64/Release/` - Additional build outputs

## Usage

### Command-Line Interface

The `SurfTest.exe` executable provides a command-line interface:

```bash
SurfTest.exe [options]
```

**Options:**

| Option | Description | Default |
|--------|-------------|---------|
| `-f, --file FILENAME` | CSV filename containing voxel data | VarSurf.csv |
| `-t, --thresh THRESHOLD` | Distance threshold for filtering | -1.0 (disabled) |
| `-p, --prune` | Enable tree pruning | true |
| `-s, --save` | Enable saving tree to file | true |
| `-m, --simplify` | Enable mesh simplification | true |
| `-h, --help` | Show help message | - |

**Example:**

```bash
SurfTest.exe -f data.csv -t 0.5 -p 1 -s 1 -m 1
```

### Input Format

The CSV file should have the following format:

```csv
X,Y,Z,xLen,yLen,zLen,value1,value2
double,double,double,double,double,double,type,type
1.0,2.0,3.0,0.5,0.5,0.5,0.123,label1
...
```

- **X, Y, Z**: Center coordinates of the voxel block
- **xLen, yLen, zLen**: Dimensions of the voxel block
- **Additional columns**: Optional metadata (numeric or string)

### API Usage

```cpp
#include "VoxelSurfaces.h"
#include "VoxelSurfTest.h"

// Read CSV file and generate surface
VoxelSurfTest::ReadCsvVarBlock("input.csv", -1.0, true, true, true);
```

## Project Structure

```
VolSurf/
├── include/              # Header files for external use
│   ├── VoxelSurfVar.h
│   └── VoxelSurfaces.h
├── lib/                  # Library files
│   ├── Debug/x64/
│   └── Release/x64/
├── x64/                  # Build outputs
│   ├── Debug/
│   └── Release/
├── SurfTest.cpp          # Main command-line entry point
├── VoxelSurfTest.cpp     # Core voxel processing logic
├── QuadMesh.cpp          # Quad mesh generation
├── PolyDataToVoxel.cpp   # VTK polydata to voxel conversion
├── FileVTKFunc.cpp       # VTK file operations
├── csv.h                 # CSV parsing utilities
└── VarSurf.csv           # Sample voxel data
```

## Documentation

- `docs/USER_GUIDE.md` - Detailed user guide with API documentation
- `docs/TUTORIAL.md` - Tutorials for typical use cases
- `docs/DEPENDENCIES.md` - Dependency documentation

