#pragma once

#include "IntegrationMapGeneratorPass.h"
#include "IrrandianceMapPass.h"
#include "RadianceMapGenerationPass.h"

#include "EnvironmentMap.h"

#include "RenderingEngine.h"
#include "ImageUtils.h"

class EnvironmentMapGenerator
{
public:
	struct Options
	{
		VkExtent2D radianceMapDimensions;
		size_t randianceMapMipCount;

		VkExtent2D irradianceMapDimensions;
		VkExtent2D integrationMapDimensions;
	};

	struct SaveOptions
	{
		std::string_view radianceMap;
		std::string_view irradianceMap;
		std::string_view integration;

		EnvironmentMapGenerator::Options environmentMapOptions;
	};

	struct LoadOptions
	{
		std::string_view radianceMap;
		size_t randianceMapMipCount;
		std::string_view irradianceMap;
		std::string_view integration;
	};

public:
	EnvironmentMap GenerateEnviromentMap(RenderingEngine& renderingEngine, Image source, const Options& options);

	EnvironmentMap LoadEnviromentMap(RenderingEngine& renderingEngine, const LoadOptions& loadOptions);
	void SaveEnvironmentMap(const GraphicsAPI& grpahicsAPI, CommandPool& commandPool, EnvironmentMap& environmentMap, const SaveOptions& saveOptions);

	void CleanUp(VkDevice device);
private:
	Image LoadCubeMapWithMipLevels(const GraphicsAPI& grpahicsAPI, CommandPool& commandPool, std::string_view folder, size_t mipLevels);
	Image LoadIntegrationMap(const GraphicsAPI& grpahicsAPI, CommandPool& commandPool, std::string_view filename);

	IrrandianceMapPass irradianceMapPass;
	RadianceMapGenerationPass radianceMapGenerationPass;
	IntegrationMapGeneratorPass integrationMapGeneratorPass;
};

