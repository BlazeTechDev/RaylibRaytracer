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

	TracingEngine::Initialize(Vector2(2048, 1024));

	TracingEngine::skyMaterial = SkyMaterial{ SKYBLUE, SKYBLUE, BROWN, ORANGE, Vector3(-0.5f, -1, -0.5f), 1, 0.5 };

	TracingEngine::spheres.push_back({ Vector3(-6, 1, 0), 1, {Vector4(1,1,1,1), Vector4(1,1,1,1), Vector4(0,0,0,0)} });
	TracingEngine::spheres.push_back({ Vector3(-3, 1, 0), 1, {Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,0,0,0)} });
	TracingEngine::spheres.push_back({ Vector3(0, 1, 0), 1, {Vector4(1,1,1,1), Vector4(0,0,0,0), Vector4(0,0,0,0)} });

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

	TracingEngine::Unload();

	CloseWindow();
}
