#include "TracingEngine.h"

#include <rlgl.h>
#include <raymath.h>
#include <iostream>

void TracingEngine::Initialize(Vector2 resolution, int maxBounces, int raysPerPixel, float blur)
{
	numRenderedFrames = 0;
	TracingEngine::resolution = resolution;
	TracingEngine::maxBounces = maxBounces;
	TracingEngine::raysPerPixel = raysPerPixel;
	TracingEngine::blur = blur;

	raytracingRenderTexture = LoadRenderTexture(resolution.x, resolution.y);
	previouseFrameRenderTexture = LoadRenderTexture(resolution.x, resolution.y);

	raytracingShader = LoadShader(0, TextFormat("resources/shaders/raytracer_fragment.glsl", 430));
	postShader = LoadShader(0, TextFormat("resources/shaders/post_fragment.glsl", 430));

	tracingParams.cameraPosition = GetShaderLocation(raytracingShader, "cameraPosition");
	tracingParams.cameraDirection = GetShaderLocation(raytracingShader, "cameraDirection");
	tracingParams.screenCenter = GetShaderLocation(raytracingShader, "screenCenter");
	tracingParams.viewParams = GetShaderLocation(raytracingShader, "viewParams");
	tracingParams.resolution = GetShaderLocation(raytracingShader, "resolution");
	tracingParams.numRenderedFrames = GetShaderLocation(raytracingShader, "numRenderedFrames");
	tracingParams.previousFrame = GetShaderLocation(raytracingShader, "previousFrame");
	tracingParams.raysPerPixel = GetShaderLocation(raytracingShader, "raysPerPixel");
	tracingParams.maxBounces = GetShaderLocation(raytracingShader, "maxBounces");
	tracingParams.denoise = GetShaderLocation(raytracingShader, "denoise");
	tracingParams.blur = GetShaderLocation(raytracingShader, "blur");
	tracingParams.pause = GetShaderLocation(raytracingShader, "pause");

	postParams.resolution = GetShaderLocation(postShader, "resolution");
	postParams.denoise = GetShaderLocation(postShader, "denoise");

	Vector2 screenCenter = Vector2(resolution.x / 2.0f, resolution.y / 2.0f);
	SetShaderValue(raytracingShader, tracingParams.screenCenter, &screenCenter, SHADER_UNIFORM_VEC2);
	SetShaderValue(raytracingShader, tracingParams.resolution, &resolution, SHADER_UNIFORM_VEC2);

	SetShaderValue(postShader, postParams.resolution, &resolution, SHADER_UNIFORM_VEC2);

	SetShaderValue(raytracingShader, tracingParams.raysPerPixel, &raysPerPixel, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, tracingParams.maxBounces, &maxBounces, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, tracingParams.blur, &blur, SHADER_UNIFORM_FLOAT);

	sphereSSBO = rlLoadShaderBuffer(sizeof(SphereBuffer), NULL, RL_DYNAMIC_COPY);
	meshesSSBO = rlLoadShaderBuffer(sizeof(MeshBuffer), NULL, RL_DYNAMIC_COPY);
	trianglesSSBO = rlLoadShaderBuffer(sizeof(TriangleBuffer), NULL, RL_DYNAMIC_COPY);
	nodesSSBO = rlLoadShaderBuffer(sizeof(NodeBuffer), NULL, RL_DYNAMIC_COPY);
}

Vector3 TracingEngine::TriangleCenter(Triangle* triangle)
{
	return (triangle->posA + triangle->posB + triangle->posC) / 3;
}

Vector3 TracingEngine::BoundingBoxCenter(PaddedBoundingBox* box)
{
	return (box->min + box->max) / 2;
}

float TracingEngine::BoundingBoxCenterOnAxis(PaddedBoundingBox* box, int axis)
{
	switch (axis)
	{
	case 0:
		return BoundingBoxCenter(box).x;
	case 1:
		return BoundingBoxCenter(box).y;
	case 2:
		return BoundingBoxCenter(box).z;
	}
}

float TracingEngine::TriangleCenterOnAxis(Triangle* triangle, int axis)
{
	switch (axis)
	{
	case 0:
		return TriangleCenter(triangle).x;
	case 1:
		return TriangleCenter(triangle).y;
	case 2:
		return TriangleCenter(triangle).z;
	}
}

void TracingEngine::SplitNode(int parentIndex, int depth, int maxDepth)
{
	if (depth == maxDepth)
	{
		return;
	}

	nodes[parentIndex].childIndex = nodes.size();

	Vector3 size = nodes[parentIndex].bounds.max - nodes[parentIndex].bounds.min;
	int splitAxis = size.x > std::max(size.y, size.z) ? 0 : size.y > size.z ? 1 : 2;
	float splitPos = BoundingBoxCenterOnAxis(&nodes[parentIndex].bounds, splitAxis);

	Node childA = { .triangleIndex = nodes[parentIndex].triangleIndex };
	Node childB = { .triangleIndex = nodes[parentIndex].triangleIndex };

	childA.bounds.min = BoundingBoxCenter(&nodes[parentIndex].bounds);
	childA.bounds.max = BoundingBoxCenter(&nodes[parentIndex].bounds);

	childB.bounds.min = BoundingBoxCenter(&nodes[parentIndex].bounds);
	childB.bounds.max = BoundingBoxCenter(&nodes[parentIndex].bounds);

	for (int i = 0; i < nodes[parentIndex].numTriangles; i++)
	{
		int triIndex = nodes[parentIndex].triangleIndex + i;
		bool isSideA = TriangleCenterOnAxis(&triangles[triIndex], splitAxis) < splitPos;
		Node* child = isSideA ? &childA : &childB;

		GrowToIncludeTriangle(&child->bounds, triangles[triIndex]);
		child->numTriangles++;

		if (isSideA)
		{
			int swap = child->triangleIndex + child->numTriangles - 1;
			std::swap(triangles[triIndex], triangles[swap]);
			childB.triangleIndex++;
		}
	}

	int childIndexA = nodes.size();
	int childIndexB = nodes.size() + 1;

	nodes.push_back(childA);
	nodes.push_back(childB);

	SplitNode(childIndexA, depth + 1, maxDepth);
	SplitNode(childIndexB, depth + 1, maxDepth);
}

PaddedBoundingBox TracingEngine::GetMeshPaddedBoundingBox(Mesh mesh)
{
	PaddedBoundingBox pb;
	BoundingBox b = GetMeshBoundingBox(mesh);

	pb.min = b.min;
	pb.max = b.max;

	return pb;
}

void TracingEngine::GrowToInclude(PaddedBoundingBox* box, Vector3 point)
{
	PaddedBoundingBox temp = *box;

	Vector3 center = BoundingBoxCenter(box);

	box->min = Vector3Min(temp.min, point);
	box->max = Vector3Max(temp.max, point);
}

void TracingEngine::GrowToIncludeTriangle(PaddedBoundingBox* box, Triangle triangle)
{
	GrowToInclude(box, triangle.posA);
	GrowToInclude(box, triangle.posB);
	GrowToInclude(box, triangle.posC);
}

Vector4 TracingEngine::ColorToVector4(Color color)
{
	float colors[4] = { (float)color.r / (float)255, (float)color.g / (float)255,
					   (float)color.b / (float)255, (float)color.a / (float)255 };
	return Vector4(colors[0], colors[1], colors[2], colors[3]);
}

void TracingEngine::UploadSky()
{
	unsigned int skyColorZenithLocation = GetShaderLocation(raytracingShader, "skyMaterial.skyColorZenith");
	unsigned int skyColorHorizonLocation = GetShaderLocation(raytracingShader, "skyMaterial.skyColorHorizon");
	unsigned int groundColorLocation = GetShaderLocation(raytracingShader, "skyMaterial.groundColor");
	unsigned int sunColorLocation = GetShaderLocation(raytracingShader, "skyMaterial.sunColor");
	unsigned int sunDirectionLocation = GetShaderLocation(raytracingShader, "skyMaterial.sunDirection");
	unsigned int sunFocusLocation = GetShaderLocation(raytracingShader, "skyMaterial.sunFocus");
	unsigned int sunIntensityLocation = GetShaderLocation(raytracingShader, "skyMaterial.sunIntensity");

	Vector4 skyColorZenith = ColorToVector4(skyMaterial.skyColorZenith);
	SetShaderValue(raytracingShader, skyColorZenithLocation, &skyColorZenith, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector

	Vector4 skyColorHorizon = ColorToVector4(skyMaterial.skyColorHorizon);
	SetShaderValue(raytracingShader, skyColorHorizonLocation, &skyColorHorizon, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector

	Vector4 groundColor = ColorToVector4(skyMaterial.groundColor);
	SetShaderValue(raytracingShader, groundColorLocation, &groundColor, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector
	
	Vector4 sunColor = ColorToVector4(skyMaterial.sunColor);
	SetShaderValue(raytracingShader, sunColorLocation, &sunColor, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector

	SetShaderValue(raytracingShader, sunDirectionLocation, &skyMaterial.sunDirection, SHADER_UNIFORM_VEC3);   // Set shader uniform value vector
	SetShaderValue(raytracingShader, sunFocusLocation, &skyMaterial.sunFocus, SHADER_UNIFORM_FLOAT);
	SetShaderValue(raytracingShader, sunIntensityLocation, &skyMaterial.sunIntensity, SHADER_UNIFORM_FLOAT);
}

void TracingEngine::GenerateBVHS()
{
	int triangleOffset = 0;

	for (int i = 0; i < meshes.size(); i++)
	{
		RaytracingMesh mesh = meshes[i];

		PaddedBoundingBox bounds{};
		bounds.min = Vector3(mesh.boundingMin.x, mesh.boundingMin.y, mesh.boundingMin.z);
		bounds.max = Vector3(mesh.boundingMax.x, mesh.boundingMax.y, mesh.boundingMax.z);

		Node root = { .bounds = bounds, .triangleIndex = mesh.firstTriangleIndex, .numTriangles = mesh.numTriangles };
		
		nodes.push_back(root);
		
		meshes[i].rootNodeIndex = nodes.size() - 1;

		triangleOffset += mesh.numTriangles;

		SplitNode(nodes.size() - 1, 0, meshes[i].bvhDepth);
	}

	for (int i = 0; i < nodes.size(); i++)
	{
		nodeBuffer.nodes[i] = nodes[i];
	}
}

void TracingEngine::UploadSSBOS()
{
	rlUpdateShaderBuffer(sphereSSBO, &sphereBuffer, sizeof(SphereBuffer), 0);
	rlUpdateShaderBuffer(meshesSSBO, &meshBuffer, sizeof(MeshBuffer), 0);
	rlUpdateShaderBuffer(trianglesSSBO, &triangleBuffer, sizeof(TriangleBuffer), 0);
	rlUpdateShaderBuffer(nodesSSBO, &nodeBuffer, sizeof(NodeBuffer), 0);

	rlEnableShader(raytracingShader.id);
	rlBindShaderBuffer(sphereSSBO, 1);
	rlBindShaderBuffer(meshesSSBO, 2);
	rlBindShaderBuffer(trianglesSSBO, 3);
	rlBindShaderBuffer(nodesSSBO, 4);
	rlDisableShader();
}

void TracingEngine::UploadSpheres()
{
	for (size_t i = 0; i < spheres.size(); i++)
	{
		sphereBuffer.spheres[i] = spheres[i];
	}
}

void TracingEngine::UploadTriangles()
{
	for (size_t i = 0; i < triangles.size(); i++)
	{
		triangleBuffer.triangles[i] = triangles[i];
	}
}

void TracingEngine::UploadMeshes()
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		meshBuffer.meshes[i] = meshes[i];
	}
}

void TracingEngine::UploadRaylibModel(Model model, RaytracingMaterial material, bool indexed, int bvhDepth)
{
	for (int m = 0; m < model.meshCount; m++)
	{
		Mesh mesh = model.meshes[m];

		BoundingBox bounds = GetMeshBoundingBox(mesh);
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		MatrixDecompose(model.transform, &position, &rotation, &scale);

		bounds.min += position;
		bounds.max += position;

		int firstTriIndex = totalTriangles;

		if (indexed)
		{
			for (int i = 0; i < mesh.triangleCount; i++) {
				Triangle tri;

				// For each triangle, we have 3 indices (in an indexed mesh)
				int idx1 = mesh.indices[i * 3];
				int idx2 = mesh.indices[i * 3 + 1];
				int idx3 = mesh.indices[i * 3 + 2];

				// Assign positions from the vertices array
				Vector3 temp1 = *(Vector3*)&mesh.vertices[idx1 * 3] * model.transform;       // 3 floats per position
				Vector3 temp2 = *(Vector3*)&mesh.vertices[idx2 * 3] * model.transform;
				Vector3 temp3 = *(Vector3*)&mesh.vertices[idx3 * 3] * model.transform;

				tri.posA = temp1;
				tri.posB = temp2;
				tri.posC = temp3;

				Quaternion rotation = QuaternionIdentity();
				Vector3 position = Vector3(0, 0, 0);
				Vector3 scale = Vector3(1, 1, 1);
				MatrixDecompose(model.transform, &position, &rotation, &scale);

				// Assign normals from the normals array
				Vector3 tempA = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx1 * 3], rotation);      // 3 floats per normal
				Vector3 tempB = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx2 * 3], rotation);
				Vector3 tempC = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx3 * 3], rotation);

				tri.normalA = tempA;
				tri.normalB = tempB;
				tri.normalC = tempC;

				triangles.push_back(tri);
				totalTriangles++;
			}
		}
		else
		{
			for (int i = 0; i < mesh.triangleCount; i++) {
				Triangle tri;

				// For each triangle, we have 3 indices (in an indexed mesh)
				int idx1 = i * 3;
				int idx2 = i * 3 + 1;
				int idx3 = i * 3 + 2;

				// Assign positions from the vertices array
				Vector3 temp1 = *(Vector3*)&mesh.vertices[idx1 * 3] * model.transform;       // 3 floats per position
				Vector3 temp2 = *(Vector3*)&mesh.vertices[idx2 * 3] * model.transform;
				Vector3 temp3 = *(Vector3*)&mesh.vertices[idx3 * 3] * model.transform;

				tri.posA = temp1;
				tri.posB = temp2;
				tri.posC = temp3;

				Quaternion rotation = QuaternionIdentity();
				Vector3 position = Vector3(0, 0, 0);
				Vector3 scale = Vector3(1, 1, 1);
				MatrixDecompose(model.transform, &position, &rotation, &scale);

				// Assign normals from the normals array
				Vector3 tempA = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx1 * 3], rotation);      // 3 floats per normal
				Vector3 tempB = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx2 * 3], rotation);
				Vector3 tempC = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx3 * 3], rotation);

				tri.normalA = tempA;
				tri.normalB = tempB;
				tri.normalC = tempC;

				triangles.push_back(tri);
				totalTriangles++;
			}
		}

		RaytracingMesh rmesh = { firstTriIndex, mesh.triangleCount, 0, bvhDepth, material, Vector4(bounds.min.x, bounds.min.y, bounds.min.z, 0), Vector4(bounds.max.x, bounds.max.y, bounds.max.z, 0)};

		TracingEngine::meshes.push_back(rmesh);
	}

	models.push_back(model);
}

void TracingEngine::UploadStaticData()
{
	UploadSpheres();
	UploadSky();

	GenerateBVHS();
	UploadTriangles();
	UploadMeshes();

	UploadSSBOS();
}

void TracingEngine::UploadData(Camera* camera)
{
	float planeHeight = 0.01f * tan(camera->fovy * 0.5f * DEG2RAD) * 2;
	float planeWidth = planeHeight * (resolution.x / resolution.y);
	Vector3 viewParams = Vector3(planeWidth, planeHeight, 0.01f);
	SetShaderValue(raytracingShader, tracingParams.viewParams, &viewParams, SHADER_UNIFORM_VEC3);

	if (denoise)
	{
		if (!pause)
		{
			numRenderedFrames++;
		}
	}
	else
	{
		numRenderedFrames = 0;
	}

	SetShaderValue(raytracingShader, tracingParams.numRenderedFrames, &numRenderedFrames, SHADER_UNIFORM_INT);

	SetShaderValue(raytracingShader, tracingParams.cameraPosition, &camera->position, SHADER_UNIFORM_VEC3);

	float camDist = 1.0f / (tanf(camera->fovy * 0.5f * DEG2RAD));
	Vector3 camDir = Vector3Scale(Vector3Normalize(Vector3Subtract(camera->target, camera->position)), camDist);
	SetShaderValue(raytracingShader, tracingParams.cameraDirection, &(camDir), SHADER_UNIFORM_VEC3);

	SetShaderValue(raytracingShader, tracingParams.denoise, &denoise, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, tracingParams.pause, &pause, SHADER_UNIFORM_INT);

	SetShaderValue(postShader, postParams.denoise, &denoise, SHADER_UNIFORM_INT);
}

void TracingEngine::Render(Camera* camera)
{
	BeginTextureMode(raytracingRenderTexture);
	ClearBackground(BLACK);

	rlEnableDepthTest();
	BeginShaderMode(raytracingShader);

	DrawTextureRec(previouseFrameRenderTexture.texture, Rectangle(0, 0, (float)resolution.x, (float)-resolution.y), Vector2(0, 0), WHITE);
	//DrawRectangleRec(Rectangle(0, 0, (float)resolution.x, (float)resolution.y), WHITE);
	
	EndShaderMode();
	EndTextureMode();

	BeginDrawing();
	BeginShaderMode(postShader);
	ClearBackground(BLACK);

	DrawTextureRec(raytracingRenderTexture.texture, Rectangle(0, 0, (float)resolution.x, (float)-resolution.y), Vector2(0, 0), WHITE);
	EndShaderMode();

	if (debug)
	{
		DrawDebug(camera);
	}

	EndDrawing();

	BeginTextureMode(previouseFrameRenderTexture);
	ClearBackground(WHITE);
	DrawTextureRec(raytracingRenderTexture.texture, Rectangle(0, 0, (float)resolution.x, (float)-resolution.y), Vector2(0, 0), WHITE);
	EndTextureMode();
}

void TracingEngine::DrawDebugBounds(PaddedBoundingBox* box, Color color)
{
	Vector3 dimentions = box->max - box->min;
	DrawCubeWires(BoundingBoxCenter(box), dimentions.x, dimentions.y, dimentions.z, color);
}

void TracingEngine::DrawDebug(Camera* camera)
{
	BeginMode3D(static_cast<Camera3D>(*camera));

	for (size_t i = 0; i < spheres.size(); i++)
	{
		DrawSphereWires(spheres[i].position, spheres[i].radius, 10, 10, RED);
	}

	for (size_t i = 0; i < nodes.size(); i++)
	{
		if (nodes[i].childIndex == 0)
		{
			DrawDebugBounds(&nodes[i].bounds, ORANGE);
		}
	}

	DrawGrid(10, 1);

	EndMode3D();

	DrawFPS(10, 10);
	DrawText(TextFormat("triangles: %i", triangles.size()), 10, 30, 20, RED);
	DrawText(TextFormat("nodes: %i", nodes.size()), 10, 50, 20, RED);
}

void TracingEngine::Unload()
{
	UnloadRenderTexture(raytracingRenderTexture);
	UnloadShader(raytracingShader);
}