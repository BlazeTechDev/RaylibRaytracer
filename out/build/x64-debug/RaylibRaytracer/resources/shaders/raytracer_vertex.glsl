#version 330

layout(std430, binding = 3) buffer sphereBuffer
{
	Sphere spheres[];
};

out buffer pass_sphereBuffer;

void main()
{
	pass_sphereBuffer = sphereBuffer;
}