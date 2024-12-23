#pragma once

#include <vector>
#include <raylib.h>

struct TracingParams
{
	int cameraPosition,
		cameraDirection,
		screenCenter,
		viewParams,
		resolution,
		currentFrame,
		previousFrame,
		numRenderedFrames,
		denoise,
		pause;
};

struct SkyMaterial
{
	Color skyColorZenith;
	Color skyColorHorizon;
	Color groundColor;
	Color sunColor;
	Vector3 sunDirection;
	float sunFocus;
	float sunIntensity;
};

struct RaytracingMaterial
{
	Vector4 color;
	Vector4 emission;
	Vector4 e_s_b_b;
};

struct Sphere
{
	Vector3 position;
	float radius;
	RaytracingMaterial mat;
};

struct Triangle
{
	Vector3 posA;
	float paddingA;
	Vector3 posB;
	float paddingB;
	Vector3 posC;
	float paddingC;
	Vector3 normalA;
	float paddingD;
	Vector3 normalB;
	float paddingE;
	Vector3 normalC;
	float paddingF;
};

struct RaytracingMesh
{
	int firstTriangleIndex;
	int numTriangles;
	long long padding;
	RaytracingMaterial material;
	Vector4 boundingMin;
	Vector4 boundingMax;
};

struct SphereBuffer
{
	Sphere spheres[3];
};

struct TriangleBuffer
{
	Triangle triangles[1000];
};

struct MeshBuffer
{
	RaytracingMesh meshes[2];
};

class TracingEngine
{
private:
	inline static Shader raytracingShader;
	inline static Shader postShader;
	inline static RenderTexture2D raytracingRenderTexture;
	inline static RenderTexture2D previouseFrameRenderTexture;
	inline static TracingParams tracingParams;
	inline static Vector2 resolution;

	inline static int numRenderedFrames;

	inline static int sphereSSBO;
	inline static int trianglesSSBO;
	inline static int meshesSSBO;


	inline static MeshBuffer meshBuffer;
	inline static TriangleBuffer triangleBuffer;
	inline static int totalTriangles = 0;
	inline static int totalMeshes = 0;

	inline static SphereBuffer sphereBuffer;

	static Vector4 ColorToVector4(Color color);

	static void UploadSpheres();
	static void UploadMeshes();
	static void UploadTriangles();

	static void UploadSky();
	static void UploadSSBOS();

	inline static std::vector<RaytracingMesh> meshes;
	inline static std::vector<Triangle> triangles;

public:

	inline static std::vector<Sphere> spheres;

	inline static std::vector<Model> models;

	inline static bool debug = false;
	inline static bool denoise = false;
	inline static bool pause = false;

	inline static SkyMaterial skyMaterial;

	static void Initialize(Vector2 resolution);

	static void UploadRaylibModel(Model model, RaytracingMaterial material, bool indexed);
	static void UploadStaticData();
	static void UploadData(Camera* camera);
	static void Render(Camera* camera);
	static void DrawDebug(Camera* camera);

	static void Unload();
};