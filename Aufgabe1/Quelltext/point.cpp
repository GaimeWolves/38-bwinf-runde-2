#include "point.h"

#include <cmath>

bool point_t::operator==(const point_t &rhs) const
{
	return x == rhs.x && y == rhs.y;
}

bool point_t::operator!=(const point_t &rhs) const
{
	return x != rhs.x || y != rhs.y;
}

bool point_t::operator >(const point_t &rhs) const
{
	return x == rhs.x ? y > rhs.y : x > rhs.x;
}

bool point_t::operator <(const point_t &rhs) const
{
	return x == rhs.x ? y < rhs.y : x < rhs.x;
}

point_t point_t::operator+(const point_t &rhs)
{
	return { x + rhs.x, y + rhs.y };
}

point_t point_t::operator*(const int &rhs)
{
	return { x * rhs, y * rhs };
}

point_t point_t::operator-(const point_t &rhs)
{
	return { x - rhs.x, y - rhs.y };
}

point_t& point_t::operator+=(const point_t &rhs)
{
	x += rhs.x;
	y += rhs.y;
	
	return *this;
}

point_t& point_t::operator-=(const point_t &rhs)
{
	x -= rhs.x;
	y -= rhs.y;

	return *this;
}

point_t& point_t::operator*=(const int &rhs)
{
	x *= rhs;
	y *= rhs;

	return *this;
}

point_t& point_t::normalize()
{
	x = x ? x / std::abs(x) : 0;
	y = y ? y / std::abs(y) : 0;

	return *this;
}

point_t& point_t::rotate90()
{
	int tmp = x;
	x = -y;
	y = tmp;

	return *this;
}
