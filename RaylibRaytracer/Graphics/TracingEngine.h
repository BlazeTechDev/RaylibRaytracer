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
	Color color;
	Color emissionColor;
	float emissionStrength;
	float smoothness;
};

struct Sphere
{
	Vector3 position;
	float radius;
	RaytracingMaterial material;
};

struct Triangle
{
	Vector3 posA;
	Vector3 posB;
	Vector3 posC;
	Vector3 normalA;
	Vector3 normalB;
	Vector3 normalC;
};

struct RaytracingMesh
{
	Matrix transform;
	int firstTriangleIndex;
	int numTriangles;
	Vector3 boundsMin;
	Vector3 boundsMax;
	RaytracingMaterial material;
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

	static Vector4 ColorToVector4(Color color);

	static void UploadSpheres();
	static void UploadSphere(Sphere* sphere, int index);

	static void UploadTriangle(Triangle triangle, int triangleIndex);

	static void UploadMeshes();
	static void UploadMesh(RaytracingMesh* mesh, int index);

	static void UploadSky();

	inline static std::vector<RaytracingMesh> meshes;
	inline static std::vector<Triangle> triangles;

public:
	static void UploadTriangles();

	inline static std::vector<Sphere> spheres;

	inline static std::vector<Model> models;

	inline static bool debug = false;
	inline static bool denoise = false;
	inline static bool pause = false;

	inline static SkyMaterial skyMaterial;

	static void Initialize(Vector2 resolution);

	static void UploadRaylibModel(Model* model, RaytracingMaterial material);
	static void UploadData(Camera* camera);
	static void Render(Camera* camera);
	static void DrawDebug(Camera* camera);

	static void Unload();
};