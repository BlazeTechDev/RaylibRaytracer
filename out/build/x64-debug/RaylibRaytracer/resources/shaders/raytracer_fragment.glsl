#version 330

in vec2 fragTexCoord;

uniform vec3 viewParams;
uniform vec2 resolution;

uniform vec3 cameraPosition;
uniform vec3 cameraDirection;
uniform vec2 screenCenter;

uniform sampler2D texture0;

uniform int numRenderedFrames;

uniform bool denoise;
uniform bool pause;

struct SkyMaterial
{
	vec4 skyColorZenith;
	vec4 skyColorHorizon;
	vec4 groundColor;
	vec4 sunColor;
	vec3 sunDirection;
	float sunFocus;
	float sunIntensity;
};

struct RayTracingMaterial
{
	vec4 color;
	vec4 emissionColor;
	float emissionStrength;
	float smoothness;
};

struct Sphere
{
	vec3 position;
	float radius;
	RayTracingMaterial material;
};

#define NR_SPHERES 40  
uniform Sphere spheres[NR_SPHERES];

struct Triangle
{
	vec3 posA;
	vec3 posB;
	vec3 posC;
	vec3 normalA;
	vec3 normalB;
	vec3 normalC;
};

#define NR_MAX_TRIANGLES 10

uniform Triangle triangles[NR_MAX_TRIANGLES];

struct Mesh
{
	mat4 transform;
	int firstTriangleIndex;
	int numTriangles;
	vec3 boundsMin;
	vec3 boundsMax;
	RayTracingMaterial material;
};

#define NR_MAX_MESHES 3

uniform Mesh meshes[NR_MAX_MESHES];

#define NR_MAX_BOUNCES 5
#define NR_NUM_RAYS_PER_PIXEL 5


uniform SkyMaterial skyMaterial;

out vec4 out_color;

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct HitInfo
{
	bool didHit;
	float distance;
	vec3 hitPoint;
	vec3 hitNormal;
	RayTracingMaterial material;
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

HitInfo RayTriangle(Ray ray, Triangle tri)
{
	vec3 edgeAB = tri.posB - tri.posA;
	vec3 edgeAC = tri.posC - tri.posA;
	vec3 normalVector = cross(edgeAB, edgeAC);
	vec3 ao = ray.origin - tri.posA;
	vec3 dao = cross(ao, ray.direction);

	float determinant = -dot(ray.direction, normalVector);
	float invDet = 1 / determinant;

	float dst = dot(ao, normalVector) * invDet;
	float u = dot(edgeAC, dao) * invDet;
	float v = -dot(edgeAB, dao) * invDet;
	float w = 1 - u - v;

	HitInfo hitInfo;
	hitInfo.didHit = determinant >= 1E-6 && dst >= 0 && u >= 0 && v >= 0 && w >= 0;
	hitInfo.hitPoint = ray.origin + ray.direction * dst;
	hitInfo.hitNormal = normalize(tri.normalA * w + tri.normalB * u + tri.normalC * v);
	hitInfo.distance = dst;
	return hitInfo;
}

HitInfo RaySphere(Ray ray, vec3 center, float radius)
{
	HitInfo hitInfo;
	hitInfo.didHit = false;
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

	return hitInfo;
}

HitInfo CalculateRayCollision(Ray ray)
{
	HitInfo closestHit;
	closestHit.didHit = false;

	closestHit.distance = 100000000;

	for (int i = 0; i < NR_SPHERES; i++)
	{
		Sphere sphere = spheres[i];
		HitInfo hitInfo = RaySphere(ray, sphere.position, sphere.radius);

		if (hitInfo.didHit && hitInfo.distance < closestHit.distance)
		{
			closestHit = hitInfo;
			closestHit.material = sphere.material;
		}
	}

	for (int i = 0; i < NR_MAX_MESHES; i++)
	{
		Mesh mesh = meshes[i];

		for (int t = 0; t < mesh.numTriangles; t++)
		{
			int triIndex = mesh.firstTriangleIndex + t;
			Triangle tri = triangles[triIndex];

			HitInfo hitInfo = RayTriangle(ray, tri);

			if (hitInfo.didHit && hitInfo.distance < closestHit.distance)
			{
				closestHit = hitInfo;
				closestHit.material = mesh.material;
			}
		}
	}

	return closestHit;
}

float random(inout int state)
{
	state = state * 747796405 + 2891336453;
	int result = ((state >> ((state) >> 28) + 4) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return result / 4294967295.0;
}

float randomNormalDistribution(inout int state)
{
	float theta = 2 * 3.1415926 * random(state);
	float rho = sqrt(-2 * log(random(state)));
	return rho * cos(theta);
}

vec3 randomDirection(inout int state)
{
	float x = randomNormalDistribution(state);
	float y = randomNormalDistribution(state);
	float z = randomNormalDistribution(state);
	return normalize(vec3(x, y, z));
}

vec3 randomHemisphereDirection(vec3 normal, inout int state)
{
	vec3 dir = randomDirection(state);
	return dir * sign(dot(normal, dir));
}

vec3 getEnvironmentLight(Ray ray)
{
	float skyGradientT = pow(smoothstep(0, 0.4, ray.direction.y), 0.35);
	vec3 skyGradient = mix(skyMaterial.skyColorHorizon.rgb, skyMaterial.skyColorZenith.rgb, skyGradientT);
	float sun = pow(max(0, dot(ray.direction, -skyMaterial.sunDirection)), skyMaterial.sunFocus) * skyMaterial.sunIntensity;

	float groundToSkyT = smoothstep(-0.01, 0, ray.direction.y);
	float sunMask = float(int(groundToSkyT >= 1));
	return mix(skyMaterial.groundColor.rgb, skyGradient, groundToSkyT) + sun * sunMask * skyMaterial.sunColor.rgb;
}

vec3 trace(Ray ray, inout int rngState)
{
	vec3 incomingLight = vec3(0);
	vec3 rayColor = vec3(1);

	vec3 debugNormal = vec3(0);

	for (int i = 0; i <= NR_MAX_BOUNCES; i++)
	{
		HitInfo hitInfo = CalculateRayCollision(ray);
		if (hitInfo.didHit)
		{
			ray.origin = hitInfo.hitPoint;
			vec3 specularDirection = reflect(ray.direction, hitInfo.hitNormal);
			vec3 diffuseDirection = normalize(hitInfo.hitNormal + randomDirection(rngState));
			ray.direction = mix(diffuseDirection, specularDirection, hitInfo.material.smoothness);

			RayTracingMaterial material = hitInfo.material;
			vec3 emittedLight = material.emissionColor.rgb * material.emissionStrength;
			
			incomingLight += emittedLight * rayColor;
			rayColor *= material.color.rgb;

			debugNormal = hitInfo.hitNormal;
		}
		else
		{
			incomingLight += getEnvironmentLight(ray) * rayColor;
			break;
		}
	}

	return incomingLight;
}

Ray offsetRay(Ray ray, float offsetStrength, inout int rngState)
{
	ray.direction += randomDirection(rngState) * offsetStrength;
	return ray;
}

vec3 drawFrame(Ray ray, inout int rngState)
{
	vec3 total = vec3(0);

	for (int i = 0; i < NR_NUM_RAYS_PER_PIXEL; i++)
	{
		total += trace(offsetRay(ray, 0.0005f, rngState), rngState);
	}

	return total / NR_NUM_RAYS_PER_PIXEL;
}

void main()
{
	vec2 UV = gl_FragCoord.xy / resolution;

	vec2 nCoord = (gl_FragCoord.xy - screenCenter.xy) / screenCenter.y;
	mat3 cameraMatrix = setCamera();

	float focalLength = length(cameraDirection);
	vec3 rayDirection = cameraMatrix * normalize(vec3(nCoord, focalLength));

	Ray ray;
	ray.origin = cameraPosition;
	ray.direction = rayDirection;

	int pixelIndex = int(gl_FragCoord.y * gl_FragCoord.x);

	int rngState = pixelIndex + numRenderedFrames * 719393;

	vec3 render = drawFrame(ray, rngState);

	float weight = 1.0 / (numRenderedFrames + 1);
	vec3 accumulatedAverage = vec3(1);

	if (denoise)
	{
		if (!pause)
		{
			accumulatedAverage = ((texture(texture0, fragTexCoord).xyz * (1 - weight)) + (render * weight));
		}
		else
		{
			accumulatedAverage = texture(texture0, fragTexCoord).xyz;
		}

		out_color = vec4(accumulatedAverage, 1);
	}
	else
	{
		out_color = vec4(render, 1);
	}
}