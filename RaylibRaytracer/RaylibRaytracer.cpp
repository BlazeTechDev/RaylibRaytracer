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

	TracingEngine::Initialize(Vector2(2048, 1024), 7, 10, 0.001f);

	TracingEngine::skyMaterial = SkyMaterial{ WHITE, SKYBLUE, BROWN, WHITE, Vector3(-0.5f, -1, -0.5f), 1, 0.5 };

	RaytracingMaterial red = { Vector4(1,1,1,1), Vector4(1,0,0,10), Vector4(0,0,0,0) };
	RaytracingMaterial red2 = { Vector4(1,0.6f,0.6f,0), Vector4(0,0,0,0), Vector4(0,0,0,0) };
	RaytracingMaterial green = { Vector4(1,1,1,1), Vector4(0,0,1,10), Vector4(0,0,0,0) };
	RaytracingMaterial blue = { Vector4(1,1,1,1), Vector4(0,1,0,10), Vector4(0,0,0,0) };
	RaytracingMaterial white = { Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,0,0,0) };
	RaytracingMaterial grey = { Vector4(0.5f,0.5f,0.5f,1), Vector4(0,0,0,0), Vector4(0,0,0,0) };
	RaytracingMaterial light = { Vector4(1,0.8f,0.7f,1), Vector4(1,1,1,1.2f), Vector4(0,0,0,0) };
	RaytracingMaterial metal = { Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,1,0,0) };

	Model dragon = LoadModel("resources/meshes/monkey.obj");
	dragon.transform = MatrixTranslate(0, 1, -1);
	TracingEngine::UploadRaylibModel(dragon, red2, false, 7);

	Model floor = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	TracingEngine::UploadRaylibModel(floor, white, true, 0);

	Model wall = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	wall.transform = MatrixRotateX(PI / 2) * MatrixTranslate(0, 0, -2);
	TracingEngine::UploadRaylibModel(wall, white, true, 0);

	Model ceiling = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	ceiling.transform = MatrixRotateX(PI) * MatrixTranslate(0, 3, 0);
	TracingEngine::UploadRaylibModel(ceiling, white, true, 0);

	Model lighting = LoadModelFromMesh(GenMeshCube(2, 1, 2));
	lighting.transform = MatrixTranslate(0, 3, 0);
	TracingEngine::UploadRaylibModel(lighting, light, true, 0);

	Model left = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	left.transform = MatrixRotateZ(-PI / 2) * MatrixTranslate(-2, 0, 0);
	TracingEngine::UploadRaylibModel(left, white, true, 0);

	Model right = LoadModelFromMesh(GenMeshPlane(50, 50, 1, 1));
	right.transform = MatrixRotateZ(PI / 2) * MatrixTranslate(2, 0, 0);
	TracingEngine::UploadRaylibModel(right, white, true, 0);

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
	
	UnloadModel(dragon);
	UnloadModel(floor);
	UnloadModel(wall);
	UnloadModel(left);
	UnloadModel(right);
	UnloadModel(ceiling);
	UnloadModel(lighting);

	TracingEngine::Unload();

	CloseWindow();
}

