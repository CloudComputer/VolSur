#pragma once

#include "BaseSim.h"

#include <vtkAutoInit.h>
// VTK_MODULE_INIT(vtkRenderingOpenGL2); //OpenGL
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCleanPolyData.h>
#include <vtkColor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkPLYWriter.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolygon.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSTLWriter.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTimerLog.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
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

// 自定义哈希函数
struct PairHash
{
  size_t operator()(const std::pair<vtkIdType, vtkIdType>& key) const
  {
    std::stringstream stream;
    stream << key.first << "-" << key.second;
    std::string str;
    stream >> str;
    size_t hash = std::hash<std::string>()(str);
    return hash;
  }
};

struct PairEqual
{
  bool operator()(
    const std::pair<vtkIdType, vtkIdType>& a, const std::pair<vtkIdType, vtkIdType>& b) const
  {
    return (a.first == b.first && a.second == b.second);
  }
};

using Edge = std::pair<vtkIdType, vtkIdType>;
using EdgeFaceMap = std::map<Edge, std::vector<vtkIdType>>;
//using EdgeFaceMap = std::unordered_map<Edge, std::vector<vtkIdType>, PairHash, PairEqual>;

class CQuadMeshSim : public CBaseSim
{
public:
  CQuadMeshSim()
    : angleThreshold(5.0)
  {
    m_maxCellId = 0;
    bOptimize = false;
  }

  void SetAngleThreshold(double degrees) { angleThreshold = degrees; }

  void SetOptimize(bool b) { bOptimize = b; }

  double GetAngleThreshold() const { return angleThreshold; }

  vtkSmartPointer<vtkPolyData> Simplify(vtkPolyData* input)
  {
    vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
    if (!input)
    {
      vtkGenericWarningMacro("Input or output is null!");
      return nullptr;
    }

    vtkSmartPointer<vtkCellArray> inputPolys = input->GetPolys();
    vtkIdType numCells = input->GetNumberOfCells();
    if (numCells == 0)
    {
      return nullptr;
    }

    // Step 1: // 计算法向量和邻接关系
    std::vector<std::vector<int>> adjFaces(numCells);
    std::vector<std::vector<vtkIdType>> cellPoints(numCells);
    std::vector<Vector3> normals(numCells);
    ComputeNormalsAndAdjacency(input, adjFaces, cellPoints, normals);

    // Step 2: Find coplanar regions using BFS
    std::vector<bool> visited(numCells, false);
    std::vector<bool> merged(numCells, false);
    m_maxCellId = numCells;
    double cosThreshold = cos(vtkMath::RadiansFromDegrees(angleThreshold));
    vtkSmartPointer<vtkCellArray> outputPolys = vtkSmartPointer<vtkCellArray>::New();

    std::vector<std::vector<vtkIdType>> regions;
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      if (visited[i])
        continue;
      std::vector<vtkIdType> region =
        FindCoplanarRegion(i, adjFaces, normals, visited, cosThreshold);
      regions.emplace_back(std::move(region));
    }

    for (int i = 0; i < regions.size(); ++i)
    {
      if (regions[i].size() > 1)
      {
        // Merge region
        MergeRegion(input, outputPolys, merged, regions[i], cellPoints);
      }
    }

    // Step 3: 添加未处理的四边形
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      if (merged[cellId])
        continue;
      input->GetCellPoints(cellId, cellIds);
      outputPolys->InsertNextCell(cellIds);
    }

    // Build output
    output->SetPoints(input->GetPoints());
    output->SetPolys(outputPolys);

    // 清理孤立点
    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
    cleaner->SetInputData(output);
    cleaner->PointMergingOn();
    cleaner->Update();
    return cleaner->GetOutput();
  }

private:
  vtkIdType m_maxCellId;

  struct Vector3
  {
    double x, y, z;
    Vector3()
      : x(0)
      , y(0)
      , z(0)
    {
    }
    Vector3(double x_, double y_, double z_)
      : x(x_)
      , y(y_)
      , z(z_)
    {
    }

    void Normalize()
    {
      double len = sqrt(x * x + y * y + z * z);
      if (len == 0)
        return;
      x /= len;
      y /= len;
      z /= len;
    }

    double Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
  };

  void ComputeNormalsAndAdjacency(vtkPolyData* input, std::vector<std::vector<int>>& adjFaces,
    std::vector<std::vector<vtkIdType>>& cellPoints, std::vector<Vector3>& normals)
  {
    vtkCellArray* polys = input->GetPolys();
    vtkPoints* points = input->GetPoints();
    vtkIdType numCells = input->GetNumberOfCells();

    // Edge map: (minId, maxId) -> cellIds sharing this edge
    std::map<std::pair<vtkIdType, vtkIdType>, std::vector<vtkIdType>> edgeMap;

    polys->InitTraversal();
    vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();

    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      polys->GetNextCell(ids);
      if (ids->GetNumberOfIds() != 4)
        continue;

      std::vector<vtkIdType> pts(4);
      for (int i = 0; i < 4; ++i)
      {
        pts[i] = ids->GetId(i);
      }
      cellPoints[cellId] = pts;

      // Compute normal
      normals[cellId] = ComputeQuadNormal(points, pts);

      // Add edges to edgeMap // 添加边到边映射
      for (int j = 0; j < 4; ++j)
      {
        vtkIdType a = pts[j];
        vtkIdType b = pts[(j + 1) % 4];
        auto edge = std::make_pair(std::min(a, b), std::max(a, b));
        edgeMap[edge].push_back(cellId);
      }
    }

    // Build adjacency from edgeMap
    for (const auto& entry : edgeMap)
    {
      const std::vector<vtkIdType>& cellIds = entry.second;
      if (cellIds.size() == 2)
      { // Internal edge
        vtkIdType id1 = cellIds[0];
        vtkIdType id2 = cellIds[1];
        adjFaces[id1].push_back(id2);
        adjFaces[id2].push_back(id1);
      }
    }
  }

  Vector3 ComputeQuadNormal(vtkPoints* points, const std::vector<vtkIdType>& quadPoints)
  {
    double p0[3], p1[3], p2[3];
    points->GetPoint(quadPoints[0], p0);
    points->GetPoint(quadPoints[1], p1);
    points->GetPoint(quadPoints[2], p2);
    // points->GetPoint(quadPoints[3], p3);

    // Triangles: p0-p1-p2 and p0-p2-p3
    Vector3 v01(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);
    Vector3 v02(p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]);
    // Vector3 v03(p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2]);

    Vector3 n1 = Cross(v01, v02);
    // Vector3 n2 = Cross(v02, v03);
    // Vector3 n = Vector3(n1.x + n2.x, n1.y + n2.y, n1.z + n2.z);
    n1.Normalize();
    return n1;
  }

  Vector3 Cross(const Vector3& a, const Vector3& b)
  {
    return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
  }

  std::vector<vtkIdType> FindCoplanarRegion(vtkIdType start,
    const std::vector<std::vector<int>>& adjFaces, const std::vector<Vector3>& normals,
    std::vector<bool>& visited, double cosThreshold)
  {
    std::vector<vtkIdType> region;
    region.reserve(8);
    std::queue<vtkIdType> q;
    q.push(start);
    visited[start] = true;

    while (!q.empty())
    {
      vtkIdType cellId = q.front();
      q.pop();
      region.push_back(cellId);

      for (int neighborId : adjFaces[cellId])
      {
        if (visited[neighborId])
          continue;
        double dot = fabs(normals[cellId].Dot(normals[neighborId]));
        if (dot >= cosThreshold)
        {
          visited[neighborId] = true;
          q.push(neighborId);
        }
      }
    }
    return region;
  }

  /**
   * @brief 查找并返回单元中与'targetPoint'相邻且不是'otherSharedPoint'的点。
   * * @param cell 定义单元的点ID列表（必须是有序的，例如逆时针）。
   * @param targetPoint 我们要查找其邻居的点。
   * @param otherSharedPoint 需要排除的相邻点（共享边上的另一个点）。
   * @return vtkIdType 相邻的唯一（外围）点。
   */
  vtkIdType findUniqueAdjacentPoint(
    const std::vector<vtkIdType>& cell, vtkIdType targetPoint, vtkIdType otherSharedPoint)
  {
    for (size_t i = 0; i < cell.size(); ++i)
    {
      if (cell[i] == targetPoint)
      {
        // 检查前一个点（处理循环边界）
        vtkIdType prevPoint = cell[(i + cell.size() - 1) % cell.size()];
        if (prevPoint != otherSharedPoint)
        {
          return prevPoint;
        }

        // 检查后一个点（处理循环边界）
        vtkIdType nextPoint = cell[(i + 1) % cell.size()];
        if (nextPoint != otherSharedPoint)
        {
          return nextPoint;
        }
      }
    }
    // 如果找不到，这是一个逻辑错误，表明输入单元有问题
    throw std::runtime_error("Could not find unique adjacent point. Check cell connectivity.");
  }

  // vtkIdType findUniqueAdjacentPointOpt(
  //   const std::vector<vtkIdType>& cell, vtkIdType targetPoint, vtkIdType otherSharedPoint)
  //{
  //   for (size_t i = 0; i < cell.size(); ++i)
  //   {
  //     if (cell[i] == targetPoint)
  //     {
  //       // 检查前一个点（处理循环边界）
  //       vtkIdType prevPoint = cell[(i + cell.size() - 1) % cell.size()];
  //       // 检查后一个点（处理循环边界）
  //       vtkIdType nextPoint = cell[(i + 1) % cell.size()];

  //      if (prevPoint == otherSharedPoint)
  //      {
  //        return nextPoint;
  //      }

  //      if (nextPoint == otherSharedPoint)
  //      {
  //        return prevPoint;
  //      }
  //    }
  //  }
  //  // 如果找不到，这是一个逻辑错误，表明输入单元有问题
  //  throw std::runtime_error("Could not find unique adjacent point. Check cell connectivity.");
  //}

  // bool mergeAdjacentQuads(const std::vector<vtkIdType>& quad1, const std::vector<vtkIdType>&
  // quad2,
  //   std::vector<vtkIdType>& mergedQuad, std::vector<Edge>& adjEdges)
  //{
  //   // 步骤1: 输入验证
  //   if (quad1.size() != 4 || quad2.size() != 4)
  //   {
  //     return false;
  //   }

  //  // 步骤2: 查找共享的点
  //  std::unordered_set<vtkIdType> quad1_points(quad1.begin(), quad1.end());
  //  std::vector<vtkIdType> sharedPoints;
  //  for (vtkIdType id : quad2)
  //  {
  //    if (quad1_points.count(id))
  //    {
  //      sharedPoints.push_back(id);
  //    }
  //  }

  //  // 两个相邻的四边形必须共享且只共享两个点
  //  if (sharedPoints.size() != 2)
  //  {
  //    return false;
  //  }

  //  // 步骤3: 识别共享边和外围点
  //  vtkIdType s1 = sharedPoints[0];
  //  vtkIdType s2 = sharedPoints[1];

  //  try
  //  {
  //    // 假设单元的点是按顺序（例如，逆时针）排列的。
  //    // 新四边形的四个角点是与共享点相邻、但本身不是共享点的那些点。

  //    // 查找 quad1 中与 s1 和 s2 相邻的唯一顶点
  //    vtkIdType p4 = findUniqueAdjacentPointOpt(quad1, s1, s2);
  //    vtkIdType p1 = findUniqueAdjacentPointOpt(quad1, s2, s1);

  //    // 查找 quad2 中与 s1 和 s2 相邻的唯一顶点
  //    vtkIdType p3 = findUniqueAdjacentPointOpt(quad2, s1, s2);
  //    vtkIdType p2 = findUniqueAdjacentPointOpt(quad2, s2, s1);

  //    // 步骤4: 按照正确的顺序组装新的四边形
  //    mergedQuad[0] = p1;
  //    mergedQuad[1] = p2;
  //    mergedQuad[2] = p3;
  //    mergedQuad[3] = p4;

  //    Edge edge1 = (s1 < p3) ? std::make_pair(s1, p3) : std::make_pair(p3, s1);
  //    Edge edge2 = (s1 < p4) ? std::make_pair(s1, p4) : std::make_pair(p4, s1);
  //    Edge edge3 = (s2 < p1) ? std::make_pair(s2, p1) : std::make_pair(p1, s2);
  //    Edge edge4 = (s2 < p2) ? std::make_pair(s2, p2) : std::make_pair(p2, s2);

  //    adjEdges[0] = edge1;  //quad1
  //    adjEdges[1] = edge2;  //quad2
  //    adjEdges[2] = edge3;  //quad2
  //    adjEdges[3] = edge4;  //quad1

  //    Edge edge5 = (p1 < p2) ? std::make_pair(p1, p2) : std::make_pair(p2, p1);
  //    Edge edge6 = (p3 < p4) ? std::make_pair(p3, p4) : std::make_pair(p4, p3);

  //    adjEdges[4] = edge5; //mergedQuad
  //    adjEdges[5] = edge6; //mergedQuad
  //  }
  //  catch (const std::runtime_error& e)
  //  {
  //    std::cerr << "Error during merge: " << e.what() << std::endl;
  //    return false;
  //  }

  //  return true;
  //}

  bool mergeAdjacentQuads(const std::vector<vtkIdType>& quad1, const std::vector<vtkIdType>& quad2,
    std::vector<vtkIdType>& mergedQuad)
  {
    if (quad1.size() != 4 || quad2.size() != 4)
    {
      return false;
    }

    std::unordered_set<vtkIdType> quad1_points(quad1.begin(), quad1.end());
    std::vector<vtkIdType> sharedPoints;
    for (vtkIdType id : quad2)
    {
      if (quad1_points.count(id))
      {
        sharedPoints.push_back(id);
      }
    }

    if (sharedPoints.size() != 2)
    {
      return false;
    }

    vtkIdType s1 = sharedPoints[0];
    vtkIdType s2 = sharedPoints[1];

    try
    {
      // 假设单元的点是按顺序（例如，逆时针）排列的。
      // 新四边形的四个角点是与共享点相邻、但本身不是共享点的那些点。

      // 查找 quad1 中与 s1 和 s2 相邻的唯一顶点
      vtkIdType p4 = findUniqueAdjacentPoint(quad1, s1, s2);
      vtkIdType p1 = findUniqueAdjacentPoint(quad1, s2, s1);

      // 查找 quad2 中与 s1 和 s2 相邻的唯一顶点
      vtkIdType p3 = findUniqueAdjacentPoint(quad2, s1, s2);
      vtkIdType p2 = findUniqueAdjacentPoint(quad2, s2, s1);

      // 步骤4: 按照正确的顺序组装新的四边形
      mergedQuad[0] = p1;
      mergedQuad[1] = p2;
      mergedQuad[2] = p3;
      mergedQuad[3] = p4;
    }
    catch (const std::runtime_error& e)
    {
      std::cerr << "Error during merge: " << e.what() << std::endl;
      return false;
    }

    return true;
  }

  bool mergeAdjacentQuads(const std::vector<vtkIdType>& quad1, const std::vector<vtkIdType>& quad2,
    std::vector<vtkIdType>& mergedQuad, Edge& commonEdge, Edge& quad1Edge, Edge& quad2Edge)
  {
    if (quad1.size() != 4 || quad2.size() != 4)
    {
      return false;
    }

    std::unordered_set<vtkIdType> quad1_points(quad1.begin(), quad1.end());
    std::vector<vtkIdType> sharedPoints;
    for (vtkIdType id : quad2)
    {
      if (quad1_points.count(id))
      {
        sharedPoints.push_back(id);
      }
    }

    if (sharedPoints.size() != 2)
    {
      return false;
    }

    vtkIdType s1 = sharedPoints[0];
    vtkIdType s2 = sharedPoints[1];

    commonEdge = (s1 < s2) ? std::make_pair(s1, s2) : std::make_pair(s2, s1);

    try
    {
      // 假设单元的点是按顺序（例如，逆时针）排列的。
      // 新四边形的四个角点是与共享点相邻、但本身不是共享点的那些点。

      // 查找 quad1 中与 s1 和 s2 相邻的唯一顶点
      vtkIdType p4 = findUniqueAdjacentPoint(quad1, s1, s2);
      vtkIdType p1 = findUniqueAdjacentPoint(quad1, s2, s1);

      quad1Edge = (p1 < p4) ? std::make_pair(p1, p4) : std::make_pair(p4, p1);

      // 查找 quad2 中与 s1 和 s2 相邻的唯一顶点
      vtkIdType p3 = findUniqueAdjacentPoint(quad2, s1, s2);
      vtkIdType p2 = findUniqueAdjacentPoint(quad2, s2, s1);

      quad2Edge = (p3 < p2) ? std::make_pair(p3, p2) : std::make_pair(p2, p3);

      // 步骤4: 按照正确的顺序组装新的四边形
      mergedQuad[0] = p1;
      mergedQuad[1] = p2;
      mergedQuad[2] = p3;
      mergedQuad[3] = p4;
    }
    catch (const std::runtime_error& e)
    {
      std::cerr << "Error during merge: " << e.what() << std::endl;
      return false;
    }

    return true;
  }

  void MergeRegion(vtkPolyData* mesh, vtkCellArray* outputPolys, std::vector<bool>& merged,
    std::vector<vtkIdType>& region, const std::vector<std::vector<vtkIdType>>& cellPoints)
  {
    std::unordered_map<vtkIdType, std::vector<vtkIdType>> mapNewQuads; // 合并后四边形单元

    // 构建边-面映射
    EdgeFaceMap edgeFaceMap;

    for (vtkIdType cellId : region)
    { // region为该区域共面的所有四边形单元

      const auto& points = cellPoints[cellId];
      for (int j = 0; j < 4; ++j)
      {
        vtkIdType v0 = points[j];
        vtkIdType v1 = points[(j + 1) % 4];
        Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
        edgeFaceMap[edge].push_back(cellId);
      }

      mapNewQuads[cellId] = points;
    }

    // 只需要维护新增边-新增四边形单元动态关系
    vtkIdType iNewCellId = m_maxCellId;
    std::vector<bool> mergedNew;
    mergedNew.reserve(16);

    std::vector<vtkIdType> mergedQuad(4, -1);
    Edge commonEdge, quad1Edge, quad2Edge;
    EdgeFaceMap::iterator quad1EdgeIter, quad2EdgeIter;

    while (1)
    {
	  bool bAdded = false;
		
      for (auto entry = edgeFaceMap.begin(); entry != edgeFaceMap.end(); ++entry)
      {
        if (entry->second.size() != 2)
        { // 非内部边（被两个面共享）
          continue;
        }

        bAdded = true;

        // 两个面需要被合并
        vtkIdType f1 = entry->second[0];
        vtkIdType f2 = entry->second[1];

        std::vector<vtkIdType>& quad1 = mapNewQuads[f1];
        std::vector<vtkIdType>& quad2 = mapNewQuads[f2];

        // 单元状态标记
        if (f1 < m_maxCellId)
          merged[f1] = true;
        else
          mergedNew[f1 - m_maxCellId] = true;
        if (f2 < m_maxCellId)
          merged[f2] = true;
        else
          mergedNew[f2 - m_maxCellId] = true;

        // 新的四边形单元
        mergedNew.push_back(false);

        if (bOptimize) // optimized code
        {
          if (!mergeAdjacentQuads(quad1, quad2, mergedQuad, commonEdge, quad1Edge, quad2Edge))
          {
            int iTest = 0;
            assert(false);
          }

          entry->second.clear(); // common edge

          // dyanmic topo: remove one quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = quad1[j];
            vtkIdType v1 = quad1[(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            if (edge == commonEdge)
              continue;
            if (edge == quad1Edge)
            {
              quad1EdgeIter = edgeFaceMap.find(quad1Edge);
              std::vector<vtkIdType>& quad = quad1EdgeIter->second;
              quad.erase(std::remove(quad.begin(), quad.end(), f1), quad.end());
            }
            else
            {
              std::vector<vtkIdType>& quad = edgeFaceMap[edge];
              quad.erase(std::remove(quad.begin(), quad.end(), f1), quad.end());
            }
          }
          // dyanmic topo: remove another quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = quad2[j];
            vtkIdType v1 = quad2[(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            if (edge == commonEdge)
              continue;
            if (edge == quad2Edge)
            {
              quad2EdgeIter = edgeFaceMap.find(quad2Edge);
              std::vector<vtkIdType>& quad = quad2EdgeIter->second;
              quad.erase(std::remove(quad.begin(), quad.end(), f2), quad.end());
            }
            else
            {
              std::vector<vtkIdType>& quad = edgeFaceMap[edge];
              quad.erase(std::remove(quad.begin(), quad.end(), f2), quad.end());
            }
          }

          mapNewQuads[iNewCellId] = mergedQuad;

          // dyanmic topo: add a new quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = mergedQuad[j];
            vtkIdType v1 = mergedQuad[(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            if (edge == quad1Edge)
            {
              std::vector<vtkIdType>& quad = quad1EdgeIter->second;
              quad.push_back(iNewCellId);
            }
            else if (edge == quad2Edge)
            {
              std::vector<vtkIdType>& quad = quad2EdgeIter->second;
              quad.push_back(iNewCellId);
            }
            else
            {
              edgeFaceMap[edge].push_back(iNewCellId);
            }
          }
        }
        else
        {
          if (!mergeAdjacentQuads(mapNewQuads[f1], mapNewQuads[f2], mergedQuad))
          {
            int iTest = 0;
            assert(false);
          }

          // dyanmic topo: remove one quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = mapNewQuads[f1][j];
            vtkIdType v1 = mapNewQuads[f1][(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            edgeFaceMap[edge].erase(
              std::remove(edgeFaceMap[edge].begin(), edgeFaceMap[edge].end(), f1),
              edgeFaceMap[edge].end());
          }
          // dyanmic topo: remove another quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = mapNewQuads[f2][j];
            vtkIdType v1 = mapNewQuads[f2][(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            edgeFaceMap[edge].erase(
              std::remove(edgeFaceMap[edge].begin(), edgeFaceMap[edge].end(), f2),
              edgeFaceMap[edge].end());
          }

          mapNewQuads[iNewCellId] = mergedQuad;

          // dyanmic topo: add a new quad
          for (int j = 0; j < 4; ++j)
          {
            vtkIdType v0 = mergedQuad[j];
            vtkIdType v1 = mergedQuad[(j + 1) % 4];
            Edge edge = (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
            edgeFaceMap[edge].push_back(iNewCellId);
          }
        }

        ++iNewCellId;

        // break;  //waste time
      }

      if (!bAdded)
        break;
    }

    // 添加新增的且最终的四边形
    for (auto it = mapNewQuads.begin(); it != mapNewQuads.end(); ++it)
    {
      if (it->first < m_maxCellId)
      {
        if (merged[it->first])
          continue;
      }
      else
      {
        if (mergedNew[it->first - m_maxCellId])
          continue;
      }

      vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
      for (int j = 0; j < 4; ++j)
      {
        cellIds->InsertNextId(it->second[j]);
      }
      outputPolys->InsertNextCell(cellIds);
    }

    return;
  }

private:
  double angleThreshold;
  bool bOptimize;
};
