#include "ImageData.h"

#include "Buffer.h"
#include "ImageUtils.h"

#include <CodeAnalysis/Warnings.h>
#pragma warning(push)  
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS )

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef _UNICODE
    #undef  _UNICODE
    #include <IL/il.h>
    #define _UNICODE
#else
    #include <IL/il.h>
#endif // _UNICODE

#pragma warning(pop)

#include <stdexcept>
#include <string>

#include <filesystem>

void ImageData::InitialiseLoaders()
{
    ilInit();
}

ImageData ImageData::LoadImageFromFile(const std::string_view& filename, size_t channelCount, size_t channelSize)
{
    ImageData loadedImageData;

    std::filesystem::path path(filename);

    if (path.extension() == ".dds")
    {
        loadedImageData.imageDataType = ImageDataType::DDS;

        uint8_t* data = nullptr;

        ILuint imgID = 0;
        ilGenImages(1, &imgID);
        ilBindImage(imgID);

        loadedImageData.ddsImageId = imgID;

        ILboolean success = ilLoadImage(filename.data());

        //Image loaded successfully
        if (success == IL_TRUE)
        {
            //Convert image to RGBA
            success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
            loadedImageData.data = ilGetData();
            loadedImageData.width = ilGetInteger(IL_IMAGE_WIDTH);
            loadedImageData.height = ilGetInteger(IL_IMAGE_HEIGHT);
            loadedImageData.channelCount = ilGetInteger(IL_IMAGE_CHANNELS);

            //imageData.channelCount = 4;
        }
    }
    else
    {
        loadedImageData.imageDataType = ImageDataType::Other;

        loadedImageData.data = stbi_load(filename.data(), &loadedImageData.width, &loadedImageData.height, &loadedImageData.channelCount, STBI_rgb_alpha);
        loadedImageData.channelCount = 4;

        if (loadedImageData.data == nullptr)
        {
            throw std::runtime_error("Could not load " + std::string(filename));
        }
    }

    //Convert
    if (channelCount != loadedImageData.channelCount)
    {
        //Drop unwanted channels
        ImageData convertedImageData = CreateImageOfSize(loadedImageData.width, loadedImageData.height, channelCount);

        for (size_t y = 0; y < loadedImageData.height; y++)
        {
            for (size_t x = 0; x < loadedImageData.width; x++)
            {
                unsigned loadedImageBytePerPixel = loadedImageData.GetChannelCount();
                unsigned char* originalPixels = loadedImageData.GetDataPtr() + (x + loadedImageData.GetHeight() * y) * loadedImageBytePerPixel;

                unsigned convertedImageBytePerPixel = convertedImageData.GetChannelCount();
                unsigned char* coonvertedPixels = convertedImageData.GetDataPtr() + (x + convertedImageData.GetHeight() * y) * convertedImageBytePerPixel;

                for (int c = 0; c < convertedImageData.GetChannelCount(); c++)
                {
                    coonvertedPixels[c] = originalPixels[c];
                }
            }
        }

        loadedImageData.FreeImageData();

        return convertedImageData;
    }
    else
    {
        return loadedImageData;
    }

}

ImageData ImageData::CreateImageOfSize(int width, int height, int channels)
{
    ImageData image;
    image.imageDataType = ImageDataType::Manual;
    image.width = width;
    image.height = height;
    image.channelCount = channels;
    image.data = new uint8_t[width * height * channels];
    return image;
}

void ImageData::SaveImageToFile(const std::string_view& filename, const ImageData image)
{
    std::filesystem::path path(filename);

    if (path.extension() == ".png")
    {
        stbi_write_png(filename.data(), image.GetWidth(), image.GetHeight(), image.GetChannelCount(), image.GetDataPtr(), image.GetWidth() * image.GetChannelCount());
    }
    else
    {
        throw std::runtime_error("Saving to this format is not supported: " + path.string());
    }
}

void ImageData::FreeImageData()
{
    if (imageDataType == ImageDataType::DDS)
    {
        //Delete file from memory
        ilDeleteImages(1, &ddsImageId);
    }
    else if (imageDataType == ImageDataType::Manual)
    {
        delete[] data;
    }
    else
    {
        stbi_image_free(data);
    }

    data = nullptr;
}

int ImageData::GetWidth() const
{
    return width;
}

int ImageData::GetHeight() const
{
    return height;
}

int ImageData::GetChannelCount() const
{
    return channelCount;
}

size_t ImageData::GetImageSize() const
{
    return width * height * channelCount;
}

uint8_t* ImageData::GetDataPtr()
{
    return data;
}

const uint8_t* ImageData::GetDataPtr() const
{
    return data;
}
