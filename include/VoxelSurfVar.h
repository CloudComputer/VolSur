
// Copyright Contributors to the OpenVDB Project
// @author CLOUD

#pragma once

#ifdef FASTVOXEL_EXPORTS
#define FASTVOXEL_API __declspec(dllexport)
#else
#define FASTVOXEL_API __declspec(dllimport)
#endif


struct FASTVOXEL_API VARextVoxSurf;

struct FASTVOXEL_API VARPoint3D;
struct FASTVOXEL_API VARTriangleFace;
struct FASTVOXEL_API VARQuadFace;
struct FASTVOXEL_API VARQuadFaceInfo;


struct FASTVOXEL_API VARextvoxel_addr
{
    int x, y, z;
    int size[3];
    VARextvoxel_addr()
    {
        x = y = z = size[0] = size[1] = size[2] = 0;
    }
    VARextvoxel_addr(int ix, int iy, int iz)
    {
        x = ix;
        y = iy;
        z = iz;
        //unit 1
        size[0] = 1;
        size[1] = 1;
        size[2] = 1;
    }
    VARextvoxel_addr(int ix, int iy, int iz, int iS[3])
	{
        x = ix;
        y = iy;
        z = iz;
        size[0] = iS[0];
        size[1] = iS[1];
        size[2] = iS[2];
	}
    bool is_equal_pt() const
    {//unit 1
        return (size[0] == 1) &&
            (size[1] == 1) &&
            (size[2] == 1);
    }
};

struct FASTVOXEL_API VARPoint3D
{
    double x, y, z;
    VARPoint3D(double x = 0.0, double y = 0.0, double z = 0.0)
        : x(x), y(y), z(z)
    {
    }
};

struct FASTVOXEL_API VARTriangleFace
{
    int v0, v1, v2;
    VARTriangleFace(int v0 = 0, int v1 = 0, int v2 = 0)
        : v0(v0), v1(v1), v2(v2)
    {
    }
};

struct FASTVOXEL_API VARQuadFace
{
    int v0, v1, v2, v3;
    VARQuadFace(int v0 = 0, int v1 = 0, int v2 = 0, int v3 = 0) : v0(v0), v1(v1), v2(v2), v3(v3)
    {
    }
};

struct FASTVOXEL_API VARQuadFaceInfo
{
    VARextvoxel_addr addr;
    int ifacedir;
};

struct FASTVOXEL_API VARVoxPointKey
{
    int i, j, k;
    VARVoxPointKey()
    {
        i = j = k = -1;
    }
    VARVoxPointKey(int x, int y, int z) : i(x), j(y), k(z)
    {
    }
    bool operator==(const VARVoxPointKey& other) const
    {
        return i == other.i && j == other.j && k == other.k;
    }
};

struct FASTVOXEL_API VARVoxPointKeyHash
{
    size_t operator()(const VARVoxPointKey& key) const
    {
        return (key.i * 73856093) ^ (key.j * 19349663) ^ (key.k * 83492791);
    }
};


struct VARextVoxSurf FASTVOXEL_API* VARSURFACE_Init(const char* pathname);

void FASTVOXEL_API VARSURFACE_VoxelSize(struct VARextVoxSurf* pSurface, double voxelsize[3]);

bool FASTVOXEL_API VARSURFACE_Insert(struct VARextVoxSurf* pSurface, const VARextvoxel_addr &voxel);

void FASTVOXEL_API VARSURFACE_TreeInfo(struct VARextVoxSurf* pSurface);

bool FASTVOXEL_API VARSURFACE_Prune(struct VARextVoxSurf* pSurface);

bool FASTVOXEL_API VARSURFACE_Save(struct VARextVoxSurf* pSurface);

// Note: memory release
bool FASTVOXEL_API VARSURFACE_BuildConsSurOpt(struct VARextVoxSurf* pSurface, VARQuadFaceInfo** quads_out, size_t* quad_count_out);

void FASTVOXEL_API VARSURFACE_Release(struct VARextVoxSurf** pSurface);

void FASTVOXEL_API VARSURFACE_FreeConSurOptData(struct VARQuadFaceInfo* quads);

void FASTVOXEL_API VARSURFACE_FreeConsSurTree(struct VARextVoxSurf* pSurface);

bool FASTVOXEL_API VARVOXEL_ConsSurPlyFile(const char* filename,
    const VARPoint3D* points, const size_t point_count, const VARQuadFace* quads, const size_t quad_count);

bool FASTVOXEL_API VARVOXEL_ConsSurPlyFile(int iMaxLevel, double voxelSize[3], const char* filename,
    const VARQuadFaceInfo* quads, const size_t quad_count);


