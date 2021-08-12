#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"

#include <string>

class ImageData
{
public:
	static void InitialiseLoaders();
	static ImageData LoadImageFromFile(const std::string_view& filename, size_t channelCount = 4, size_t channelSize = 1);
	static ImageData CreateImageOfSize(int width, int height, int channels);
	static void SaveImageToFile(const std::string_view& filename, const ImageData image);

	void FreeImageData();

	int GetWidth() const;
	int GetHeight() const;
	int GetChannelCount() const;

	size_t GetImageSize() const;

	uint8_t* GetDataPtr();
	const uint8_t* GetDataPtr() const;

private:
	int width;
	int height;
	int channelCount;
	uint8_t* data = nullptr;

	enum class ImageDataType
	{
		DDS,
		Manual,
		Other
	} imageDataType;
	uint32_t ddsImageId = 0;
};

