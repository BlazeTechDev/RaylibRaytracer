#include "TracingEngine.h"

#include <rlgl.h>
#include <raymath.h>
#include <iostream>

void TracingEngine::Initialize(Vector2 resolution)
{
	numRenderedFrames = 0;
	TracingEngine::resolution = resolution;

	raytracingRenderTexture = LoadRenderTexture(resolution.x, resolution.y);
	previouseFrameRenderTexture = LoadRenderTexture(resolution.x, resolution.y);

	raytracingShader = LoadShader(0, TextFormat("resources/shaders/raytracer_fragment.glsl", 330));

	tracingParams.cameraPosition = GetShaderLocation(raytracingShader, "cameraPosition");
	tracingParams.cameraDirection = GetShaderLocation(raytracingShader, "cameraDirection");
	tracingParams.screenCenter = GetShaderLocation(raytracingShader, "screenCenter");
	tracingParams.viewParams = GetShaderLocation(raytracingShader, "viewParams");
	tracingParams.resolution = GetShaderLocation(raytracingShader, "resolution");
	tracingParams.numRenderedFrames = GetShaderLocation(raytracingShader, "numRenderedFrames");
	tracingParams.previousFrame = GetShaderLocation(raytracingShader, "previousFrame");
	tracingParams.denoise = GetShaderLocation(raytracingShader, "denoise");
	tracingParams.pause = GetShaderLocation(raytracingShader, "pause");

	Vector2 screenCenter = Vector2(resolution.x / 2.0f, resolution.y / 2.0f);
	SetShaderValue(raytracingShader, tracingParams.screenCenter, &screenCenter, SHADER_UNIFORM_VEC2);
	SetShaderValue(raytracingShader, tracingParams.resolution, &resolution, SHADER_UNIFORM_VEC2);
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

void TracingEngine::UploadSpheres()
{
	for (size_t i = 0; i < spheres.size(); i++)
	{
		UploadSphere(&spheres[i], (int)i);
	}
}

void TracingEngine::UploadSphere(Sphere* sphere, int index)
{
	unsigned int spherePositionLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].position", index));
	unsigned int sphereRadiusLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].radius", index));
	unsigned int sphereColorLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].material.color", index));
	unsigned int sphereEmissionColorLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].material.emissionColor", index));
	unsigned int sphereEmissionStrengthLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].material.emissionStrength", index));
	unsigned int sphereSmoothnessLocation = GetShaderLocation(raytracingShader, TextFormat("spheres[%i].material.smoothness", index));

	SetShaderValue(raytracingShader, spherePositionLocation, &sphere->position, SHADER_UNIFORM_VEC3);   // Set shader uniform value vector
	SetShaderValue(raytracingShader, sphereRadiusLocation, &sphere->radius, SHADER_UNIFORM_FLOAT);   // Set shader uniform value vector

	Vector4 color = ColorToVector4(sphere->material.color);
	SetShaderValue(raytracingShader, sphereColorLocation, &color, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector
	
	Vector4 emissionColor = ColorToVector4(sphere->material.emissionColor);
	SetShaderValue(raytracingShader, sphereEmissionColorLocation, &emissionColor, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector
	
	SetShaderValue(raytracingShader, sphereEmissionStrengthLocation, &sphere->material.emissionStrength, SHADER_UNIFORM_FLOAT);
	SetShaderValue(raytracingShader, sphereSmoothnessLocation, &sphere->material.smoothness, SHADER_UNIFORM_FLOAT);
}

void TracingEngine::UploadTriangles()
{
	for (size_t i = 0; i < triangles.size(); i++)
	{
		UploadTriangle(triangles[i], (int)i);
	}
}

void TracingEngine::UploadTriangle(Triangle triangle, int triangleIndex)
{
	unsigned int posALocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].posA", triangleIndex));
	unsigned int posBLocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].posB", triangleIndex));
	unsigned int posCLocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].posC", triangleIndex));
	unsigned int normalALocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].normalA", triangleIndex));
	unsigned int normalBLocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].normalB", triangleIndex));
	unsigned int normalCLocation = GetShaderLocation(raytracingShader, TextFormat("triangles[%i].normalC", triangleIndex));

	SetShaderValue(raytracingShader, posALocation, &triangle.posA, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, posBLocation, &triangle.posB, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, posCLocation, &triangle.posC, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, normalALocation, &triangle.normalA, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, normalBLocation, &triangle.normalB, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, normalCLocation, &triangle.normalC, SHADER_UNIFORM_VEC3);
}

void TracingEngine::UploadMeshes()
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		UploadMesh(&meshes[i], (int)i);
	}
}

void TracingEngine::UploadMesh(RaytracingMesh* mesh, int index)
{
	unsigned int transformLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].transform", index));
	unsigned int firstTrianglesLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].firstTriangleIndex", index));
	unsigned int numTrianglesLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].numTriangles", index));
	unsigned int boundsMinLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].boundsMin", index));
	unsigned int boundsMaxLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].boundsMax", index));
	unsigned int meshColorLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].material.color", index));
	unsigned int meshEmissionColorLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].material.emissionColor", index));
	unsigned int meshEmissionStrengthLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].material.emissionStrength", index));
	unsigned int meshSmoothnessLocation = GetShaderLocation(raytracingShader, TextFormat("meshes[%i].material.smoothness", index));

	SetShaderValueMatrix(raytracingShader, transformLocation, mesh->transform);

	SetShaderValue(raytracingShader, firstTrianglesLocation, &mesh->firstTriangleIndex, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, numTrianglesLocation, &mesh->numTriangles, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, boundsMinLocation, &mesh->boundsMin, SHADER_UNIFORM_VEC3);
	SetShaderValue(raytracingShader, boundsMaxLocation, &mesh->boundsMax, SHADER_UNIFORM_VEC3);

	Vector4 color = ColorToVector4(mesh->material.color);
	SetShaderValue(raytracingShader, meshColorLocation, &color, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector

	Vector4 emissionColor = ColorToVector4(mesh->material.emissionColor);
	SetShaderValue(raytracingShader, meshEmissionColorLocation, &emissionColor, SHADER_UNIFORM_VEC4);   // Set shader uniform value vector

	SetShaderValue(raytracingShader, meshEmissionStrengthLocation, &mesh->material.emissionStrength, SHADER_UNIFORM_FLOAT);
	SetShaderValue(raytracingShader, meshSmoothnessLocation, &mesh->material.smoothness, SHADER_UNIFORM_FLOAT);
}

void TracingEngine::UploadRaylibModel(Model* model, RaytracingMaterial material)
{
	for (int m = 0; m < model->meshCount; m++)
	{
		Mesh mesh = model->meshes[m];

		int firstTriIndex = (int)triangles.size();

		for (int i = 0; i < mesh.triangleCount; i++) {
			Triangle tri;

			// For each triangle, we have 3 indices (in an indexed mesh)
			int idx1 = mesh.indices[i * 3];
			int idx2 = mesh.indices[i * 3 + 1];
			int idx3 = mesh.indices[i * 3 + 2];

			// Assign positions from the vertices array
			tri.posA = *(Vector3*)&mesh.vertices[idx1 * 3] * model->transform;       // 3 floats per position
			tri.posB = *(Vector3*)&mesh.vertices[idx2 * 3] * model->transform;
			tri.posC = *(Vector3*)&mesh.vertices[idx3 * 3] * model->transform;

			Quaternion rotation = QuaternionIdentity();
			Vector3 position = Vector3(0, 0, 0);
			Vector3 scale = Vector3(1, 1, 1);
			MatrixDecompose(model->transform, &position, &rotation, &scale);

			// Assign normals from the normals array
			tri.normalA = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx1 * 3], rotation);      // 3 floats per normal
			tri.normalB = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx2 * 3], rotation);
			tri.normalC = Vector3RotateByQuaternion(*(Vector3*)&mesh.normals[idx3 * 3], rotation);

			triangles.push_back(tri);
		}

		RaytracingMesh rmesh = { model->transform, firstTriIndex, (int)triangles.size(), Vector3(1,1,1), Vector3(-1,-1,-1), material };

		TracingEngine::meshes.push_back(rmesh);

		UploadMesh(&rmesh, TracingEngine::meshes.size() - 1);
	}
}

void TracingEngine::UploadData(Camera* camera)
{
	float planeHeight = 0.01f * tan(camera->fovy * 0.5f * DEG2RAD) * 2;
	float planeWidth = planeHeight * (resolution.x / resolution.y);
	Vector3 viewParams = Vector3(planeWidth, planeHeight, 0.01f);
	SetShaderValue(raytracingShader, tracingParams.viewParams, &viewParams, SHADER_UNIFORM_VEC3);

	if (denoise)
	{
		numRenderedFrames++;
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

	UploadSpheres();
	UploadSky();
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
	ClearBackground(BLACK);

	DrawTextureRec(raytracingRenderTexture.texture, Rectangle(0, 0, (float)resolution.x, (float)-resolution.y), Vector2(0, 0), WHITE);

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

void TracingEngine::DrawDebug(Camera* camera)
{
	BeginMode3D(static_cast<Camera3D>(*camera));

	DrawGrid(10, 1);

	EndMode3D();

	DrawFPS(10, 10);
}

void TracingEngine::Unload()
{
	UnloadRenderTexture(raytracingRenderTexture);
	UnloadShader(postShader);
	UnloadShader(raytracingShader);
}