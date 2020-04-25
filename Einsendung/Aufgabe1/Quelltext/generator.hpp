#pragma once

#include <vector>

#include "global.hpp"
#include "point.hpp"

// Stellt die Begrenzungen für den Gradient-Descent dar
typedef struct constraint_t {
	uint32_t size = 10;
	double minDI = 0, maxDI = 100;
	double minDensity = 0, maxDensity = 1;
	double minLength = 0, maxLength = 5;
} constraint_t;

// Stellt die Spielfeldeigenschaften dar
typedef struct difficulty_t {
	uint32_t size;
	double density;
	double length;

	double di;
	double deltaDensity;
	double deltaLength;

	void calculate();
} difficulty_t;

difficulty_t solveConstraints(constraint_t constraints, bool debug);
map_t generateConfig(difficulty_t difficulty);
void writeToFile(map_t &map, std::string path);
