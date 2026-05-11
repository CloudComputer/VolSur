# Data Format Documentation

## Overview

This document describes the data formats used by VolSurf for input and output.

## Input Format

### CSV File Structure

VolSurf reads voxel data from CSV files with a specific structure.

#### Header Row

The first row must contain column names:

| Column | Required | Description |
|--------|----------|-------------|
| X | Yes | X coordinate of voxel block center |
| Y | Yes | Y coordinate of voxel block center |
| Z | Yes | Z coordinate of voxel block center |
| xLen | Yes | Width of voxel block along X axis |
| yLen | Yes | Width of voxel block along Y axis |
| zLen | Yes | Width of voxel block along Z axis |
| [additional] | No | Optional metadata columns |

#### Type Row

The second row specifies the data type for each column:

| Type Indicator | Description |
|----------------|-------------|
| `int32_t` or any integer | Integer column |
| `float32_t`, `double`, or any decimal | Floating-point column |
| Any other string | String column (max 24 characters) |

#### Data Rows

Subsequent rows contain the actual voxel data.

### Example CSV

```csv
X,Y,Z,xLen,yLen,zLen,distance,intensity,label
double,double,double,double,double,double,double,double,varchar24
0.5,0.5,0.5,1.0,1.0,1.0,0.0,0.85,block_001
1.5,0.5,0.5,1.0,1.0,1.0,0.3,0.92,block_002
0.5,1.5,0.5,1.0,1.0,1.0,0.2,0.78,block_003
0.5,0.5,1.5,1.0,1.0,1.0,0.5,0.65,block_004
```

