#include "Scenes.h"

#include "Box.h"
#include "BVHNode.h"
#include "CheckerTexture.h"
#include "ConstantColour.h"
#include "Dialectric.h"
#include "DiffuseLight.h"
#include "Lambertian.h"
#include "Material.h"
#include "Metal.h"
#include "MovingSphere.h"
#include "NoiseTexture.h"
#include "HittableList.h"
#include "Sphere.h"
#include "XYRectangle.h"
#include "XZRectangle.h"
#include "YZRectangle.h"
#include "InstanceYRotation.h"
#include "InstanceTranslation.h"
#include "Util.h"

Hittable* Scenes::RandomScene()
{
    int n = 500;
    Hittable** list = new Hittable * [n + 1];

    Texture* checker = new CheckerTexture(new ConstantColour(Vector3(0.2f, 0.3f, 0.1f)), new ConstantColour(Vector3(0.9f, 0.9f, 0.9f)));
    list[0] = new Sphere(Vector3(0.0f, -1000.0f, 0.0f), 1000, new Lambertian(checker));

    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float chooseMaterial = Util::RandomFloat();
            Vector3 center(a + 0.9f * Util::RandomFloat(), 0.2f, b + 0.9f * Util::RandomFloat());
            if ((center - Vector3(4.0f, 0.0f, 2.0f)).Length() > 0.9f)
            {
                if (chooseMaterial < 0.8f) //diffuse
                {
                    //list[i++] = new MovingSphere(center, center + Vector3(0, 0.5 * Util::RandomFloat(), 0), 0.0, 1.0, 0.2, new Lambertian(new ConstantColour(Vector3(Util::RandomFloat() * Util::RandomFloat(), Util::RandomFloat() * Util::RandomFloat(), Util::RandomFloat() * Util::RandomFloat()))));
                    list[i++] = new Sphere(center, 0.2f, new Lambertian(new ConstantColour(Vector3(Util::RandomFloat() * Util::RandomFloat(), Util::RandomFloat() * Util::RandomFloat(), Util::RandomFloat() * Util::RandomFloat()))));
                }
                else if (chooseMaterial < 0.95f) //metal
                {
                    list[i++] = new Sphere(center, 0.2f, new Metal(Vector3(0.5f * (1.0f + Util::RandomFloat()), 0.5f * (1.0f + Util::RandomFloat()), 0.5f * (1.0f + Util::RandomFloat())), 0.5f * Util::RandomFloat()));
                }
                else //glass
                {
                    list[i++] = new Sphere(center, 0.2f, new Dialectric(1.5f));
                }
            }
        }
    }

    list[i++] = new Sphere(Vector3(0.0f, 1.0f, 0.0f), 1.0f, new Dialectric(1.5f));
    list[i++] = new Sphere(Vector3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(new ConstantColour(Vector3(0.4f, 0.2f, 0.1f))));
    list[i++] = new Sphere(Vector3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vector3(0.7f, 0.6f, 0.5f), 0.0f));

    return new BVHNode(list, i, 0.0f, 1.0f);
}

Hittable* Scenes::TwoPerlinSpheres()
{
    constexpr int listSize = 4;

    Texture* perlinNoiseTexture = new NoiseTexture(1.0);

    Hittable** list = new Hittable * [listSize];
    list[0] = new Sphere(Vector3(0.0f, 2.0f, 0.0f), 2.0f, new Lambertian(perlinNoiseTexture));
    list[1] = new Sphere(Vector3(0.0f, -1000, 0.0f), 1000.0f, new Lambertian(perlinNoiseTexture));
    list[2] = new Sphere(Vector3(0.0f, 7.0f, 0.0f), 2.0f, new DiffuseLight(new ConstantColour(Vector3(4.0f, 4.0f, 4.0f))));
    list[3] = new XYRectangle(3.0f, 5.0f, 1.0f, 3.0f, -2.0f, new DiffuseLight(new ConstantColour(Vector3(4.0f, 4.0f, 4.0f))));

    return new HittableList(list, listSize);
}

Hittable* Scenes::CornellBox()
{
    constexpr int listSize = 8;

    int i = 0;
    Material* red = new Lambertian(new ConstantColour(Vector3(0.65f, 0.05f, 0.05f)));
    Material* white = new Lambertian(new ConstantColour(Vector3(0.73f, 0.73f, 0.73f)));
    Material* green = new Lambertian(new ConstantColour(Vector3(0.12f, 0.45f, 0.15f)));
    Material* light = new DiffuseLight(new ConstantColour(Vector3(15.0f, 15.0f, 15.0f)));

    Hittable** list = new Hittable * [listSize];
    list[i++] = new YZRectangle(0.0f, 555.0f, 0.0f, 555.0f, 555.0f, green);
    list[i++] = new YZRectangle(0.0f, 555.0f, 0.0f, 555.0f, 0.0f, red);
    list[i++] = new XZRectangle(213.0f, 343.0f, 227, 332.0f, 554.0f, light);
    list[i++] = new XZRectangle(0.0f, 555.0f, 0.0f, 555.0f, 0.0f, white);
    list[i++] = new XZRectangle(0.0f, 555.0f, 0.0f, 555.0f, 555.0f, white);
    list[i++] = new XYRectangle(0.0f, 555.0f, 0.0f, 555.0f, 555.0f, white);

    Hittable* box1 = new Box(Vector3(0.0f, 0.0f, 0.0f), Vector3(165.0f, 330.0f, 165.0f), white);
    box1 = new InstanceYRotation(box1, 15.0f);
    box1 = new InstanceTranslation(box1, Vector3(265.0f, 0.0f, 295.0f));
    list[i++] = box1;

    Hittable* box2 = new Box(Vector3(0.0f, 0.0f, 0.0f), Vector3(165.0f, 165.0f, 165.0f), white);
    box2 = new InstanceYRotation(box2, -18.0f);
    box2 = new InstanceTranslation(box2, Vector3(130.0f, 0.0f, 65.0f));
    list[i++] = box2;

    return new HittableList(list, listSize);
}
