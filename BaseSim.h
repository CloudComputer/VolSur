#pragma once

#include <vtkPolyData.h>

class CBaseSim
{
public:
  CBaseSim();
  ~CBaseSim();

 public:
  virtual void SetAngleThreshold(double degrees) = 0;
   virtual vtkSmartPointer<vtkPolyData> Simplify(vtkPolyData* input) = 0;
};
