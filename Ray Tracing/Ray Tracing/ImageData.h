#pragma once

#include "Vector3.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

template<size_t width, size_t height>
class ImageData
{
public:
	void Write(Vector3 item, size_t x, size_t y)
	{
		std::lock_guard<std::mutex> lockguard(mutex);
		data[y][x] = item;
		currentPixelsComplete++;
		if (currentPixelsComplete % 10000 == 0)
		{
			std::cout << currentPixelsComplete << "/" << totalPixelCount << "\n";
		}
	}

	Vector3 Read(size_t x, size_t y)
	{
		std::lock_guard<std::mutex> lockguard(mutex);
		return data[y][x];
	}

	void WriteImageDataToFile(std::filesystem::path filepath, size_t ns)
	{
		std::ofstream file(filepath);

		if (file.is_open())
		{
			std::lock_guard<std::mutex> lockguard(mutex);

			file << "P3\n" << width << " " << height << "\n255\n";

			for (const auto& row : data)
			{
				for (Vector3 colour : row)
				{
					//Gamma correction
					colour /= static_cast<float>(ns);

					colour = Vector3(std::sqrt(colour.x), std::sqrt(colour.y), std::sqrt(colour.z));

					int ir = static_cast<int>(255.99 * colour.x);
					int ig = static_cast<int>(255.99 * colour.y);
					int ib = static_cast<int>(255.99 * colour.z);

					file << ir << " " << ig << " " << ib << "\n";
				}
			}
		}
	}

private:
	std::array<std::array<Vector3, width>, height> data;
	std::mutex mutex;
	size_t totalPixelCount = width * height;
	size_t currentPixelsComplete = 0;
};