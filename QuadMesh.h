#pragma once

#include "VoxelSurfVar.h"
#include <vector>

class QuadMesh
{
public:
	QuadMesh() {};
	~QuadMesh() {};

public:
	static bool GenerateQuadMesh(bool bSimpl, double voxelSize[3], const char* filename,
		const VARQuadFaceInfo* quads, const size_t quad_count);
};

