
// Copyright Contributors to the OpenVDB Project
// @author CLOUD

#pragma once

#ifdef FASTVOXEL_EXPORTS
#define FASTVOXEL_API __declspec(dllexport)
#else
#define FASTVOXEL_API __declspec(dllimport)
#endif


struct FASTVOXEL_API extVoxSurf;

struct FASTVOXEL_API Point3D;
struct FASTVOXEL_API TriangleFace;
struct FASTVOXEL_API QuadFace;
struct FASTVOXEL_API QuadFaceInfo;


struct FASTVOXEL_API extvoxel_addr
{
	int x, y, z;
	int level;
	extvoxel_addr()
		: x(0), y(0), z(0), level(0)
	{
	}
	extvoxel_addr(int ix, int iy, int iz, int il)
		: x(ix), y(iy), z(iz), level(il)
	{
	}
};

struct FASTVOXEL_API Point3D
{
    double x, y, z;
    Point3D(double x = 0.0, double y = 0.0, double z = 0.0)
        : x(x), y(y), z(z)
    {
    }
};

struct FASTVOXEL_API TriangleFace
{
    int v0, v1, v2;
    TriangleFace(int v0 = 0, int v1 = 0, int v2 = 0)
        : v0(v0), v1(v1), v2(v2)
    {
    }
};

struct FASTVOXEL_API QuadFace
{
    int v0, v1, v2, v3;
    QuadFace(int v0 = 0, int v1 = 0, int v2 = 0, int v3 = 0) : v0(v0), v1(v1), v2(v2), v3(v3)
    {
    }
};

struct FASTVOXEL_API QuadFaceInfo
{
    extvoxel_addr addr;
    int ifacedir;
};

struct FASTVOXEL_API VoxPointKey
{
    int i, j, k;
    VoxPointKey()
    {
        i = j = k = -1;
    }
    VoxPointKey(int x, int y, int z) : i(x), j(y), k(z)
    {
    }
    bool operator==(const VoxPointKey& other) const
    {
        return i == other.i && j == other.j && k == other.k;
    }
};

struct FASTVOXEL_API VoxPointKeyHash
{
    size_t operator()(const VoxPointKey& key) const
    {
        return (key.i * 73856093) ^ (key.j * 19349663) ^ (key.k * 83492791);
    }
};


struct extVoxSurf FASTVOXEL_API* SURFACE_Init(const int& iMaxLevel);

bool FASTVOXEL_API SURFACE_Insert(struct extVoxSurf* pSurface, const extvoxel_addr &voxel);

bool FASTVOXEL_API SURFACE_Prune(struct extVoxSurf* pSurface);

// Memory must be freed externally by calling SURFACE_FreeConSurOptData
bool FASTVOXEL_API SURFACE_BuildConsSur(struct extVoxSurf* pSurface, QuadFaceInfo** quads_out, size_t* quad_count_out);

// Memory must be freed externally by calling SURFACE_FreeConSurOptData
bool FASTVOXEL_API SURFACE_BuildConsSurOpt(struct extVoxSurf* pSurface, QuadFaceInfo** quads_out, size_t* quad_count_out);

// Memory must be freed externally by calling SURFACE_FreeConSurOptData
bool FASTVOXEL_API SURFACE_BuildConsSurOptDeep(struct extVoxSurf* pSurface, QuadFaceInfo** quads_out, size_t* quad_count_out);

void FASTVOXEL_API SURFACE_Release(struct extVoxSurf** pSurface);

void FASTVOXEL_API SURFACE_FreeConSurOptData(QuadFaceInfo* quads);

void FASTVOXEL_API SURFACE_FreeConsSurTree(struct extVoxSurf* pSurface);

bool FASTVOXEL_API SURFACE_VoxelLocaladdr(const extvoxel_addr& addr, extvoxel_addr* localvox, int imaxlevel);

bool FASTVOXEL_API SURFACE_VoxelLocaladdr(const extvoxel_addr& addr, extvoxel_addr* localvox, int localL, int globalL);

bool FASTVOXEL_API SURFACE_VoxelGlobladdr(const extvoxel_addr& addr, extvoxel_addr* globalvox, int imaxlevel);

bool FASTVOXEL_API SURFACE_VoxelGlobladdr(const extvoxel_addr& local, extvoxel_addr* globalvox, int localL, int globalL);

bool FASTVOXEL_API VOXEL_ConsSurPlyFile(const char* filename,
    const Point3D* points, const size_t point_count, const QuadFace* quads, const size_t quad_count);

bool FASTVOXEL_API VOXEL_ConsSurPlyFile(int iMaxLevel, const char* filename,
    const QuadFaceInfo* quads, const size_t quad_count);


