#pragma once

#include <cstdint>

typedef struct point_t
{
	int x, y;

	bool operator==(const point_t &rhs) const;
	bool operator!=(const point_t &rhs) const;
	bool operator >(const point_t &rhs) const;
	bool operator <(const point_t &rhs) const;

	point_t operator+(const point_t &rhs);
	point_t operator-(const point_t &rhs);

	point_t operator*(const int &rhs);
	
	point_t& operator+=(const point_t &rhs);
	point_t& operator-=(const point_t &rhs);

	point_t& operator*=(const int &rhs);
	
	point_t& normalize();
	point_t& rotate90();
} point_t;
