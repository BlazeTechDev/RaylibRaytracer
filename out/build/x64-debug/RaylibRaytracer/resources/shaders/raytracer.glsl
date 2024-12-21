#version 330

uniform vec3 viewParams;
uniform vec2 resolution;

uniform vec3 cameraPosition;
uniform vec3 cameraDirection;
uniform vec2 screenCenter;

out vec4 out_color;

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct RayTracingMaterial
{
	vec4 color;
};

struct Sphere
{
	vec3 position;
	float radius;
	RayTracingMaterial material;
};

struct HitInfo
{
	bool didHit;
	float distance;
	vec3 hitPoint;
	vec3 hitNormal;
};

vec3 CalcRayDir(vec2 nCoord) {
	vec3 horizontal = normalize(cross(cameraDirection, vec3(.0, 1.0, .0)));
	vec3 vertical = normalize(cross(horizontal, cameraDirection));
	return normalize(cameraDirection + horizontal * nCoord.x + vertical * nCoord.y);
}

mat3 setCamera()
{
	vec3 cw = normalize(cameraDirection);
	vec3 cp = vec3(0.0, 1.0, 0.0);
	vec3 cu = normalize(cross(cw, cp));
	vec3 cv = (cross(cu, cw));
	return mat3(cu, cv, cw);
}

HitInfo RaySphere(Ray ray, vec3 center, float radius)
{
	HitInfo hitInfo;
	vec3 offsetRayOrigin = ray.origin - center;

	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(offsetRayOrigin, ray.direction);
	float c = dot(offsetRayOrigin, offsetRayOrigin) - (radius * radius);

	float discriminant = b * b - 4.0 * a * c;

	if (discriminant >= 0.0)
	{
		float distance = (-b - sqrt(discriminant)) / (2.0 * a);

		if (distance >= 0.0)
		{
			hitInfo.didHit = true;
			hitInfo.distance = distance;
			hitInfo.hitPoint = ray.origin + (ray.direction * distance);
			hitInfo.hitNormal = normalize(hitInfo.hitPoint - center);
		}
	}
	else
	{
		hitInfo.didHit = false;
	}

	return hitInfo;
}

HitInfo CalculateRayCollision(Ray ray)
{
	HitInfo closestHit;
	closesHit.didHit = false;

	for (int i = 0; i < numSpheres; i++)
	{
		Sphere sphere = spheres[i];
		HitInfo hitInfo = RaySphere(ray, sphere.position, sphere.radius);

		if (hitInfo.didHit && hitInfo.distance < sphere.radius)
		{
			closestHit = hitInfo;
			closestHit.material = sphere.material;
		}
	}

	return closestHit;
}

void main()
{
	vec2 nCoord = (gl_FragCoord.xy - screenCenter.xy) / screenCenter.y;
	mat3 cameraMatrix = setCamera();

	float focalLength = length(cameraDirection);
	vec3 rayDirection = cameraMatrix * normalize(vec3(nCoord, focalLength));

	Ray ray;
	ray.origin = cameraPosition;
	ray.direction = rayDirection;

	if (RaySphere(ray, vec3(0,0,0), 1).didHit)
	{
		out_color = vec4(1);
	}
	else
	{
		out_color = vec4(ray.direction, 1);
	}
}