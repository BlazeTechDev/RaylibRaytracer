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

	TracingEngine::spheres.push_back({ Vector3(-6, 1, 0), 1, {WHITE, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(-3, 1, 0), 1, {WHITE, WHITE, 0, 0.2f} });
	TracingEngine::spheres.push_back({ Vector3(0, 1, 0), 1, {WHITE, WHITE, 0, 0.4f} });
	TracingEngine::spheres.push_back({ Vector3(3, 1, 0), 1, {WHITE, WHITE, 0, 0.6f} });
	TracingEngine::spheres.push_back({ Vector3(6, 1, 0), 1, {WHITE, WHITE, 0, 0.8f} });
	TracingEngine::spheres.push_back({ Vector3(9, 1, 0), 1, {WHITE, WHITE, 0, 1} });

	TracingEngine::spheres.push_back({ Vector3(0, 3, 0), 1, {WHITE, WHITE, 1.2f, 0.3f} });
	TracingEngine::spheres.push_back({ Vector3(-3, 3, 0), 1, {WHITE, RED, 1.2f, 0.3f} });
	TracingEngine::spheres.push_back({ Vector3(3, 3, 0), 1, {WHITE, GREEN, 1.2f, 0.3f} });

	TracingEngine::spheres.push_back({ Vector3(-6, 1, 3), 1, {RED, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(-3, 1, 3), 1, {BLUE, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(0, 1, 3), 1, {GREEN, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(3, 1, 3), 1, {GRAY, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(6, 1, 3), 1, {ORANGE, WHITE, 0, 0} });
	TracingEngine::spheres.push_back({ Vector3(9, 1, 3), 1, {PURPLE, WHITE, 0, 0} });

	Mesh cube = GenMeshPlane(50,50,1,1);
	Model model = LoadModelFromMesh(cube);

	Mesh wall = GenMeshPlane(50, 25, 1, 1);
	Model model2 = LoadModelFromMesh(wall);

	model2.transform = MatrixTranslate(0, -2, 0) * MatrixRotateX(PI / 2);

	Mesh mirror = GenMeshPlane(50, 25, 1, 1);
	Model model3 = LoadModelFromMesh(mirror);

	model3.transform = MatrixTranslate(0, -12, 10) * MatrixRotateZ(PI / 2);

	TracingEngine::UploadRaylibModel(&model, { GRAY, WHITE, 0, 0.1f});
	TracingEngine::UploadRaylibModel(&model2, { WHITE, WHITE, 0, 0 });

	TracingEngine::UploadTriangles();

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
