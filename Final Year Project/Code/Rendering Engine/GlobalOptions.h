#pragma once

enum class GBufferDisplayMode
{
	NONE = 0,
	ALBEDO = 1,
	METALNESS = 2,
	ROUGHNESS = 3,
	NORMALS = 4
};

class GlobalOptions
{
public:
	static GlobalOptions& GetInstace()
	{
		static GlobalOptions theInstance;
		return theInstance;
	}

	bool environmentLight = true;

	GBufferDisplayMode gBufferDisplayMode = GBufferDisplayMode::NONE;

	int deferredLightingLimit = 100;

	float cameraSpeed = 10.0;
};
