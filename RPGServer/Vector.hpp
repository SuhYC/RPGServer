#pragma once

#include <math.h>

class Vector2
{
public:
	Vector2(float x_, float y_) : x(x_), y(y_) {}
	Vector2(Vector2& other_) : x(other_.x), y(other_.y) {}
	Vector2(const Vector2& other_) : x(other_.x), y(other_.y) {}

	void Set(float x_, float y_)
	{
		x = x_;
		y = y_;
	}

	Vector2& operator=(Vector2& other_)
	{
		x = other_.x;
		y = other_.y;

		return *this;
	}

	Vector2& operator=(Vector2&& other_) noexcept
	{
		x = other_.x;
		y = other_.y;

		return *this;
	}

	float SquaredLength() const { return x * x + y * y; }
	float Magnitude() const { return sqrt(SquaredLength()); }

	float x;
	float y;
};