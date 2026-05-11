
#include "QuadMesh.h"
#include "MeshConvert.h"

#include <unordered_set>
#include <functional>
#include <unordered_map>
#include <limits>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <string>


bool QuadMesh::GenerateQuadMesh(bool bSimpl, double voxelSize[3], const char* filename,
    const VARQuadFaceInfo* quads, const size_t quad_count)
{
	if (!quads || quad_count == 0)
	{
		return false;
	}

	std::chrono::steady_clock::time_point t0, t1;
	t0 = std::chrono::steady_clock::now();

	// Store vertices and face indices
	std::vector<VARPoint3D> vertices;
	std::vector<std::vector<int>> faces;
	vertices.reserve(quad_count * 2);
	faces.reserve(quad_count);

	// Map for vertex deduplication
	std::unordered_map<VARVoxPointKey, uint64_t, VARVoxPointKeyHash> vertexMap;
	vertexMap.reserve(quad_count * 2);
	uint64_t vertexCount = 0;

	VARextvoxel_addr local;

	// Process each quad face
	for (size_t i = 0; i < quad_count; ++i)
	{
		const VARQuadFaceInfo& quadInfo = quads[i];
		local.x = quadInfo.addr.x;
		local.y = quadInfo.addr.y;
		local.z = quadInfo.addr.z;

		// Calculate voxel world coordinate bounds
		VARPoint3D voxelMin(
			voxelSize[0] * quadInfo.addr.x + 0.0,  //Assume origin at (0,0,0)
			voxelSize[1] * quadInfo.addr.y + 0.0,
			voxelSize[2] * quadInfo.addr.z + 0.0
		);

		// Generate four vertices based on face direction
		std::vector<VARPoint3D> faceVertices(4);
		std::vector<VARVoxPointKey> key(4);

		// Define vertex configuration for six face directions (relative to voxel min corner)
		switch (quadInfo.ifacedir)
		{
		case 0: // +X face
			faceVertices[0] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z);
			faceVertices[1] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z);
			faceVertices[2] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			faceVertices[3] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z + voxelSize[2]);
			key[0] = VARVoxPointKey(local.x + 1, local.y, local.z);
			key[1] = VARVoxPointKey(local.x + 1, local.y + 1, local.z);
			key[2] = VARVoxPointKey(local.x + 1, local.y + 1, local.z + 1);
			key[3] = VARVoxPointKey(local.x + 1, local.y, local.z + 1);
			break;
		case 1: // -X face
			faceVertices[0] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z + voxelSize[2]);
			faceVertices[1] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			faceVertices[2] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z);
			faceVertices[3] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z);
			key[0] = VARVoxPointKey(local.x, local.y, local.z + 1);
			key[1] = VARVoxPointKey(local.x, local.y + 1, local.z + 1);
			key[2] = VARVoxPointKey(local.x, local.y + 1, local.z);
			key[3] = VARVoxPointKey(local.x, local.y, local.z);
			break;
		case 2: // +Y face
			faceVertices[0] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			faceVertices[1] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			faceVertices[2] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z);
			faceVertices[3] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z);
			key[0] = VARVoxPointKey(local.x, local.y + 1, local.z + 1);
			key[1] = VARVoxPointKey(local.x + 1, local.y + 1, local.z + 1);
			key[2] = VARVoxPointKey(local.x + 1, local.y + 1, local.z);
			key[3] = VARVoxPointKey(local.x, local.y + 1, local.z);
			break;
		case 3: // -Y face
			faceVertices[0] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z);
			faceVertices[1] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z);
			faceVertices[2] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z + voxelSize[2]);
			faceVertices[3] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z + voxelSize[2]);
			key[0] = VARVoxPointKey(local.x, local.y, local.z);
			key[1] = VARVoxPointKey(local.x + 1, local.y, local.z);
			key[2] = VARVoxPointKey(local.x + 1, local.y, local.z + 1);
			key[3] = VARVoxPointKey(local.x, local.y, local.z + 1);
			break;
		case 4: // +Z face
			faceVertices[0] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z + voxelSize[2]);
			faceVertices[1] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z + voxelSize[2]);
			faceVertices[2] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			faceVertices[3] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z + voxelSize[2]);
			key[0] = VARVoxPointKey(local.x, local.y, local.z + 1);
			key[1] = VARVoxPointKey(local.x + 1, local.y, local.z + 1);
			key[2] = VARVoxPointKey(local.x + 1, local.y + 1, local.z + 1);
			key[3] = VARVoxPointKey(local.x, local.y + 1, local.z + 1);
			break;
		case 5: // -Z face
			faceVertices[0] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y, voxelMin.z);
			faceVertices[1] = VARPoint3D(voxelMin.x, voxelMin.y, voxelMin.z);
			faceVertices[2] = VARPoint3D(voxelMin.x, voxelMin.y + voxelSize[1], voxelMin.z);
			faceVertices[3] = VARPoint3D(voxelMin.x + voxelSize[0], voxelMin.y + voxelSize[1], voxelMin.z);
			key[0] = VARVoxPointKey(local.x + 1, local.y, local.z);
			key[1] = VARVoxPointKey(local.x, local.y, local.z);
			key[2] = VARVoxPointKey(local.x, local.y + 1, local.z);
			key[3] = VARVoxPointKey(local.x + 1, local.y + 1, local.z);
			break;
		default:
			continue; // Invalid face direction
		}

		// Build indices for the four vertices of current face
		std::vector<int> faceIndices(4);

		for (int j = 0; j < 4; ++j)
		{
			const VARPoint3D& v = faceVertices[j];
			const VARVoxPointKey& voxkey = key[j];

			// Check if vertex already exists
			auto it = vertexMap.find(voxkey);
			if (it != vertexMap.end())
			{
				// Use existing vertex index
				faceIndices[j] = it->second;
			}
			else
			{
				// Add new vertex
				vertices.push_back(v);
				vertexMap[voxkey] = vertexCount;
				faceIndices[j] = vertexCount;
				vertexCount++;
			}
		}

		faces.push_back(faceIndices);
	}

	t1 = std::chrono::steady_clock::now();
	std::cout << "QuadMesh Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";

	std::cout << "Gene Quads: " << faces.size() << std::endl;
	std::cout << "Gene Verts: " << vertices.size() << std::endl;

	if (bSimpl)
	{
		t0 = std::chrono::steady_clock::now();

		std::vector<VARPoint3D> outputVertices;
		std::vector<std::vector<int>> outputFaces;
		bool success = MeshConvert::SimplifyMeshWithQuadMeshSim(vertices, faces, outputVertices, outputFaces);
		if (!success) return false;

		t1 = std::chrono::steady_clock::now();
		std::cout << "SimpMesh Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";

		double reductionFaces = 100.0 * (1.0 - static_cast<double>(outputFaces.size()) / faces.size());
		double reductionVerts = 100.0 * (1.0 - static_cast<double>(outputVertices.size()) / vertices.size());

		faces = outputFaces;
		vertices = outputVertices;
		
		std::cout << "Simp Quads: " << faces.size() << std::endl;
		std::cout << "Simp Verts: " << vertices.size() << std::endl;
		
		std::cout << "Reduction Quads(%): " << reductionFaces << std::endl;
		std::cout << "Reduction Verts(%): " << reductionVerts << std::endl;
	}

	{
		std::cout << "Writing file... " << std::endl;

		t0 = std::chrono::steady_clock::now();

		// ply file
		std::ofstream file(filename, std::ios::binary);
		if (!file.is_open())
		{
			return false;
		}

		// file header
		file << "ply\n";
		file << "format ascii 1.0\n";
		file << "comment Generated from voxel surface\n";
		file << "element vertex " << vertices.size() << "\n";
		file << "property float x\n";
		file << "property float y\n";
		file << "property float z\n";
		file << "element face " << faces.size() << "\n";
		file << "property list uchar int vertex_indices\n";
		file << "end_header\n";

		// vertices
		for (const auto& vertex : vertices)
		{
			file << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
		}

		// faces
		for (const auto& face : faces)
		{
			file << "4 ";
			for (int index : face)
			{
				file << index << " ";
			}
			file << "\n";
		}

		file.close();

		t1 = std::chrono::steady_clock::now();
		std::cout << "Writing Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
	}

    return true;
}
