#include "GriedTest.h"



CGriedTest::CGriedTest()
{
}

CGriedTest::~CGriedTest()
{
}

vtkSmartPointer<vtkPolyData> CGriedTest::CreatePlaneGrid(int rows, int cols, double size)
{
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto cells = vtkSmartPointer<vtkCellArray>::New();

  //crate points
  for (int i = 0; i <= rows; i++)
  {
    for (int j = 0; j <= cols; j++)
    {
      points->InsertNextPoint(j * size, i * size, 0.0);
    }
  }

  //crate cells
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      vtkIdType quad[4];
      quad[0] = i * (cols + 1) + j;
      quad[1] = i * (cols + 1) + j + 1;
      quad[2] = (i + 1) * (cols + 1) + j + 1;
      quad[3] = (i + 1) * (cols + 1) + j;
      cells->InsertNextCell(4, quad);
    }
  }

  auto mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->SetPoints(points);
  mesh->SetPolys(cells);

  return mesh;
}