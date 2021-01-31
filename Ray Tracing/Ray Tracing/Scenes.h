#pragma once

class Hittable;

namespace Scenes
{
	Hittable* RandomScene();
	Hittable* TwoPerlinSpheres();
	Hittable* CornellBox();
}

