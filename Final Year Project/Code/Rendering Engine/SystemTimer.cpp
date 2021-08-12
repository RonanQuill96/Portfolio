#include "SystemTimer.h"

void SystemTimer::Start()
{
	previousTime = SystemClock::now();
}

void SystemTimer::Update()
{
	auto currentTime = SystemClock::now();

	std::chrono::duration<double> frameTimeDuration = currentTime - previousTime;
	frameTime = frameTimeDuration.count();

	previousTime = currentTime;
}

double SystemTimer::GetFrameTime() const
{
	return frameTime;
}

float SystemTimer::GetFrameTimeF() const
{
	return static_cast<float>(frameTime);
}
