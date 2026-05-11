
#include "VoxelSurfTest.h"

#include <limits>
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>

//FastVoxel
#include "VoxelSurfVar.h"
#include "QuadMesh.h"

//THIRD
#include "csv.h"

#include <cstdio> // for remove() and rename()
#include <chrono>
#include <thread>

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkHexahedron.h>
#include <vtkXMLUnstructuredGridWriter.h>


static std::vector<std::string> simple_split_surf(const char* s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		// In practice, you may need to remove spaces or quotes
		// e.g., token.erase(0, token.find_first_not_of(" \t\""));
		//       token.erase(token.find_last_not_of(" \t\"") + 1);
		tokens.emplace_back(std::move(token)); // Move instead of copy
	}
	return tokens;
}

void simple_split_surf(const char* s, char delimiter, std::vector<std::string>& tokens) {
	tokens.clear();
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		// In practice, you may need to remove spaces or quotes
		// e.g., token.erase(0, token.find_first_not_of(" \t\""));
		//       token.erase(token.find_last_not_of(" \t\"") + 1);
		tokens.emplace_back(std::move(token)); // Move instead of copy
	}
	return;
}

static bool can_parse_as_long(const char* s) {
	std::istringstream iss(s);
	int n;
	return (iss >> n) && iss.eof();  // Successfully reads integer with no remaining characters
}

static bool can_parse_as_double(const char* s) {
	std::istringstream iss(s);
	double d;
	return (iss >> d) && iss.eof();  // Successfully reads double with no remaining characters
}

void VoxelSurfTest::ReadCsvVarBlockVTK(std::string filename)
{
	// Voxel size: must be a multiple of all block dimensions
	double voxelSize = 1e10;

	// Block model definitions
	double dOrigin[3], dDest[3];

	// Calculation range
	double dXOrigin, dYOrigin, dZOrigin, dXDest, dYDest, dZDest;
	// Block lengths
	double dXLen, dYLen, dZLen;

	// Bottom-left corner
	dXOrigin = std::numeric_limits<double>::max();
	dYOrigin = std::numeric_limits<double>::max();
	dZOrigin = std::numeric_limits<double>::max();

	// Top-right corner
	dXDest = std::numeric_limits<double>::min();
	dYDest = std::numeric_limits<double>::min();
	dZDest = std::numeric_limits<double>::min();

	// Minimum unit block size
	double dbMinSize[3] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

	std::cout << "Reading csv file: " << filename << std::endl;

	// Field information
	std::vector<std::string> header_fields;
	std::vector<int> header_types;  // Field types: 0-int, 1-float, 2-string

	try
	{
		io::LineReader line_reader(filename.c_str());

		// Read first line as header
		char* header_c_str = line_reader.next_line();
		if (header_c_str)
		{
			char delimiter = ',';
			header_fields = simple_split_surf(header_c_str, delimiter);
		}

		if (header_fields.size() <= 6)
		{
			std::cout << "File Header error." << std::endl;
			return;
		}

		// Read second line to determine types
		char* type_c_str = line_reader.next_line();
		if (type_c_str)
		{
			char delimiter = ',';
			std::vector<std::string> string_fields = simple_split_surf(type_c_str, delimiter);

			// If float stored as 0, should further check if it's actually an integer
			for (const auto& field : string_fields) {
				if (can_parse_as_long(field.c_str())) {
					header_types.push_back(0);
				}
				else if (can_parse_as_double(field.c_str())) {
					header_types.push_back(1);
				}
				else {
					header_types.push_back(2);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	std::chrono::steady_clock::time_point t0, t1;
	t0 = std::chrono::steady_clock::now();

	std::cout << "Reading csv file: first time..." << std::endl;

	try
	{// First pass to get block model basic information
		io::CSVReader<6> in(filename);
		//ignore_extra_column: If a named column exists in the file but not in the parameter list, it will be silently ignored
		//ignore_missing_column: If a named column does not exist in the file but is in the parameter list, read_row will not modify the corresponding variable
		in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

		double dX, dY, dZ, xLen, yLen, zLen;
		while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
		{
			// Round to avoid floating point issues
			xLen = std::round(xLen * 1000.0) / 1000.0;
			yLen = std::round(yLen * 1000.0) / 1000.0;
			zLen = std::round(zLen * 1000.0) / 1000.0;

			if (dbMinSize[0] > xLen)  dbMinSize[0] = xLen;
			if (dbMinSize[1] > yLen)  dbMinSize[1] = yLen;
			if (dbMinSize[2] > zLen)  dbMinSize[2] = zLen;

			dOrigin[0] = dX - xLen / 2.0;
			if (dOrigin[0] < dXOrigin)
			{
				dXOrigin = dOrigin[0];
			}

			dDest[0] = dX + xLen / 2.0;
			if (dDest[0] > dXDest)
			{
				dXDest = dDest[0];
			}

			dOrigin[1] = dY - yLen / 2.0;
			if (dOrigin[1] < dYOrigin)
			{
				dYOrigin = dOrigin[1];
			}

			dDest[1] = dY + yLen / 2.0;
			if (dDest[1] > dYDest)
			{
				dYDest = dDest[1];
			}

			dOrigin[2] = dZ - zLen / 2.0;
			if (dOrigin[2] < dZOrigin)
			{
				dZOrigin = dOrigin[2];
			}

			dDest[2] = dZ + zLen / 2.0;
			if (dDest[2] > dZDest)
			{
				dZDest = dDest[2];
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	dXLen = dXDest - dXOrigin;  // Maximum span of block model on x-axis
	dYLen = dYDest - dYOrigin;  // Maximum span of block model on y-axis
	dZLen = dZDest - dZOrigin;  // Maximum span of block model on z-axis

	// Complete block model range using octree (with bottom-left corner as reference, complete top-right)
	double dMaxLevel = std::log2(std::max(std::max(std::round(dXLen / dbMinSize[0]), std::round(dYLen / dbMinSize[1])), std::round(dZLen / dbMinSize[2])));
	int iMaxLevel = std::ceil(dMaxLevel);
	double dXLenCloud = dbMinSize[0] * std::pow(2, iMaxLevel);
	double dYLenCloud = dbMinSize[1] * std::pow(2, iMaxLevel);
	double dZLenCloud = dbMinSize[2] * std::pow(2, iMaxLevel);

	// Minimum voxel size
	if (voxelSize > dbMinSize[0])
		voxelSize = dbMinSize[0];
	if (voxelSize > dbMinSize[1])
		voxelSize = dbMinSize[1];
	if (voxelSize > dbMinSize[2])
		voxelSize = dbMinSize[2];

	std::cout << "Minimum Voxel Size: " << voxelSize << std::endl;

	t1 = std::chrono::steady_clock::now();
	std::cout << "Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
	t0 = std::chrono::steady_clock::now();

	std::cout << "Reading csv file: second time..." << std::endl;

	// Field information
	std::string defstring;
	for (int i = /*0*/6; i < header_fields.size(); i++)
	{// Fields start from column 7
		int iType = header_types[i];
		std::string type;
		if (iType == 0)
			type = "int32_t";
		else if (iType == 1)
			type = "float32_t";
		else if (iType == 2)
			type = "varchar24";  // Max 24 characters
		else
			continue;
		std::string name = header_fields[i];
		if (!defstring.empty())
			defstring += "; ";
		defstring += type;
		defstring += " ";
		defstring += name;
	}

	// Create voxel file
	double origin[3] = { dXOrigin, dYOrigin, dZOrigin };
	double size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };
	double rot[3] = { 0.0, 0.0, 0.0 };
	double used_size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };

#ifdef VTKSURFVAR

	// Create VTK data structure
	vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

	// Store cell data (block size)
	vtkSmartPointer<vtkFloatArray> cellData = vtkSmartPointer<vtkFloatArray>::New();
	cellData->SetNumberOfComponents(1);
	cellData->SetName("block_value");

	{
		// Second read: process each block
		try
		{
			io::CSVReader<6> in(filename);
			//ignore_extra_column: If a named column exists in the file but not in the parameter list, it will be silently ignored
			//ignore_missing_column: If a named column does not exist in the file but is in the parameter list, read_row will not modify the corresponding variable
			in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

			int blockCount = 0;
			double dX, dY, dZ, xLen, yLen, zLen;
			while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
			{
				// Round to avoid floating point issues
				xLen = std::round(xLen * 1000.0) / 1000.0;
				yLen = std::round(yLen * 1000.0) / 1000.0;
				zLen = std::round(zLen * 1000.0) / 1000.0;

				// Calculate minimum corner coordinates
				double minX = dX - xLen * 0.5;
				double minY = dY - yLen * 0.5;
				double minZ = dZ - zLen * 0.5;

				// Calculate maximum corner coordinates
				double maxX = minX + xLen;
				double maxY = minY + yLen;
				double maxZ = minZ + zLen;

				// Calculate 8 vertex coordinates of block (based on min corner and size)
				// VTK_HEXAHEDRON vertex order: (0,0,0), (1,0,0), (1,1,0), (0,1,0), (0,0,1), (1,0,1), (1,1,1), (0,1,1)
				// Corresponding block coordinate order: [0, 1, 3, 2, 4, 5, 7, 6]
				double vertices[8][3] = {
					{minX, minY, minZ},  // 0: (0,0,0)
					{maxX, minY, minZ},  // 1: (1,0,0)
					{maxX, maxY, minZ},  // 2: (1,1,0)
					{minX, maxY, minZ},  // 3: (0,1,0)
					{minX, minY, maxZ},  // 4: (0,0,1)
					{maxX, minY, maxZ},  // 5: (1,0,1)
					{maxX, maxY, maxZ},  // 6: (1,1,1)
					{minX, maxY, maxZ}   // 7: (0,1,1)
				};

				// Add vertices to points
				vtkIdType pointIds[8];
				for (int i = 0; i < 8; i++)
				{
					pointIds[i] = points->InsertNextPoint(vertices[i]);
				}

				// Create hexahedral cell
				vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

				// Set points directly in order since vertices array already satisfies VTK_HEXAHEDRON specification
				for (int i = 0; i < 8; i++)
				{
					hex->GetPointIds()->SetId(i, pointIds[i]);
				}

				cells->InsertNextCell(hex);

				// Store block size
				int iLevel = log2(std::round(dXLenCloud / xLen));
				cellData->InsertNextValue(/*iLevel*/xLen * yLen * zLen);

				blockCount++;
			}

			std::cout << "Total blocks processed: " << blockCount << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error during second read: " << e.what() << std::endl;
			return;
		}
	}

	std::cout << "Created " << points->GetNumberOfPoints() << " vertices" << std::endl;
	std::cout << "Created " << cells->GetNumberOfCells() << " hexahedral cells" << std::endl;

	// Assemble unstructured grid
	unstructuredGrid->SetPoints(points);
	unstructuredGrid->SetCells(VTK_HEXAHEDRON, cells);
	unstructuredGrid->GetCellData()->SetScalars(cellData);

	// Write VTU file
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	writer->SetFileName("VarSurfVoxGrid.vtu");
	writer->SetInputData(unstructuredGrid);
	writer->SetDataModeToBinary();
	writer->Write();

#endif
}

void VoxelSurfTest::ReadCsvVarBlock(std::string filename, double dThresh, bool bPrune, bool bSave, bool bSimpl)
{
	// Block model definitions
	double dOrigin[3], dDest[3];

	// Calculation range
	double dXOrigin, dYOrigin, dZOrigin, dXDest, dYDest, dZDest;
	// Block lengths
	double dXLen, dYLen, dZLen;

	// Bottom-left corner
	dXOrigin = std::numeric_limits<double>::max();
	dYOrigin = std::numeric_limits<double>::max();
	dZOrigin = std::numeric_limits<double>::max();

	// Top-right corner
	dXDest = std::numeric_limits<double>::min();
	dYDest = std::numeric_limits<double>::min();
	dZDest = std::numeric_limits<double>::min();

	// Minimum unit block size
	double dbMinSize[3] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

	std::cout << "Reading csv file: " << filename << std::endl;

	// Field information
	std::vector<std::string> header_fields;
	std::vector<int> header_types;  // Field types: 0-int, 1-float, 2-string

	try
	{
		io::LineReader line_reader(filename.c_str());

		// Read first line as header
		char* header_c_str = line_reader.next_line();
		if (header_c_str)
		{
			char delimiter = ',';
			header_fields = simple_split_surf(header_c_str, delimiter);
		}

		if (header_fields.size() <= 6)
		{
			std::cout << "File Header error." << std::endl;
			return;
		}

		// Read second line to determine types
		char* type_c_str = line_reader.next_line();
		if (type_c_str)
		{
			char delimiter = ',';
			std::vector<std::string> string_fields = simple_split_surf(type_c_str, delimiter);

			// If float stored as 0, should further check if it's actually an integer
			for (const auto& field : string_fields) {
				if (can_parse_as_long(field.c_str())) {
					header_types.push_back(0);
				}
				else if (can_parse_as_double(field.c_str())) {
					header_types.push_back(1);
				}
				else {
					header_types.push_back(2);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	std::chrono::steady_clock::time_point t0, t1;
	t0 = std::chrono::steady_clock::now();

	std::cout << "Reading csv file: first time..." << std::endl;

	try
	{// First pass to get block model basic information
		io::CSVReader<6> in(filename);
		//ignore_extra_column: If a named column exists in the file but not in the parameter list, it will be silently ignored
		//ignore_missing_column: If a named column does not exist in the file but is in the parameter list, read_row will not modify the corresponding variable
		in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

		double dX, dY, dZ, xLen, yLen, zLen;
		while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
		{
			// Round to avoid floating point issues
			xLen = std::round(xLen*1000.0)/1000.0;
			yLen = std::round(yLen*1000.0)/1000.0;
			zLen = std::round(zLen*1000.0)/1000.0;

			if (dbMinSize[0] > xLen)  dbMinSize[0] = xLen;
			if (dbMinSize[1] > yLen)  dbMinSize[1] = yLen;
			if (dbMinSize[2] > zLen)  dbMinSize[2] = zLen;

			dOrigin[0] = dX - xLen / 2.0;
			if (dOrigin[0] < dXOrigin)
			{
				dXOrigin = dOrigin[0];
			}

			dDest[0] = dX + xLen / 2.0;
			if (dDest[0] > dXDest)
			{
				dXDest = dDest[0];
			}

			dOrigin[1] = dY - yLen / 2.0;
			if (dOrigin[1] < dYOrigin)
			{
				dYOrigin = dOrigin[1];
			}

			dDest[1] = dY + yLen / 2.0;
			if (dDest[1] > dYDest)
			{
				dYDest = dDest[1];
			}

			dOrigin[2] = dZ - zLen / 2.0;
			if (dOrigin[2] < dZOrigin)
			{
				dZOrigin = dOrigin[2];
			}

			dDest[2] = dZ + zLen / 2.0;
			if (dDest[2] > dZDest)
			{
				dZDest = dDest[2];
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return;
	}

	dXLen = dXDest - dXOrigin;  // Maximum span of block model on x-axis
	dYLen = dYDest - dYOrigin;  // Maximum span of block model on y-axis
	dZLen = dZDest - dZOrigin;  // Maximum span of block model on z-axis

	// Complete block model range using octree (with bottom-left corner as reference, complete top-right)
	double dMaxLevel = std::log2(std::max(std::max(std::round(dXLen / dbMinSize[0]), std::round(dYLen / dbMinSize[1])), std::round(dZLen / dbMinSize[2])));
	int iMaxLevel = std::ceil(dMaxLevel);
	double dXLenCloud = dbMinSize[0] * std::pow(2, iMaxLevel);
	double dYLenCloud = dbMinSize[1] * std::pow(2, iMaxLevel);
	double dZLenCloud = dbMinSize[2] * std::pow(2, iMaxLevel);

	t1 = std::chrono::steady_clock::now();
	std::cout << "Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
	t0 = std::chrono::steady_clock::now();

	std::cout << "Reading csv file: second time..." << std::endl;

	// Field information
	std::string defstring;
	for (int i = /*0*/6; i < header_fields.size(); i++)
	{// Fields start from column 7
		int iType = header_types[i];
		std::string type;
		if (iType == 0)
			type = "int32_t";
		else if (iType == 1)
			type = "float32_t";
		else if (iType == 2)
			type = "varchar24";  // Max 24 characters
		else
			continue;
		std::string name = header_fields[i];
		if (!defstring.empty())
			defstring += "; ";
		defstring += type;
		defstring += " ";
		defstring += name;
	}

	// Create voxel file
	double origin[3] = { dXOrigin, dYOrigin, dZOrigin };
	double size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };
	double rot[3] = { 0.0, 0.0, 0.0 };
	double used_size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };
	double dvox[3] = { (dXLen / dbMinSize[0]), (dYLen / dbMinSize[1]), (dZLen / dbMinSize[2]) };

	std::cout << "BBox Min Point: " << std::setw(10) << origin[0] << ", " << origin[1] << ", " << origin[2] << std::endl;
	std::cout << "BBox Length: " << dXLen << ", " << dYLen << ", " << dZLen << std::endl;
	std::cout << "Minimum Voxel Size: " << dbMinSize[0] << ", " << dbMinSize[1] << ", " << dbMinSize[2] << std::endl;
	std::cout << "Voxel XYZ Number: " << dvox[0] << ", " << dvox[1] << ", " << dvox[2] << std::endl;
	std::cout << "Voxel Total Number: " << static_cast<size_t>(dvox[0] * dvox[1] * dvox[2]) << std::endl << std::endl;

	size_t quad_count = 0;

	// Build boolean volume model
	{
		VARextVoxSurf* pSurface = VARSURFACE_Init("VarSurf.tree");

		std::chrono::steady_clock::time_point t0, t1;
		t0 = std::chrono::steady_clock::now();
		int iI, iJ, iK;
		int iSize[3];
		{
			try
			{	// Second pass to create block model
				io::LineReader line_reader(filename.c_str());
				// First line is header
				char* header_c_str = line_reader.next_line();

				std::vector<std::string> string_fields;
				double dX[3], xLen, yLen, zLen;
				uint64_t iCsv = 0;
				while (char* line_c_str = line_reader.next_line())
				{
					char delimiter = ',';
					simple_split_surf(line_c_str, delimiter, string_fields);

					// First column is index
					dX[0] = std::stod(string_fields[0]);
					dX[1] = std::stod(string_fields[1]);
					dX[2] = std::stod(string_fields[2]);
					xLen = std::stod(string_fields[3]);
					yLen = std::stod(string_fields[4]);
					zLen = std::stod(string_fields[5]);
					
					if (dThresh >= 0.0)
					{// Threshold constraint check (time check under different constraints)
						std::string str = string_fields[6];
						double dStr = std::stod(str);
						if (dStr >= dThresh)
							continue;
					}

					// Convert block origin coordinates to offset relative to bounding box bottom-left, then divide by voxel size and floor
					iI = static_cast<int>(std::floor((dX[0] - origin[0]) / dbMinSize[0]));
					iJ = static_cast<int>(std::floor((dX[1] - origin[1]) / dbMinSize[1]));
					iK = static_cast<int>(std::floor((dX[2] - origin[2]) / dbMinSize[2]));
					iSize[0] = static_cast<int>(std::floor(xLen / dbMinSize[0]));
					iSize[1] = static_cast<int>(std::floor(yLen / dbMinSize[1]));
					iSize[2] = static_cast<int>(std::floor(zLen / dbMinSize[2]));

					// Add a variable block
					VARSURFACE_Insert(pSurface, VARextvoxel_addr(iI, iJ, iK, iSize));

					++iCsv;
				}

				std::cout << "Total Blocks: " << iCsv << std::endl;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error: " << e.what() << std::endl;
				return;
			}
		}

		t1 = std::chrono::steady_clock::now();
		std::cout << "ConsTree Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";

		VARSURFACE_TreeInfo(pSurface);
		if (bPrune)
		{// Need to execute pruning operation
			t0 = std::chrono::steady_clock::now();
			VARSURFACE_Prune(pSurface);
			t1 = std::chrono::steady_clock::now();
			std::cout << "TreePrune Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
			VARSURFACE_TreeInfo(pSurface);
		}

		if (bSave)
		{
			t0 = std::chrono::steady_clock::now();
			VARSURFACE_Save(pSurface);
			t1 = std::chrono::steady_clock::now();
			std::cout << "SaveTree Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
		}

		t0 = std::chrono::steady_clock::now();
		VARQuadFaceInfo* quads = nullptr;
		bool result = VARSURFACE_BuildConsSurOpt(pSurface, &quads, &quad_count);
		t1 = std::chrono::steady_clock::now();
		std::cout << "BuildSurface Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
		
		bool plyResult = QuadMesh::GenerateQuadMesh(bSimpl, dbMinSize, "VarSurf.ply", quads, quad_count);

		//bool plyResult = VARVOXEL_ConsSurPlyFile(iMaxLevel, "VarSurf.ply", quads, quad_count);
		
		VARSURFACE_FreeConSurOptData(quads);
		VARSURFACE_Release(&pSurface);
	}
}

