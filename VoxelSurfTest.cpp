
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
		// 实际应用中可能需要去除空格或引号
		// 例如：token.erase(0, token.find_first_not_of(" \t\""));
		//       token.erase(token.find_last_not_of(" \t\"") + 1);
		tokens.emplace_back(std::move(token)); // 移动而非拷贝
	}
	return tokens;
}

void simple_split_surf(const char* s, char delimiter, std::vector<std::string>& tokens) {
	tokens.clear();
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		// 实际应用中可能需要去除空格或引号
		// 例如：token.erase(0, token.find_first_not_of(" \t\""));
		//       token.erase(token.find_last_not_of(" \t\"") + 1);
		tokens.emplace_back(std::move(token)); // 移动而非拷贝
	}
	return;
}

static bool can_parse_as_long(const char* s) {
	std::istringstream iss(s);
	int n;
	return (iss >> n) && iss.eof();  // 成功读取整数且无剩余字符
}

static bool can_parse_as_double(const char* s) {
	std::istringstream iss(s);
	double d;
	return (iss >> d) && iss.eof();  // 成功读取浮点数且无剩余字符
}

void VoxelSurfTest::ReadCsvVarBlockVTK(std::string filename)
{
	//体素大小：要求该体素大小是所有块段各维度尺寸的倍数
	double voxelSize = 1e10;

	//块段模型定义
	double dOrigin[3], dDest[3];

	//计算范围
	double dXOrigin, dYOrigin, dZOrigin, dXDest, dYDest, dZDest;
	//块段长度
	double dXLen, dYLen, dZLen;

	//左下角点
	dXOrigin = std::numeric_limits<double>::max();
	dYOrigin = std::numeric_limits<double>::max();
	dZOrigin = std::numeric_limits<double>::max();

	//右上角点
	dXDest = std::numeric_limits<double>::min();
	dYDest = std::numeric_limits<double>::min();
	dZDest = std::numeric_limits<double>::min();

	//最小单元块尺寸
	double dbMinSize[3] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

	std::cout << "Reading csv file: " << filename << std::endl;

	//字段信息
	std::vector<std::string> header_fields;
	std::vector<int> header_types;  //字段模型：0-int，1-float，2-string

	try
	{
		io::LineReader line_reader(filename.c_str());

		// 读取第一行作为表头
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

		// 读取第二行判断类型
		char* type_c_str = line_reader.next_line();
		if (type_c_str)
		{
			char delimiter = ',';
			std::vector<std::string> string_fields = simple_split_surf(type_c_str, delimiter);

			//如果浮点数存为0，则应进一步判断是否确实为整数
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
	{//第一次遍历获取块段模型基本信息
		io::CSVReader<6> in(filename);
		//ignore_extra_column：如果具有名称的列在文件中，但不在参数列表中，则会以静默方式忽略该列
		//ignore_missing_column：如果具有名称的列不在文件中，但在参数列表中，则 read_row 不会修改相应的变量
		in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

		double dX, dY, dZ, xLen, yLen, zLen;
		while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
		{
			//取整，避免浮点问题
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

	dXLen = dXDest - dXOrigin;  //x轴上 此块段模型的最大跨度
	dYLen = dYDest - dYOrigin;  //y轴上 此块段模型的最大跨度
	dZLen = dZDest - dZOrigin;  //z轴上 此块段模型的最大跨度

	//按八叉树补全块段模型范围（以左下角为基准,补全右上角）
	double dMaxLevel = std::log2(std::max(std::max(std::round(dXLen / dbMinSize[0]), std::round(dYLen / dbMinSize[1])), std::round(dZLen / dbMinSize[2])));
	int iMaxLevel = std::ceil(dMaxLevel);
	double dXLenCloud = dbMinSize[0] * std::pow(2, iMaxLevel);
	double dYLenCloud = dbMinSize[1] * std::pow(2, iMaxLevel);
	double dZLenCloud = dbMinSize[2] * std::pow(2, iMaxLevel);

	//最小体素尺寸
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

	//字段信息
	std::string defstring;
	for (int i = /*0*/6; i < header_fields.size(); i++)
	{//从第7列开始为字段
		int iType = header_types[i];
		std::string type;
		if (iType == 0)
			type = "int32_t";
		else if (iType == 1)
			type = "float32_t";
		else if (iType == 2)
			type = "varchar24";  //24字符串大小
		else
			continue;
		std::string name = header_fields[i];
		if (!defstring.empty())
			defstring += "; ";
		defstring += type;
		defstring += " ";
		defstring += name;
	}

	// 创建体素文件
	double origin[3] = { dXOrigin, dYOrigin, dZOrigin };
	double size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };
	double rot[3] = { 0.0, 0.0, 0.0 };
	double used_size[3] = { dXLenCloud, dYLenCloud, dZLenCloud };

#ifdef VTKSURFVAR

	// 创建 VTK 数据结构
	vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

	// 存储单元数据（块段大小）
	vtkSmartPointer<vtkFloatArray> cellData = vtkSmartPointer<vtkFloatArray>::New();
	cellData->SetNumberOfComponents(1);
	cellData->SetName("block_value");

	{
		// 第二次读取：处理每个块段
		try
		{
			io::CSVReader<6> in(filename);
			//ignore_extra_column：如果具有名称的列在文件中，但不在参数列表中，则会以静默方式忽略该列
			//ignore_missing_column：如果具有名称的列不在文件中，但在参数列表中，则 read_row 不会修改相应的变量
			in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

			int blockCount = 0;
			double dX, dY, dZ, xLen, yLen, zLen;
			while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
			{
				//取整，避免浮点问题
				xLen = std::round(xLen * 1000.0) / 1000.0;
				yLen = std::round(yLen * 1000.0) / 1000.0;
				zLen = std::round(zLen * 1000.0) / 1000.0;

				// 计算最小角点坐标
				double minX = dX - xLen * 0.5;
				double minY = dY - yLen * 0.5;
				double minZ = dZ - zLen * 0.5;

				// 计算最大角点坐标
				double maxX = minX + xLen;
				double maxY = minY + yLen;
				double maxZ = minZ + zLen;

				// 计算块段的8个顶点坐标（基于最小角点和尺寸）
				// VTK_HEXAHEDRON 顶点顺序: (0,0,0), (1,0,0), (1,1,0), (0,1,0), (0,0,1), (1,0,1), (1,1,1), (0,1,1)
				// 对应到块段的坐标顺序为：[0, 1, 3, 2, 4, 5, 7, 6]
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

				// 添加顶点到points
				vtkIdType pointIds[8];
				for (int i = 0; i < 8; i++)
				{
					pointIds[i] = points->InsertNextPoint(vertices[i]);
				}

				// 创建六面体单元
				vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();

				// 直接按顺序设置，因为 vertices 数组已经满足 VTK_HEXAHEDRON 规范
				for (int i = 0; i < 8; i++)
				{
					hex->GetPointIds()->SetId(i, pointIds[i]);
				}

				cells->InsertNextCell(hex);

				// 存储块段大小
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

	// 组装非结构化网格
	unstructuredGrid->SetPoints(points);
	unstructuredGrid->SetCells(VTK_HEXAHEDRON, cells);
	unstructuredGrid->GetCellData()->SetScalars(cellData);

	// 写入 VTU 文件
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	writer->SetFileName("VarSurfVoxGrid.vtu");
	writer->SetInputData(unstructuredGrid);
	writer->SetDataModeToBinary();
	writer->Write();

#endif
}

void VoxelSurfTest::ReadCsvVarBlock(std::string filename, double dThresh, bool bPrune, bool bSave, bool bSimpl)
{
	//块段模型定义
	double dOrigin[3], dDest[3];

	//计算范围
	double dXOrigin, dYOrigin, dZOrigin, dXDest, dYDest, dZDest;
	//块段长度
	double dXLen, dYLen, dZLen;

	//左下角点
	dXOrigin = std::numeric_limits<double>::max();
	dYOrigin = std::numeric_limits<double>::max();
	dZOrigin = std::numeric_limits<double>::max();

	//右上角点
	dXDest = std::numeric_limits<double>::min();
	dYDest = std::numeric_limits<double>::min();
	dZDest = std::numeric_limits<double>::min();

	//最小单元块尺寸
	double dbMinSize[3] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };

	std::cout << "Reading csv file: " << filename << std::endl;

	//字段信息
	std::vector<std::string> header_fields;
	std::vector<int> header_types;  //字段模型：0-int，1-float，2-string

	try
	{
		io::LineReader line_reader(filename.c_str());

		// 读取第一行作为表头
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

		// 读取第二行判断类型
		char* type_c_str = line_reader.next_line();
		if (type_c_str)
		{
			char delimiter = ',';
			std::vector<std::string> string_fields = simple_split_surf(type_c_str, delimiter);

			//如果浮点数存为0，则应进一步判断是否确实为整数
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
	{//第一次遍历获取块段模型基本信息
		io::CSVReader<6> in(filename);
		//ignore_extra_column：如果具有名称的列在文件中，但不在参数列表中，则会以静默方式忽略该列
		//ignore_missing_column：如果具有名称的列不在文件中，但在参数列表中，则 read_row 不会修改相应的变量
		in.read_header(io::ignore_extra_column, "X", "Y", "Z", "xLen", "yLen", "zLen");

		double dX, dY, dZ, xLen, yLen, zLen;
		while (in.read_row(dX, dY, dZ, xLen, yLen, zLen))
		{
			//取整，避免浮点问题
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

	dXLen = dXDest - dXOrigin;  //x轴上 此块段模型的最大跨度
	dYLen = dYDest - dYOrigin;  //y轴上 此块段模型的最大跨度
	dZLen = dZDest - dZOrigin;  //z轴上 此块段模型的最大跨度

	//按八叉树补全块段模型范围（以左下角为基准,补全右上角）
	double dMaxLevel = std::log2(std::max(std::max(std::round(dXLen / dbMinSize[0]), std::round(dYLen / dbMinSize[1])), std::round(dZLen / dbMinSize[2])));
	int iMaxLevel = std::ceil(dMaxLevel);
	double dXLenCloud = dbMinSize[0] * std::pow(2, iMaxLevel);
	double dYLenCloud = dbMinSize[1] * std::pow(2, iMaxLevel);
	double dZLenCloud = dbMinSize[2] * std::pow(2, iMaxLevel);

	t1 = std::chrono::steady_clock::now();
	std::cout << "Time: " << std::chrono::duration<double>(t1 - t0).count() << "s\n\n";
	t0 = std::chrono::steady_clock::now();

	std::cout << "Reading csv file: second time..." << std::endl;

	//字段信息
	std::string defstring;
	for (int i = /*0*/6; i < header_fields.size(); i++)
	{//从第7列开始为字段
		int iType = header_types[i];
		std::string type;
		if (iType == 0)
			type = "int32_t";
		else if (iType == 1)
			type = "float32_t";
		else if (iType == 2)
			type = "varchar24";  //24字符串大小
		else
			continue;
		std::string name = header_fields[i];
		if (!defstring.empty())
			defstring += "; ";
		defstring += type;
		defstring += " ";
		defstring += name;
	}

	// 创建体素文件
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

	// 构建布尔体积模型
	{
		VARextVoxSurf* pSurface = VARSURFACE_Init("VarSurf.tree");

		std::chrono::steady_clock::time_point t0, t1;
		t0 = std::chrono::steady_clock::now();
		int iI, iJ, iK;
		int iSize[3];
		{
			try
			{	//第二次遍历创建块段模型
				io::LineReader line_reader(filename.c_str());
				// 第一行为表头
				char* header_c_str = line_reader.next_line();

				std::vector<std::string> string_fields;
				double dX[3], xLen, yLen, zLen;
				uint64_t iCsv = 0;
				while (char* line_c_str = line_reader.next_line())
				{
					char delimiter = ',';
					simple_split_surf(line_c_str, delimiter, string_fields);

					//第一列为索引列
					dX[0] = std::stod(string_fields[0]);
					dX[1] = std::stod(string_fields[1]);
					dX[2] = std::stod(string_fields[2]);
					xLen = std::stod(string_fields[3]);
					yLen = std::stod(string_fields[4]);
					zLen = std::stod(string_fields[5]);
					
					if (dThresh >= 0.0)
					{//阈值约束判定（不同约束下的时间判定）
						std::string str = string_fields[6];
						double dStr = std::stod(str);
						if (dStr >= dThresh)
							continue;
					}

					// 将块原点坐标转换为相对于包围盒左下角的偏移，再除以体素大小并向下取整
					iI = static_cast<int>(std::floor((dX[0] - origin[0]) / dbMinSize[0]));
					iJ = static_cast<int>(std::floor((dX[1] - origin[1]) / dbMinSize[1]));
					iK = static_cast<int>(std::floor((dX[2] - origin[2]) / dbMinSize[2]));
					iSize[0] = static_cast<int>(std::floor(xLen / dbMinSize[0]));
					iSize[1] = static_cast<int>(std::floor(yLen / dbMinSize[1]));
					iSize[2] = static_cast<int>(std::floor(zLen / dbMinSize[2]));

					//增加一个可变块
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
		{//需要执行剪枝操作
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

