#pragma once

#include <vtkPolyData.h>


class CFileVTKFunc
{
public:
   CFileVTKFunc();
   ~CFileVTKFunc();

 public:
   static vtkSmartPointer<vtkPolyData> ReadPLYFile(const std::string& filename);

   static vtkPolyData* ReadOFFFile(const std::string& filename);

   static void WritePolyDataToOFF(vtkPolyData* polyData, const std::string& filename);

   static void PolyDataColorizerVtp(vtkPolyData* input, const std::string& filename);
};
