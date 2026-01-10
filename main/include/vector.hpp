#pragma once

#include <concepts>
#include <cmath>

//========================================

template<typename T>
concept Scalar = std::integral<T> || std::floating_point<T>;

//========================================