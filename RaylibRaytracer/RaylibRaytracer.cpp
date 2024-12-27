// RaylibRaytracer.cpp : Defines the entry point for the application.
//

#include "RaylibRaytracer.h"
#include "Graphics/TracingEngine.h"

#include <raymath.h>
#include <raylib.h>

using namespace std;

int main()
{
	InitWindow(2048, 1024, "raylib raytracer");
	SetTargetFPS(80);

	float deltaTime = 0;

	Camera camera = Camera();
	camera.position = Vector3(15, 8, 15);
	camera.target = Vector3(0, 0.5f, 0);
	camera.up = Vector3(0, 1, 0);
	camera.fovy = 45;
	camera.projection = CAMERA_PERSPECTIVE;

	DisableCursor();

	TracingEngine::Initialize(Vector2(2048, 1024), 10, 10, 0.0005f);

	TracingEngine::skyMaterial = SkyMaterial{ WHITE, SKYBLUE, BROWN, WHITE, Vector3(-0.5f, -1, -0.5f), 1, 0.5 };

	RaytracingMaterial red = { Vector4(1,1,1,1), Vector4(1,0,0,10), Vector4(0,0,0,0) };
	RaytracingMaterial red2 = { Vector4(1,0.5f,0.5f,1), Vector4(0,0,0,0), Vector4(0,0,0,0) };
	RaytracingMaterial green = { Vector4(1,1,1,1), Vector4(0,0,1,10), Vector4(0,0,0,0) };
	RaytracingMaterial blue = { Vector4(1,1,1,1), Vector4(0,1,0,10), Vector4(0,0,0,0) };
	RaytracingMaterial white = { Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,0,0,0) };
	RaytracingMaterial metal = { Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,1,0,0) };

	TracingEngine::spheres.push_back({ Vector3(0, 4, 0), 0.2, red });
	TracingEngine::spheres.push_back({ Vector3(2, 2, 0), 0.2, green });
	TracingEngine::spheres.push_back({ Vector3(-2, 2, 0), 0.2, blue });
	TracingEngine::spheres.push_back({ Vector3(-5, 2, 0), 1, white });

	Model monkey = LoadModel("resources/meshes/stanford_dragon.obj");
	monkey.transform = MatrixTranslate(0, 0.1f, 0);
	TracingEngine::UploadRaylibModel(monkey, white, false, 14);

	Model monkey2 = LoadModel("resources/meshes/monkey.obj");
	monkey2.transform = MatrixTranslate(0, 1, 5);
	TracingEngine::UploadRaylibModel(monkey2, white, false, 4);

	Model floor = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	TracingEngine::UploadRaylibModel(floor, red2, true, 0);
	
	TracingEngine::UploadStaticData();

	while (!WindowShouldClose())
	{
		UpdateCamera(&camera, CAMERA_FREE);


		TracingEngine::UploadData(&camera);

		if (IsKeyPressed(KEY_ONE)) TracingEngine::debug = !TracingEngine::debug;
		if (IsKeyPressed(KEY_R)) TracingEngine::denoise = !TracingEngine::denoise;
		if (IsKeyPressed(KEY_P)) TracingEngine::pause = !TracingEngine::pause;

		TracingEngine::Render(&camera);

		deltaTime += GetFrameTime();
	}
	
	UnloadModel(monkey2);
	UnloadModel(monkey);
	UnloadModel(floor);

	TracingEngine::Unload();

	CloseWindow();
}

