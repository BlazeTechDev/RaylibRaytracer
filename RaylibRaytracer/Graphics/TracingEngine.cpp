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

	Vector2 screenCenter = Vector2(resolution.x / 2.0f, resolution.y / 2.0f);
	SetShaderValue(raytracingShader, tracingParams.screenCenter, &screenCenter, SHADER_UNIFORM_VEC2);
	SetShaderValue(raytracingShader, tracingParams.resolution, &resolution, SHADER_UNIFORM_VEC2);

	SetShaderValue(raytracingShader, tracingParams.raysPerPixel, &raysPerPixel, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, tracingParams.maxBounces, &maxBounces, SHADER_UNIFORM_INT);
	SetShaderValue(raytracingShader, tracingParams.blur, &blur, SHADER_UNIFORM_FLOAT);

	sphereSSBO = rlLoadShaderBuffer(sizeof(SphereBuffer), NULL, RL_DYNAMIC_COPY);
	meshesSSBO = rlLoadShaderBuffer(sizeof(MeshBuffer), NULL, RL_DYNAMIC_COPY);
	trianglesSSBO = rlLoadShaderBuffer(sizeof(TriangleBuffer), NULL, RL_DYNAMIC_COPY);
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

void TracingEngine::UploadSSBOS()
{
	rlUpdateShaderBuffer(sphereSSBO, &sphereBuffer, sizeof(SphereBuffer), 0);
	rlUpdateShaderBuffer(meshesSSBO, &meshBuffer, sizeof(MeshBuffer), 0);
	rlUpdateShaderBuffer(trianglesSSBO, &triangleBuffer, sizeof(TriangleBuffer), 0);

	rlEnableShader(raytracingShader.id);
	rlBindShaderBuffer(sphereSSBO, 1);
	rlBindShaderBuffer(meshesSSBO, 2);
	rlBindShaderBuffer(trianglesSSBO, 3);
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

void TracingEngine::UploadRaylibModel(Model model, RaytracingMaterial material, bool indexed)
{
	for (int m = 0; m < model.meshCount; m++)
	{
		Mesh mesh = model.meshes[m];

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

		BoundingBox b = GetModelBoundingBox(model);
		RaytracingMesh rmesh = { firstTriIndex, (int)triangles.size(), 0, material, Vector4(b.min.x, b.min.y, b.min.z, 0), Vector4(b.max.x, b.max.y, b.max.z, 0)};

		TracingEngine::meshes.push_back(rmesh);
	}

	models.push_back(model);
}

void TracingEngine::UploadStaticData()
{
	UploadSpheres();
	UploadMeshes();
	UploadTriangles();
	UploadSky();
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

	for (size_t i = 0; i < models.size(); i++)
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		MatrixDecompose(models[i].transform, &position, &rotation, &scale);
		for (size_t j = 0; j < models[i].meshCount; j++)
		{
			BoundingBox box = GetMeshBoundingBox(models[i].meshes[j]);
			Vector3 dimentions = Vector3Subtract(box.min, box.max);
			DrawCubeWires(position, dimentions.x, dimentions.y, dimentions.z, RED);
		}
	}

	for (size_t i = 0; i < spheres.size(); i++)
	{
		DrawSphereWires(spheres[i].position, spheres[i].radius, 10, 10, RED);
	}

	DrawGrid(10, 1);

	EndMode3D();

	DrawFPS(10, 10);
}

void TracingEngine::Unload()
{
	UnloadRenderTexture(raytracingRenderTexture);
	UnloadShader(raytracingShader);
}