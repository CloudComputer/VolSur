#include "FileVTKFunc.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility> // for std::pair
#include <vector>

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkLookupTable.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkTriangleFilter.h>
#include <vtkCellSizeFilter.h>
#include <vtkMath.h>
#include <vtkPLYReader.h>


CFileVTKFunc::CFileVTKFunc()
{}

CFileVTKFunc::~CFileVTKFunc()
{
}

vtkSmartPointer<vtkPolyData> CFileVTKFunc::ReadPLYFile(const std::string& filename)
{
  vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();

  reader->SetFileName(filename.c_str());
  reader->SetGlobalWarningDisplay(0); // 禁用全局警告

  reader->Update();

  vtkPolyData* output = reader->GetOutput();
  if (!output || output->GetNumberOfPoints() == 0)
  {
    std::cerr << "Error: Empty or invalid data in PLY file." << std::endl;
    return nullptr;
  }

  return output;
}

vtkPolyData* CFileVTKFunc::ReadOFFFile(const std::string& filename)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> polygons = vtkSmartPointer<vtkCellArray>::New();

  std::ifstream file(filename.c_str());
  if (!file.is_open())
  {
    std::cerr << "Failed to open: " << filename << std::endl;
    return nullptr;
  }

  std::string line;

  while (std::getline(file, line))
  {
    if (line.empty())
      continue;

    if (line.substr(0, 3) == "OFF" || line[0] == 'O')
    {
      break; //header
    }
  }

  //read
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
      continue;

    std::istringstream iss(line);
    int numVertices, numFaces, numEdges;
    if (iss >> numVertices >> numFaces >> numEdges)
    {
      //points
      for (int i = 0; i < numVertices; ++i)
      {
        std::getline(file, line);
        std::istringstream vStream(line);
        double x, y, z;
        if (vStream >> x >> y >> z)
        {
          points->InsertNextPoint(x, y, z);
        }
      }

      //facets
      for (int i = 0; i < numFaces; ++i)
      {
        std::getline(file, line);
        std::istringstream fStream(line);
        int numPoints;
        if (fStream >> numPoints)
        {
          vtkIdType* ids = new vtkIdType[numPoints];
          for (int j = 0; j < numPoints; ++j)
          {
            int idx;
            if (fStream >> idx)
            {
              ids[j] = idx;
            }
          }
          polygons->InsertNextCell(numPoints, ids);
          delete[] ids;
        }
      }
      break;
    }
  }

  file.close();

  vtkPolyData* polyData = vtkPolyData::New();
  polyData->SetPoints(points);
  polyData->SetPolys(polygons);

  return polyData;
}

void CFileVTKFunc::WritePolyDataToOFF(vtkPolyData* polyData, const std::string& filename)
{
  if (!polyData || !polyData->GetPoints() || !polyData->GetPolys())
  {
    return;
  }

  vtkIdType numPoints = polyData->GetNumberOfPoints();
  vtkCellArray* polys = polyData->GetPolys();
  vtkIdType numPolys = polys->GetNumberOfCells();

  std::ofstream offFile(filename);
  if (!offFile.is_open())
  {
    return;
  }

  //header
  offFile << "OFF\n";
  offFile << numPoints << " " << numPolys << " 0\n";

  //points
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double pt[3];
    polyData->GetPoint(i, pt);
    offFile << pt[0] << " " << pt[1] << " " << pt[2] << "\n";
  }

  //facets
  polys->InitTraversal();
  vtkIdType npts;
  const vtkIdType* pts;
  while (polys->GetNextCell(npts, pts))
  {
    offFile << npts;
    for (int j = 0; j < npts; ++j)
    {
      offFile << " " << pts[j];
    }
    offFile << "\n";
  }

  offFile.close();
}

void CFileVTKFunc::PolyDataColorizerVtp(vtkPolyData* input, const std::string& filename)
{
  if (!input || input->GetNumberOfCells() == 0)
  {
    std::cerr << "Error vtkPolyData" << std::endl;
    return;
  }

  // 配置颜色查找表（彩虹色系）
  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  lookupTable->SetHueRange(0.666, 0.0); // 蓝 -> 红
  lookupTable->SetSaturationRange(1.0, 1.0);
  lookupTable->SetValueRange(1.0, 1.0);
  lookupTable->Build();

  // 步骤1: 三角化网格（确保面积计算准确）
  vtkSmartPointer<vtkTriangleFilter> triangulator = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator->SetInputData(input);
  triangulator->PassVertsOff();
  triangulator->PassLinesOff();
  triangulator->Update();
  vtkPolyData* triangulated = triangulator->GetOutput();

  // 步骤2: 计算单元面积
  vtkSmartPointer<vtkCellSizeFilter> sizeFilter = vtkSmartPointer<vtkCellSizeFilter>::New();
  sizeFilter->SetInputData(triangulated);
  sizeFilter->ComputeAreaOn();
  sizeFilter->ComputeVolumeOff();
  sizeFilter->ComputeLengthOff();
  sizeFilter->ComputeVertexCountOff();
  sizeFilter->Update();
  vtkPolyData* sizedData = sizeFilter->GetPolyDataOutput();

  // 步骤3: 获取面积数组
  vtkDoubleArray* areaArray =
    vtkDoubleArray::SafeDownCast(sizedData->GetCellData()->GetArray("Area"));

  // 步骤4: 计算面积范围（用于颜色映射）
  double areaRange[2];
  areaArray->GetRange(areaRange);
  lookupTable->SetRange(areaRange[0], areaRange[1]);

  // 步骤5: 创建映射器并设置颜色
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(sizedData);
  mapper->SetScalarModeToUseCellData();
  mapper->SetScalarRange(areaRange);
  mapper->SetLookupTable(lookupTable);

  // 步骤6: 写入VTP文件
  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetFileName(filename.c_str());
  writer->SetInputData(sizedData);
  writer->SetDataModeToBinary();
  writer->EncodeAppendedDataOff(); // 提高兼容性
  writer->Write();
}