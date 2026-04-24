#pragma once

#include "VoxelSurfVar.h"
#include <vector>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>


class MeshConvert
{
public:
	MeshConvert() {};
	~MeshConvert() {};

public:
	static vtkSmartPointer<vtkPolyData> ConvertToVTKPolyData(const std::vector<VARPoint3D>& inVertices,
		const std::vector<std::vector<int>>& inFaces);

	static void ConvertFromVTKPolyData(vtkPolyData* polyData, std::vector<VARPoint3D>& outVertices,
		std::vector<std::vector<int>>& outFaces);

	static bool SimplifyMeshWithQuadMeshSim(const std::vector<VARPoint3D>& inputVertices,
		const std::vector<std::vector<int>>& inputFaces,
		std::vector<VARPoint3D>& outputVertices,
		std::vector<std::vector<int>>& outputFaces,
		double angleThreshold = 5.0);
};

