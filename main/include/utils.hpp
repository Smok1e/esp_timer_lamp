#pragma once

#include <ctime>
#include <vector3.hpp>

//========================================

unsigned SecondsSinceMidnight(const tm& info);
unsigned SecondsSinceMidnight();
unsigned ParseSecondsSinceMidnight(const char* str);

//========================================