#include "generator.hpp"

#include <random>
#include <functional>
#include <map>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <algorithm>
#include <iostream>

#include "global.hpp"

/********************
 *    Konstanten    *
 ********************/

static const size_t MAX_ITERATIONS = 10000;
static const double MAX_DELTA = 1;
static const double D_DELTA = 0.1;
static const double L_DELTA = 0.1;

/****************************
 *    Statische Methoden    *
 ****************************/

static double calculateDI(double size, double density, double length);
static double calculateDeltaDensity(double size, double density, double length);
static double calculateDeltaLength(double size, double density, double length);

static std::vector<uint32_t> batteryBFS(uint32_t start, uint32_t end, map_t &map);

/*********************************************
 *    Statische Methoden Implementationen    *
 *********************************************/

static double calculateDI(double size, double density, double length)
{
	double densityFactor = 40 - 40 * std::exp(-5 * density);
	double lengthFactor = std::pow(1.0057, std::pow(length, 3));
	double sizeFactor = ((size * size) / 100) * density * length;

	return densityFactor * lengthFactor + sizeFactor;
}

static double calculateDeltaDensity(double size, double density, double length)
{
	double densityFactor = 200 * exp(-5 * density);
	double lengthFactor = std::pow(1.0057, std::pow(length, 3));
	double sizeFactor = ((size * size) / 100) * length;

	return 1 / (densityFactor * lengthFactor + sizeFactor);
}

static double calculateDeltaLength(double size, double density, double length)
{	
	double densityFactor = 40 - 40 * std::exp(-5 * density);
	double lengthFactor = 3 * std::log(1.0057) * std::pow(1.0057, std::pow(length, 3)) * std::pow(length, 2);
	double sizeFactor = ((size * size) / 100) * density;

	return 1 / (densityFactor * lengthFactor + sizeFactor);
}

static std::vector<uint32_t> batteryBFS(uint32_t start, uint32_t end, map_t &map)
{
	if (start == end && map.adjacency[start][end]->available)
		return { end };

	std::queue<uint32_t> queue;
	std::vector<bool> visited = std::vector<bool>(map.area, false);
	std::vector<uint32_t> parent = std::vector<uint32_t>(map.area, map.area);

	queue.push(start);

	while(!queue.empty())
	{
		uint32_t u = queue.front();
		queue.pop();

		if (visited[u])
			continue;

		visited[u] = true;

		for (auto v : map.adjacency[u])
		{
			if (!v.second->available)
				continue;

			if (visited[v.first])
				continue;

			// Pfad soll nur ueber Batterien gehen abgesehen vom Start und Ziel
			if (v.first != start && v.first != end && map.batteries.find(v.first) == map.batteries.end())
				continue;

			parent[v.first] = u;
			queue.push(v.first);
		}
	}

	std::vector<uint32_t> path;
	uint32_t current = end;
	while(current != start)
	{
		path.push_back(current);
		current = parent[current];
	}
	std::reverse(path.begin(), path.end());

	return path;
}

/************************************************
 *    Oeffentliche Methoden Implementationen    *
 ************************************************/

map_t generateConfig(difficulty_t difficulty)
{
	map_t map;
	map.size = difficulty.size;
	map.area = map.size * map.size;

	size_t batteryCount = (size_t)std::round((double)map.area * difficulty.density);
	size_t pathLength = (size_t)std::round((double)batteryCount * difficulty.length);

	if (batteryCount > map.area - 1)
		batteryCount = map.area - 1;

	std::vector<bool> free = std::vector<bool>(map.area, true);
	
	// Generiert ein zufaelliges Feld aus den noch freien Feldern
	std::function<uint32_t()> getRandom = [&free, &map](){
		uint32_t choice;

		do {
			choice = std::rand() % map.area;
		} while(!free[choice]);
		free[choice] = false;
	
		return choice;
	};
	
	// Zaehlt wie oft die jeweiligen Batterien besucht wurden
	// std::unordered_map behaelt die zufaellige Verteilung
	std::unordered_map<uint32_t, uint32_t> batteryCounter;

	uint32_t goalNode = std::rand() % map.area;
	uint32_t startNode = getRandom();

	while(batteryCount--)
	{
		uint32_t battery = getRandom();
		map.batteries[battery] = 0;
		batteryCounter[battery] = 0;
	}

	map.robot.position = startNode;
	parseGraph(map);

	if (map.batteries.find(goalNode) == map.batteries.end())
	{
		point_t goalPoint = decode(goalNode, map.size);
		for (auto battery : map.batteries)
		{
			auto path = findPath(decode(battery.first, map.size), goalPoint, map);
			map.adjacency[battery.first][goalNode] = path;
			map.adjacency[goalNode][battery.first] = path;
		}

		auto path = findPath(decode(startNode, map.size), goalPoint, map);
		map.adjacency[startNode][goalNode] = path;
		map.adjacency[goalNode][startNode] = path;
	}
	
	// Gibt die am wenigsten besuchte Batterie zurueck
	std::function<uint32_t()> getBattery = [&batteryCounter](){
		std::pair<uint32_t, uint32_t> min = *batteryCounter.begin();

		for (auto battery : batteryCounter)
			if (battery.second < min.second)
				min = battery;

		return min.first;
	};

	map.robot.position = goalNode;
	map.robot.charge = 0;

	while(map.robot.position != startNode)
	{
		auto startPath = batteryBFS(map.robot.position, startNode, map);
		std::vector<uint32_t> batteryPath;

		// Pfadlaenge reicht nicht mehr, also zum Start laufen
		bool walkToStart = startPath.size() >= pathLength;
		
		if (walkToStart)
			batteryPath = startPath;
		else
			batteryPath = batteryBFS(map.robot.position, getBattery(), map);

		for (auto battery : batteryPath)
		{
			auto path = map.adjacency[map.robot.position][battery];
			if (!path->available)
				break;

			uint32_t distance = path->length;
			// Zufaellig kuerzesten oder laengeren Pfad
			// zwischen den Punkten waehlen
			if (path->extendable && std::rand() % 10 > 7)
				// Zufaellige Verlaengerung zw. 0 und 10 waehlen
				distance = path->extendedLength + (std::rand() % 6) * 2;

			// Pfad entlanglaufen (Distanz wird zur Ladung addiert)
			if (battery == startNode)
				map.robot.charge += distance;
			else
			{
				uint32_t oldRobotCharge = map.robot.charge;
				map.robot.charge = map.batteries[battery];
				map.batteries[battery] = oldRobotCharge + distance;
			}
			
			map.robot.position = battery;

			if (pathLength > 0)
				pathLength--;
			batteryCounter[battery]++;
		}
	}

	return map;
}

void difficulty_t::calculate()
{
	di = calculateDI(size, density, length);
	deltaDensity = calculateDeltaDensity(size, density, length);
	deltaLength = calculateDeltaLength(size, density, length);
}

difficulty_t solveConstraints(constraint_t constraints, bool debug)
{
	std::uniform_real_distribution<double> distrib(constraints.minDI, constraints.maxDI);
	std::default_random_engine random;
	random.seed(std::rand());

	double targetDI = distrib(random);

	difficulty_t diff;

	diff.size = constraints.size;
	diff.density = (constraints.minDensity + constraints.maxDensity) / 2.0;
	diff.length = (constraints.minLength + constraints.maxLength) / 2.0;
	diff.calculate();

	size_t iterations = 0;

	while(iterations++ < MAX_ITERATIONS && std::abs(targetDI - diff.di) > MAX_DELTA)
	{
		difficulty_t newDiff = diff;

		newDiff.density -= diff.deltaDensity * D_DELTA * sgn(diff.di - targetDI);
		newDiff.length -= diff.deltaLength * L_DELTA * sgn(diff.di - targetDI);	
	
		// Beim Austreten aus den Grenzen zurueckgehen
		if (newDiff.density < constraints.minDensity || newDiff.density > constraints.maxDensity)
			newDiff.density += diff.deltaDensity * D_DELTA * sgn(diff.di - targetDI);

		if (newDiff.length < constraints.minLength || newDiff.length > constraints.maxLength)
			newDiff.length += diff.deltaLength * L_DELTA * sgn(diff.di - targetDI);	
		
		diff = newDiff;
		diff.calculate();

		if (debug)
			std::printf("Iteration: %-6lu  Target: %-3.2f  DI: %-3.2f  D: %-3.2f  L: %-3.2f \n",
				iterations,
				targetDI,
				diff.di,
				diff.density,
				diff.length
			);
	}

	return diff;
}

void writeToFile(map_t &map, std::string path)
{
	std::ofstream output;
	output.open(path);
	std::cout << "Spielsitation:\n"
			  << "--------------" << std::endl;
	
	output << map.size << '\n';
	std::cout << map.size << '\n';
	
	point_t robot = decode(map.robot.position, map.size);
	output << robot.x + 1 << ',' << robot.y + 1 << ',' << map.robot.charge << '\n';
	std::cout << robot.x + 1 << ',' << robot.y + 1 << ',' << map.robot.charge << '\n';

	output << map.batteries.size() << '\n';
	std::cout << map.batteries.size() << '\n';
	for (auto battery : map.batteries)
	{
		point_t pos = decode(battery.first, map.size);
		output << pos.x + 1 << ',' << pos.y + 1 << ',' << battery.second << '\n';
		std::cout << pos.x + 1 << ',' << pos.y + 1 << ',' << battery.second << '\n';
	}

	output.close();
	std::cout << std::endl;
}
