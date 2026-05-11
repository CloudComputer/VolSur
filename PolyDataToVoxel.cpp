
#include "PolyDataToVoxel.h"

#include <omp.h>

#include <bitset>
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>
//#include "vtkPolyDataToImageStencilOpt.h"
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkQuad.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkBitArray.h>
#include "DynamicBitSet.h"
#include <vtkXMLImageDataWriter.h>
#include <vtkImageStencilToImage.h>
#include <vtkStructuredPointsWriter.h>



void PolyDataToVoxel::SetInputData(vtkPolyData* poly, bool bComputeBounds)
{
  polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->DeepCopy(poly);
  if (bComputeBounds)
  {// First set the bounding box
    ComputeBounds();
  }

  TranslatePolyData(polyData, -offset[0], -offset[1], -offset[2]);
  TranslateBounds(-offset[0], -offset[1], -offset[2]);
}

void PolyDataToVoxel::SetBounds(double start[3], double extend[3])
{
  bounds[0] = start[0];
  bounds[1] = start[0] + extend[0];
  bounds[2] = start[1];
  bounds[3] = start[1] + extend[1];
  bounds[4] = start[2];
  bounds[5] = start[2] + extend[2];

  origin[0] = bounds[0];
  origin[1] = bounds[2];
  origin[2] = bounds[4];

  offset[0] = std::floor(origin[0]);
  offset[1] = std::floor(origin[1]);
  offset[2] = std::floor(origin[2]);
}

void PolyDataToVoxel::ComputeBounds()
{
  polyData->GetBounds(bounds);
  ExpandBounds(bounds, padding);

  origin[0] = bounds[0];
  origin[1] = bounds[2];
  origin[2] = bounds[4];

  offset[0] = std::floor(origin[0]);
  offset[1] = std::floor(origin[1]);
  offset[2] = std::floor(origin[2]);
}

vtkSmartPointer<vtkPolyData> PolyDataToVoxel::GetBoundarySurface()
{
  if (!polyData)
  {
    vtkGenericWarningMacro("Input polyData is null!");
    return nullptr;
  }

  // 1. Compute bounding box  

  // 2. Compute image dimensions
  int dims[3];
  dims[0] = static_cast<int>((offsetBounds[1] - offsetBounds[0]) / spacing[0]) + 1;
  dims[1] = static_cast<int>((offsetBounds[3] - offsetBounds[2]) / spacing[1]) + 1;
  dims[2] = static_cast<int>((offsetBounds[5] - offsetBounds[4]) / spacing[2]) + 1;

  // 3. Create stencil data
  auto stencilData = vtkSmartPointer<vtkImageStencilData>::New();

  // Improvement: Rewrite vtkPolyDataToImageStencil

  double newOrigin[3] = { 0,0,0 };
  auto stencilFilter = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  stencilFilter->SetInputData(polyData);
  stencilFilter->SetOutputOrigin(newOrigin);
  stencilFilter->SetOutputSpacing(spacing);
  //stencilFilter->SetTolerance(0.01 * std::min({ spacing[0], spacing[1], spacing[2] }));
  stencilFilter->SetOutputWholeExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  stencilFilter->Update();
  stencilData->DeepCopy(stencilFilter->GetOutput());

  // 4. Extract boundary quads
  auto boundaryMesh = ExtractBoundaryQuadsOpt(stencilData, dims);
  
  TranslatePolyData(boundaryMesh, offset[0], offset[1], offset[2]);

  return boundaryMesh;
}

void PolyDataToVoxel::ExpandBounds(double bounds[6], double padRatio)
{
  double padX = (bounds[1] - bounds[0]) * padRatio;
  double padY = (bounds[3] - bounds[2]) * padRatio;
  double padZ = (bounds[5] - bounds[4]) * padRatio;

  bounds[0] -= padX;
  bounds[1] += padX;
  bounds[2] -= padY;
  bounds[3] += padY;
  bounds[4] -= padZ;
  bounds[5] += padZ;
}

void PolyDataToVoxel::TranslatePolyData(vtkPolyData* polyData, double tx, double ty, double tz)
{
  if (!polyData)
  {
    return;
  }

  vtkPoints* points = polyData->GetPoints();
  if (!points)
  {
    return;
  }

  vtkIdType numPoints = points->GetNumberOfPoints();
  double point[3];

  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    points->GetPoint(i, point); // Get current point coordinates
    point[0] += tx;             // Translate in X direction
    point[1] += ty;             // Translate in Y direction
    point[2] += tz;             // Translate in Z direction
    points->SetPoint(i, point); // Update point coordinates
  }
}

void PolyDataToVoxel::TranslateBounds(double tx, double ty, double tz)
{
  offsetBounds[0] = bounds[0] + tx;
  offsetBounds[1] = bounds[1] + tx;
  offsetBounds[2] = bounds[2] + ty;
  offsetBounds[3] = bounds[3] + ty;
  offsetBounds[4] = bounds[4] + tz;
  offsetBounds[5] = bounds[5] + tz;
}

vtkSmartPointer<vtkPolyData> PolyDataToVoxel::DualContouring(
  vtkImageStencilData* stencilData, const int dims[3] /*, const double origin[3]*/)
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto quads = vtkSmartPointer<vtkCellArray>::New();

  int iMaxDim = std::max({ spacing[0], spacing[1], spacing[2] });
  points->Allocate(iMaxDim * iMaxDim);
  quads->Allocate(iMaxDim * iMaxDim);

  // Get voxel count (subtract 1 from each dimension)
  int voxelDims[3] = { dims[0] - 1, dims[1] - 1, dims[2] - 1 };

  // Scanline traversal along three axes [This is essentially the DC method, similar to scanline parity method - faster because middle cells can be skipped]



  return vtkSmartPointer<vtkPolyData>();
}

struct quadv
{
  quadv(int i, int j, int k, int dir)
      : i(i), j(j), k(k), dir(dir)
  {
  }
  int i = -1;
  int j = -1;
  int k = -1;
  int dir = -1;
};

vtkSmartPointer<vtkPolyData> PolyDataToVoxel::ExtractBoundaryQuadsOpt(
  vtkImageStencilData* stencilData, const int dims[3] /*, const double origin[3]*/)
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto quads = vtkSmartPointer<vtkCellArray>::New();

  int iMaxDim = std::max({ spacing[0], spacing[1], spacing[2] });
  points->Allocate(iMaxDim * iMaxDim);
  quads->Allocate(iMaxDim * iMaxDim);

  // Use 2D slice caching, replace vtkBitArray with DynamicBitset
  std::vector<DynamicBitset> currentSlice(dims[1]); // Current slice z
  std::vector<DynamicBitset> prevSlice(dims[1]);    // Previous slice z-1
  std::vector<DynamicBitset> nextSlice(dims[1]);    // Next slice z+1

  // Lambda function to pre-fill slice data
  auto fillSlice = [&](int z, std::vector<DynamicBitset>& slice)
  {
    for (int y = 0; y < dims[1]; y++)
    {
      // Initialize or reset bitset
      if (slice[y].size() != dims[0])
      {
        slice[y] = DynamicBitset(dims[0]); // Initialize to all 0
      }
      else
      {
        slice[y].reset(); // Fast reset existing bitset
      }

      int r1=-1, r2=-1;
      int iter = 0;
      int moreSubExtents = 1;

      // Get all intervals for current scanline
      while (moreSubExtents)
      {
        moreSubExtents = stencilData->GetNextExtent(r1, r2, 0, dims[0] - 1, y, z, iter);
        if (r1 <= r2)
        {
          for (int x = r1; x <= r2; x++)
          {
            slice[y].set(x, true);
          }
        }
      }
    }
  };

  // test
  if (0)
  {
    int i = 632;
    int j = 1412;
    int k = 323;
    int iTest = stencilData->IsInside(i, j, k);
    double p0[3];
    p0[0] = /*origin[0] + */ i * spacing[0]; // voxel center
    p0[1] = /*origin[1] + */ j * spacing[1];
    p0[2] = /*origin[2] + */ k * spacing[2];
    int ii = 0;
  }

  // Initial fill z=0
  fillSlice(0, currentSlice);
  if (dims[2] > 1)
  {
    fillSlice(1, nextSlice); // Fill z=1
  }

  // Process all Z slices
  for (int z = 0; z < dims[2]; z++)
  {
    // Process each voxel in current slice
    for (int y = 0; y < dims[1]; y++)
    {
      for (int x = 0; x < dims[0]; x++)
      {
        // Check if it's a solid voxel
        if (currentSlice[y].test(x))
        {
          // Check faces in six directions
          // Left face (x-1)
          if (x == 0 || !currentSlice[y].test(x - 1))
          {
            AddQuad(points, quads, x, y, z, 0);
          }

          // Right face (x+1)
          if (x == dims[0] - 1 || !currentSlice[y].test(x + 1))
          {
            AddQuad(points, quads, x + 1, y, z, 0);
          }

          // Front face (y-1)
          if (y == 0 || !currentSlice[y - 1].test(x))
          {
            AddQuad(points, quads, x, y, z, 1);
          }

          // Back face (y+1)
          if (y == dims[1] - 1 || !currentSlice[y + 1].test(x))
          {
            AddQuad(points, quads, x, y + 1, z, 1);
          }

          // Bottom face (z-1)
          if (z == 0 || !prevSlice[y].test(x))
          {
            AddQuad(points, quads, x, y, z, 2);
          }

          // Top face (z+1)
          if (z == dims[2] - 1 || !nextSlice[y].test(x))
          {
            AddQuad(points, quads, x, y, z + 1, 2);
          }
        }
      }
    }

    // Update slice cache (except last layer)
    if (z < dims[2] - 1)
    {
      prevSlice = std::move(currentSlice);
      currentSlice = std::move(nextSlice);
      nextSlice = std::vector<DynamicBitset>(dims[1]); // Reset
      // Fill nextSlice for next layer (z+2)
      fillSlice(z + 2, nextSlice);
    }
  }

  // Create polygon data
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetPolys(quads);

  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputData(polyData);
  cleaner->PointMergingOn();
  // cleaner->SetTolerance(0.1 * std::min({ spacing[0], spacing[1], spacing[2] })); //
  // Improve deduplication efficiency (if changed to 0.01, efficiency will decrease significantly)
  cleaner->Update();

  auto boundaryMesh = cleaner->GetOutput();

  return boundaryMesh;
}

vtkSmartPointer<vtkPolyData> PolyDataToVoxel::ExtractBoundaryQuads(
  vtkImageStencilData* stencilData, const int dims[3])
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto quads = vtkSmartPointer<vtkCellArray>::New();
  
  int iMaxDim = std::max({ spacing[0], spacing[1], spacing[2] });
  points->Allocate(iMaxDim * iMaxDim);
  quads->Allocate(iMaxDim * iMaxDim);

  // test
  if (0)
  {
    int i = 632;
    int j = 1412;
    int k = 323;
    int iTest = stencilData->IsInside(i, j, k);
    double p0[3];
    p0[0] = /*origin[0] + */ i * spacing[0]; // voxel center
    p0[1] = /*origin[1] + */ j * spacing[1];
    p0[2] = /*origin[2] + */ k * spacing[2];
    int ii = 0;
  }

  std::vector<quadv> vecQUADs;
  vecQUADs.reserve(iMaxDim * iMaxDim);

  // Process all Z slices
  for (int z = 0; z < dims[2]; z++)
  {
    // Process each voxel in current slice
    for (int y = 0; y < dims[1]; y++)
    {
      for (int x = 0; x < dims[0]; x++)
      {
        // Check if it's a solid voxel
        if (stencilData->IsInside(x, y, z))
        {
          // Check faces in six directions
          // Left face (x-1)
          if (x == 0 || !stencilData->IsInside(x-1, y, z))
          {
            vecQUADs.push_back(quadv(x, y, z, 0));
          }

          // Right face (x+1)
          if (x == dims[0] - 1 || !stencilData->IsInside(x+1, y, z))
          {
            vecQUADs.push_back(quadv(x + 1, y, z, 0));
          }

          // Front face (y-1)
          if (y == 0 || !stencilData->IsInside(x, y-1, z))
          {
            vecQUADs.push_back(quadv(x, y, z, 1));
          }

          // Back face (y+1)
          if (y == dims[1] - 1 || !stencilData->IsInside(x, y+1, z))
          {
            vecQUADs.push_back(quadv(x, y + 1, z, 1));
          }

          // Bottom face (z-1)
          if (z == 0 || !stencilData->IsInside(x, y, z-1))
          {
            vecQUADs.push_back(quadv(x, y, z, 2));
          }

          // Top face (z+1)
          if (z == dims[2] - 1 || !stencilData->IsInside(x, y, z+1))
          {
            vecQUADs.push_back(quadv(x, y, z + 1, 2));
          }
        }
      }
    }
  }

  auto locator = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  // locator->SetTolerance(0.1 * std::min({ spacing[0], spacing[1], spacing[2] }));
  locator->InitPointInsertion(points, offsetBounds);
  for (int i = 0; i < vecQUADs.size(); i++)
  {
    AddQuadOpt(quads, locator, vecQUADs[i].i, vecQUADs[i].j, vecQUADs[i].k, vecQUADs[i].dir);
  }

  // Create polygon data
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetPolys(quads);

  return polyData;
}

void PolyDataToVoxel::AddQuadOpt(vtkCellArray* quads, vtkIncrementalOctreePointLocator* locator,
      const int i, const int j, const int k, const int faceDir /*, const double origin[3]*/)
{
  double p0[3], p1[3], p2[3], p3[3];

  // Compute vertex coordinates
  switch (faceDir)
  {
    case 0:                                            // X-facing quad
      p0[0] = /*origin[0] + */ spacing[0] * (i - 0.5); // voxel center
      p0[1] = /*origin[1] + */ spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */ spacing[2] * (k - 0.5);

      p1[0] = p0[0];
      p1[1] = p0[1] + spacing[1];
      p1[2] = p0[2];

      p2[0] = p0[0];
      p2[1] = p1[1];
      p2[2] = p0[2] + spacing[2];

      p3[0] = p0[0];
      p3[1] = p0[1];
      p3[2] = p2[2];
      break;

    case 1:                                            // Y-facing quad
      p0[0] = /*origin[0] + */ spacing[0] * (i - 0.5); // voxel center
      p0[1] = /*origin[1] + */ spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */ spacing[2] * (k - 0.5);

      p1[0] = p0[0] + spacing[0];
      p1[1] = p0[1];
      p1[2] = p0[2];

      p2[0] = p1[0];
      p2[1] = p0[1];
      p2[2] = p0[2] + spacing[2];

      p3[0] = p0[0];
      p3[1] = p0[1];
      p3[2] = p2[2];
      break;

    case 2:                                            // Z-facing quad
      p0[0] = /*origin[0] + */ spacing[0] * (i - 0.5); // voxel center
      p0[1] = /*origin[1] + */ spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */ spacing[2] * (k - 0.5);

      p1[0] = p0[0] + spacing[0];
      p1[1] = p0[1];
      p1[2] = p0[2];

      p2[0] = p1[0];
      p2[1] = p0[1] + spacing[1];
      p2[2] = p0[2];

      p3[0] = p0[0];
      p3[1] = p2[1];
      p3[2] = p0[2];
      break;
  }

  // Add quad
  vtkIdType pts[4];
  locator->InsertUniquePoint(p0, pts[0]);
  locator->InsertUniquePoint(p1, pts[1]);
  locator->InsertUniquePoint(p2, pts[2]);
  locator->InsertUniquePoint(p3, pts[3]);

  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, pts[0]);
  quad->GetPointIds()->SetId(1, pts[1]);
  quad->GetPointIds()->SetId(2, pts[2]);
  quad->GetPointIds()->SetId(3, pts[3]);

  quads->InsertNextCell(quad);
}

void PolyDataToVoxel::AddQuad(vtkPoints* points, vtkCellArray* quads, const int i, const int j, const int k, const int faceDir/*, const double origin[3]*/)
{
  double p0[3], p1[3], p2[3], p3[3];

  // Compute vertex coordinates
  switch (faceDir)
  {
    case 0: // X-facing quad
      p0[0] = /*origin[0] + */spacing[0] * (i - 0.5);  // voxel center
      p0[1] = /*origin[1] + */spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */spacing[2] * (k - 0.5);

      p1[0] = p0[0];
      p1[1] = p0[1] + spacing[1];
      p1[2] = p0[2];

      p2[0] = p0[0];
      p2[1] = p1[1];
      p2[2] = p0[2] + spacing[2];

      p3[0] = p0[0];
      p3[1] = p0[1];
      p3[2] = p2[2];
      break;

    case 1: // Y-facing quad
      p0[0] = /*origin[0] + */spacing[0] * (i - 0.5);  // voxel center
      p0[1] = /*origin[1] + */spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */spacing[2] * (k - 0.5);

      p1[0] = p0[0] + spacing[0];
      p1[1] = p0[1];
      p1[2] = p0[2];

      p2[0] = p1[0];
      p2[1] = p0[1];
      p2[2] = p0[2] + spacing[2];

      p3[0] = p0[0];
      p3[1] = p0[1];
      p3[2] = p2[2];
      break;

    case 2: // Z-facing quad
      p0[0] = /*origin[0] + */spacing[0] * (i - 0.5);  // voxel center
      p0[1] = /*origin[1] + */spacing[1] * (j - 0.5);
      p0[2] = /*origin[2] + */spacing[2] * (k - 0.5);

      p1[0] = p0[0] + spacing[0];
      p1[1] = p0[1];
      p1[2] = p0[2];

      p2[0] = p1[0];
      p2[1] = p0[1] + spacing[1];
      p2[2] = p0[2];

      p3[0] = p0[0];
      p3[1] = p2[1];
      p3[2] = p0[2];
      break;
  }

  // Add quad
  vtkIdType pts[4];
  pts[0] = points->InsertNextPoint(p0);
  pts[1] = points->InsertNextPoint(p1);
  pts[2] = points->InsertNextPoint(p2);
  pts[3] = points->InsertNextPoint(p3);

  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, pts[0]);
  quad->GetPointIds()->SetId(1, pts[1]);
  quad->GetPointIds()->SetId(2, pts[2]);
  quad->GetPointIds()->SetId(3, pts[3]);

  quads->InsertNextCell(quad);
}

