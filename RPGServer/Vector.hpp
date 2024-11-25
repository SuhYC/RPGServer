#pragma once

#include <math.h>

class Vector2
{
public:
	Vector2() : x(0.0f), y(0.0f) {}
	Vector2(float x_, float y_) : x(x_), y(y_) {}

	void Set(float x_, float y_)
	{
		x = x_;
		y = y_;
	}

	bool operator==(const Vector2& other_) const
	{
		return x == other_.x && y == other_.y;
	}
	bool operator==(const Vector2&& other_) const
	{
		return x == other_.x && y == other_.y;
	}
	bool operator!=(const Vector2& other_) const
	{
		return x != other_.x || y != other_.y;
	}
	bool operator!=(const Vector2&& other_) const
	{
		return x != other_.x || y != other_.y;
	}
	Vector2 operator+(const Vector2& other_) const
	{
		return Vector2(x + other_.x, y + other_.y);
	}
	Vector2 operator+(const Vector2&& other_) const
	{
		return Vector2(x + other_.x, y + other_.y);
	}
	Vector2 operator*(const int mul_) const
	{
		return Vector2(x * mul_, y * mul_);
	}
	Vector2 operator*(const float mul_) const
	{
		return Vector2(x * mul_, y * mul_);
	}

	float Distance(const Vector2& other_) const
	{
		Vector2 diff(other_.x - x, other_.y - y);

		return diff.Magnitude();
	}

	float Distance(const Vector2&& other_) const
	{
		Vector2 diff(other_.x - x, other_.y - y);

		return diff.Magnitude();
	}

	float SquaredDistance(const Vector2& other_) const
	{
		Vector2 diff(other_.x - x, other_.y - y);

		return diff.SquaredLength();
	}

	float SquaredDistance(const Vector2&& other_) const
	{
		Vector2 diff(other_.x - x, other_.y - y);

		return diff.SquaredLength();
	}

	float SquaredLength() const { return x * x + y * y; }
	float Magnitude() const { return sqrt(SquaredLength()); }

	float x;
	float y;
};