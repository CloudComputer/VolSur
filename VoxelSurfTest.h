#pragma once

#include <string>


class VoxelSurfTest
{
public:
	VoxelSurfTest() {};
	~VoxelSurfTest() {};

public:
	static void ReadCsvVarBlockVTK(std::string filename);

	static void ReadCsvVarBlock(std::string filename, double dThresh, bool bPrune, bool bSave, bool bSimpl);
};

