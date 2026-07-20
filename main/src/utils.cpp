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