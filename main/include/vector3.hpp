#pragma once

#include <vector.hpp>

//========================================

template<Scalar T>
class Vector3
{
public:
	union
	{
		struct
		{
			T x, y, z;
		};
		
		T data[3];
	};
	
	Vector3();
	Vector3(T x, T y, T z);
	
	template<Scalar U>
	Vector3(const Vector3<U>& copy);
	
	T lengthSqr() const;
	float length() const;
	
	template<Scalar U> Vector3<T> operator+(const Vector3<U>& rhs) const;
	template<Scalar U> Vector3<T> operator-(const Vector3<U>& rhs) const;
	template<Scalar U> Vector3<T> operator*(const Vector3<U>& rhs) const;
	template<Scalar U> Vector3<T> operator/(const Vector3<U>& rhs) const;
	
	template<Scalar U> Vector3<T>& operator+=(const Vector3<U>& rhs);
	template<Scalar U> Vector3<T>& operator-=(const Vector3<U>& rhs);
	template<Scalar U> Vector3<T>& operator*=(const Vector3<U>& rhs);
	template<Scalar U> Vector3<T>& operator/=(const Vector3<U>& rhs);
	
	template<Scalar U> Vector3<T> operator+(U scalar) const;
	template<Scalar U> Vector3<T> operator-(U scalar) const;
	template<Scalar U> Vector3<T> operator*(U scalar) const;
	template<Scalar U> Vector3<T> operator/(U scalar) const;
	
	template<Scalar U> Vector3<T>& operator+=(U scalar);
	template<Scalar U> Vector3<T>& operator-=(U scalar);
	template<Scalar U> Vector3<T>& operator*=(U scalar);
	template<Scalar U> Vector3<T>& operator/=(U scalar);
	
};

//========================================

template<Scalar T, Scalar U>
Vector3<T> operator+(U scalar, const Vector3<T> vec);

template<Scalar T, Scalar U>
Vector3<T> operator*(U scalar, const Vector3<T> vec);

//========================================

using Vector3d = Vector3<double>;
using Vector3f = Vector3<float>;
using Vector3i = Vector3<int>;
using Vector3u = Vector3<unsigned>;

//======================================== Constructors

template<Scalar T>
Vector3<T>::Vector3():
	x(static_cast<T>(0)),
	y(static_cast<T>(0)),
	z(static_cast<T>(0))
{}

template<Scalar T>
Vector3<T>::Vector3(T x_, T y_, T z_):
	x(x_),
	y(y_),
	z(z_)
{}

template<Scalar T>
template<Scalar U>
Vector3<T>::Vector3(const Vector3<U>& copy):
	x(static_cast<T>(copy.x)),
	y(static_cast<T>(copy.y)),
	z(static_cast<T>(copy.z))
{}

//======================================== Linear operations

template<Scalar T>
T Vector3<T>::lengthSqr() const
{
	return x*x + y*y + z*z;
}

template<Scalar T>
float Vector3<T>::length() const
{
	return sqrt(x*x + y*y + z+z);
}

//======================================== Arithmetic operations with vectors

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator+(const Vector3<U>& rhs) const
{
	return Vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator-(const Vector3<U>& rhs) const
{
	return Vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator*(const Vector3<U>& rhs) const
{
	return Vector3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator/(const Vector3<U>& rhs) const
{
	return Vector3<T>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator+=(const Vector3<U>& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator-=(const Vector3<U>& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator*=(const Vector3<U>& rhs)
{
	x *= rhs.x;
	y *= rhs.y;
	z *= rhs.z;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator/=(const Vector3<U>& rhs)
{
	x /= rhs.x;
	y /= rhs.y;
	z /= rhs.z;
	
	return *this;
}

//======================================== Arithmetic operations with rhs scalars

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator+(U scalar) const
{
	return Vector3<T>(x + scalar, y + scalar, z + scalar);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator-(U scalar) const
{
	return Vector3<T>(x - scalar, y - scalar, z - scalar);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator*(U scalar) const
{
	return Vector3<T>(x * scalar, y * scalar, z * scalar);
}

template<Scalar T>
template<Scalar U>
Vector3<T> Vector3<T>::operator/(U scalar) const
{
	return Vector3<T>(x / scalar, y / scalar, z / scalar);
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator+=(U scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator-=(U scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator*=(U scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	
	return *this;
}

template<Scalar T>
template<Scalar U>
Vector3<T>& Vector3<T>::operator/=(U scalar)
{
	x /= scalar;
	y /= scalar;
	z /= scalar;
	
	return *this;
}

//======================================== Arithmetic operations with lhs scalars

template<Scalar T, Scalar U>
Vector3<T> operator+(U scalar, const Vector3<T> vec)
{
	return Vector3<T>(scalar + vec.x, scalar + vec.y, scalar + vec.z);
}

template<Scalar T, Scalar U>
Vector3<T> operator*(U scalar, const Vector3<T> vec)
{
	return Vector3<T>(scalar * vec.x, scalar * vec.y, scalar * vec.z);
}

//========================================