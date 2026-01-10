#include <utils.hpp>

#include <esp_log.h>

//========================================

unsigned SecondsSinceMidnight(const tm& info)
{
	return info.tm_sec + 60 * (info.tm_min + 60 * info.tm_hour);
}

unsigned SecondsSinceMidnight()
{
	time_t now = time(nullptr);
	
	tm local_time = {};
	localtime_r(&now, &local_time);
	
	return SecondsSinceMidnight(local_time);
}

unsigned ParseSecondsSinceMidnight(const char* str)
{
	tm info = {};
	if (strptime(str, "%H:%M:%S", &info) == str)
	{
		ESP_LOGE("parser", "%s does not match the HH:MM:SS format; aborting", str);
		abort();
	}
	
	return SecondsSinceMidnight(info);
}

//========================================

Vector3f RotateAroundX(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x,
		point.y * cos_phi - point.z * sin_phi,
		point.y * sin_phi + point.z * cos_phi
	);
}

Vector3f RotateAroundY(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x * cos_phi + point.z * sin_phi,
		point.y,
		-point.x * sin_phi + point.z * cos_phi
	);
}

Vector3f RotateAroundZ(const Vector3f& point, float sin_phi, float cos_phi)
{
	return Vector3f(
		point.x * cos_phi - point.y * sin_phi,
		point.x * sin_phi + point.y * cos_phi,
		point.z
	);
}

//========================================