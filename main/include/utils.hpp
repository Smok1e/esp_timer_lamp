#pragma once

#include <ctime>
#include <vector3.hpp>

//========================================

unsigned SecondsSinceMidnight(const tm& info);
unsigned SecondsSinceMidnight();
unsigned ParseSecondsSinceMidnight(const char* str);

Vector3f RotateAroundX(const Vector3f& point, float sin_phi, float cos_phi);
Vector3f RotateAroundY(const Vector3f& point, float sin_phi, float cos_phi);
Vector3f RotateAroundZ(const Vector3f& point, float sin_phi, float cos_phi);

//========================================