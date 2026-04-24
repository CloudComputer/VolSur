
#include "MeshConvert.h"

#include "QuadMeshSim.h"


vtkSmartPointer<vtkPolyData> MeshConvert::ConvertToVTKPolyData(const std::vector<VARPoint3D>& inVertices,
	const std::vector<std::vector<int>>& inFaces)
{
    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto cells = vtkSmartPointer<vtkCellArray>::New();

    // 1. 添加顶点
    points->SetNumberOfPoints(inVertices.size());
    for (size_t i = 0; i < inVertices.size(); ++i)
    {
        points->SetPoint(i, inVertices[i].x, inVertices[i].y, inVertices[i].z);
    }
    polyData->SetPoints(points);

    // 2. 添加四边形单元
    for (const auto& face : inFaces)
    {
        // 确保是四边形（4个顶点）
        if (face.size() == 4) {
            vtkIdType pts[4] = { face[0], face[1], face[2], face[3] };
            cells->InsertNextCell(4, pts);
        }
        // 注意：如果face是三角形(3个顶点)，此处需要不同的处理，但CQuadMeshSim可能不处理。
    }
    polyData->SetPolys(cells);

    return polyData;
}

void MeshConvert::ConvertFromVTKPolyData(vtkPolyData* polyData, std::vector<VARPoint3D>& outVertices,
    std::vector<std::vector<int>>& outFaces)
{
    outVertices.clear();
    outFaces.clear();

    // 1. 获取顶点
    vtkPoints* points = polyData->GetPoints();
    if (points)
    {
        vtkIdType numPoints = points->GetNumberOfPoints();
        outVertices.reserve(numPoints);
        for (vtkIdType i = 0; i < numPoints; ++i)
        {
            double p[3];
            points->GetPoint(i, p);
            outVertices.emplace_back(p[0], p[1], p[2]);
        }
    }

    // 2. 获取单元（四边形）
    vtkCellArray* polys = polyData->GetPolys();
    if (polys)
    {
        polys->InitTraversal();
        vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
        while (polys->GetNextCell(idList))
        {
            if (idList->GetNumberOfIds() == 4) { // 只处理四边形
                std::vector<int> face(4);
                for (int i = 0; i < 4; ++i)
                {
                    face[i] = idList->GetId(i);
                }
                outFaces.push_back(std::move(face));
            }
        }
    }
}

bool MeshConvert::SimplifyMeshWithQuadMeshSim(const std::vector<VARPoint3D>& inputVertices,
    const std::vector<std::vector<int>>& inputFaces,
    std::vector<VARPoint3D>& outputVertices,
    std::vector<std::vector<int>>& outputFaces,
    double angleThreshold)
{
    // 1. 转换为VTK格式
    vtkSmartPointer<vtkPolyData> inputPolyData = ConvertToVTKPolyData(inputVertices, inputFaces);
    if (!inputPolyData || inputPolyData->GetNumberOfPolys() == 0)
    {
        return false; // 输入数据无效
    }

    // 2. 创建并配置简化器
    CQuadMeshSim simplifier;
    simplifier.SetAngleThreshold(angleThreshold);
    simplifier.SetOptimize(true); // 使用优化算法，默认为false

    // 3. 执行简化
    vtkSmartPointer<vtkPolyData> outputPolyData = simplifier.Simplify(inputPolyData);
    if (!outputPolyData)
    {
        return false; // 简化失败
    }

    // 4. 转换回自定义数据结构
    ConvertFromVTKPolyData(outputPolyData, outputVertices, outputFaces);
    return true;
}




