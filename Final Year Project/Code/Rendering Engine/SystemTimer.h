#pragma once

#include <chrono>

class SystemTimer
{
public:
	void Start();

	void Update();

	double GetFrameTime() const;
	float GetFrameTimeF() const;

private:
	using SystemClock = std::chrono::high_resolution_clock;

	std::chrono::time_point<SystemClock> previousTime;
	double frameTime = 0.0;
};

