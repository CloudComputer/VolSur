#pragma once

#include <vtkPolyData.h>


class CGriedTest
{
public:
   CGriedTest();
   ~CGriedTest();

 public:
   static vtkSmartPointer<vtkPolyData> CreatePlaneGrid(int rows, int cols, double size = 1.0);
};
