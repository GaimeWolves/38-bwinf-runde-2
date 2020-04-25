#pragma once

#include "point.h"

#include <vector>
#include <map>
#include <cstdlib>
#include <memory>

/**************************
 *    Globale typedefs    *
 **************************/

typedef struct path_t
{
	bool available;                // Pfad vorhanden?
	std::vector<point_t> shortest; // Kuerzester Pfad
	uint32_t length;               // Dessen Laenge

	bool extendable;               // Verlaengerbar?
	std::vector<point_t> extended; // Verlaengerbarer Pfad
	uint32_t extendedLength;       // Dessen Laenge
} path_t;

typedef struct robot_t
{
	uint32_t position;
	uint32_t charge;
} robot_t;

typedef std::map<uint32_t, std::map<uint32_t, std::shared_ptr<path_t>>> adjacency_t;

typedef struct map_t
{
	uint32_t size, area; // Groesse des Spielfelds
	std::map<uint32_t, uint32_t> batteries; // Liste der Batterien
	robot_t robot;
	adjacency_t adjacency; // Adjazenzliste
} map_t;

/***************************************
 *    Globale Methodendeklarationen    *
 ***************************************/

// Kleine Helfermethode, da C++ keine richtige sgn() Methode hat
template <typename T> int sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}

uint32_t encode(point_t p, uint32_t size);
point_t decode(uint32_t n, uint32_t size);

void parseGraph(map_t &map);
std::shared_ptr<path_t> findPath(point_t start, point_t end, map_t &map);
std::vector<point_t> BFS(point_t start, point_t goal, map_t &map, bool extendable);
