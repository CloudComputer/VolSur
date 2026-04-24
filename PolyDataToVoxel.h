#pragma once

#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkImageStencilData.h>
#include <vtkPointLocator.h>
#include <vtkIncrementalOctreePointLocator.h>


class PolyDataToVoxel
{
public:
  PolyDataToVoxel()
  {
    padding = 0.1;
  }

  void SetInputData(vtkPolyData* poly, bool bComputeBounds);

  void SetVoxelSize(double dx, double dy, double dz)
  {
    spacing[0] = dx;
    spacing[1] = dy;
    spacing[2] = dz;
  }

  void SetBounds(double start[3], double extend[3]);

  vtkSmartPointer<vtkPolyData> GetBoundarySurface();

private:
  vtkSmartPointer<vtkPolyData> polyData;
  double origin[3];
  double spacing[3];
  double padding;  //外包扩展比例
  double bounds[6];  //平移前的边界
  double offsetBounds[6]; // 平移后的边界
  double offset[3];

  void ComputeBounds();
  void ExpandBounds(double bounds[6], double padRatio);

  void TranslatePolyData(vtkPolyData* polyData, double tx, double ty, double tz);
  void TranslateBounds(double tx, double ty, double tz);

  vtkSmartPointer<vtkPolyData> DualContouring(
    vtkImageStencilData* stencilData, const int dims[3] /*, const double origin[3]*/);

  vtkSmartPointer<vtkPolyData> ExtractBoundaryQuadsOpt(
    vtkImageStencilData* stencilData, const int dims[3]/*, const double origin[3]*/);

  vtkSmartPointer<vtkPolyData> ExtractBoundaryQuads(
    vtkImageStencilData* stencilData, const int dims[3] /*, const double origin[3]*/);

  // 添加四边形到网格
  void AddQuadOpt(vtkCellArray* quads, vtkIncrementalOctreePointLocator* locator,
      const int i, const int j, const int k, const int faceDir /*, const double origin[3]*/);  //vtkIncrementalOctreePointLocator比vtkPointLocator更快

  void AddQuad(vtkPoints* points, vtkCellArray* quads, const int i, const int j,
    const int k, const int faceDir/*, const double origin[3]*/);
};
