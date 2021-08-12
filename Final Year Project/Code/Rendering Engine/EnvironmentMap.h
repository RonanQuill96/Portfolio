#pragma once

#include "VulkanIncludes.h"
#include "ImageUtils.h"

class EnvironmentMap
{
public:
	Image raddianceMap;
	Image irraddianceMap;
	Image integrationMap;
};