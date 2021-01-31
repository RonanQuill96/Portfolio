#include "Camera.h"
#include "ImageData.h"
#include "Material.h"
#include "Ray.h"
#include "Util.h"
#include "Scenes.h"
#include "Vector3.h"

#include "ThreadPool.h"

constexpr int imageWidth = 1920;
constexpr int imageHeight = 1080;
constexpr int sampleCount = 100;

ImageData<imageWidth, imageHeight> imageData;

Vector3 Colour(const Ray& r, Vector3 background, Hittable* world, int depth)
{
	HitRecord hitRecord;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
	{
		return Vector3(0, 0, 0);
	}

	// If the ray hits nothing, return the background color.
	if (!world->Hit(r, 0.001f, std::numeric_limits<float>::max(), hitRecord))
	{
		return background;
	}

	Ray scattered;
	Vector3 attenuation;
	Vector3 emitted = hitRecord.materialPtr->Emitted(hitRecord.u, hitRecord.v, hitRecord.p);

	if (!hitRecord.materialPtr->Scatter(r, hitRecord, attenuation, scattered))
	{
		return emitted;
	}		

	return emitted + attenuation * Colour(scattered, background, world, depth - 1);
}

void RayTracePixel(const size_t x, const size_t y, const Vector3 background, Hittable* world, const Camera& camera, size_t maxBounces)
{
	Vector3 colour(0.0f, 0.0f, 0.0f);

	for (int s = 0; s < sampleCount; s++)
	{
		const float u = static_cast<float>(x + Util::RandomFloat()) / static_cast<float>(imageWidth);
		const float v = static_cast<float>(y + Util::RandomFloat()) / static_cast<float>(imageHeight);

		const Ray r = camera.GetRay(u, v);

		colour += Colour(r, background, world, maxBounces);
	}

	imageData.Write(colour, x, imageHeight-1 - y);
}

int main()
{
	ThreadPool threadPool(std::thread::hardware_concurrency());

	size_t sceneOption = 0;

	size_t maxBounces = 50;

	Hittable* world;

	Vector3 lookfrom;
	Vector3 lookat;
	float vfov;
	float dist_to_focus = 10.0f;
	float aperture = 0.0f;
	Vector3 background;

	switch (sceneOption)
	{
	case 0:
		lookfrom = Vector3(13.0f, 2.0f, 30.0f);
		lookat = Vector3(0.0f, 0.0f, 0.0f);
		dist_to_focus = 10.0f;
		aperture = 0.0f;
		vfov = 20.0f;
		background = Vector3(0.70f, 0.80f, 1.00f);
		world = Scenes::RandomScene();
		break;

	case 1:
		lookfrom = Vector3(278.0f, 278.0f, -800.0f);
		lookat = Vector3(278.0f, 278.0f, 0.0f);
		vfov = 40.0f;
		aperture = 0.0f;
		dist_to_focus = 10.0f;
		background = Vector3(0.0f, 0.0f, 0.0f);
		world = Scenes::CornellBox();
		break;

	case 2:
		lookfrom = Vector3(13.0f, 2.0f, 3.0f);
		lookat = Vector3(0.0f, 0.0f, 0.0f);
		dist_to_focus = 10.0f;
		aperture = 0.0f;
		vfov = 20.0f;
		world = Scenes::TwoPerlinSpheres();
		break;
	}

	Camera camera(lookfrom, lookat, Vector3(0.0f, 1.0f, 0.0f), vfov, float(imageWidth) / float(imageHeight), aperture, dist_to_focus, 0.0f, 1.0f);

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	for (int y = imageHeight - 1; y >= 0; y--)
	{
		for (int x = 0; x < imageWidth; x++)
		{
			threadPool.AddTask(RayTracePixel, x, y, background, world, camera, maxBounces);
		}
	}
	threadPool.Stop(true);

	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

	std::cout << duration << std::endl;

	imageData.WriteImageDataToFile("render.ppm", sampleCount);
}