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

static const size_t MAX_ITERATIONS = 1000;
static const double MAX_DELTA = 0.1;
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

// Implementation der DI(x,y) Funktion
static double calculateDI(double size, double density, double length) {
	double densityFactor = 40 - 40 * std::exp(-5 * density);
	double lengthFactor = std::pow(1.0057, std::pow(length, 3));
	double sizeFactor = ((size * size) / 100) * density * length;

	return densityFactor * lengthFactor + sizeFactor;
}

// Implemetation des Kehrwerts der dD(x,y) Funktion
static double calculateDeltaDensity(double size, double density, double length) {
	double densityFactor = 200 * exp(-5 * density);
	double lengthFactor = (std::pow(3, length) - 1) / (std::pow(length, 3) + 2);
	double sizeFactor = ((size * size) / 100) * length;

	return 1 / (densityFactor * lengthFactor + sizeFactor);
}

// Implemetation des Kehrwerts der dL(x,y) Funktion
static double calculateDeltaLength(double size, double density, double length) {
	double densityFactor = 40 - 40 * std::exp(-5 * density);
	double lengthFactor = ((std::pow(3, length) * std::log(3))
						/ (std::pow(length, 3) + 2))
						- ((3 * (std::pow(3, length) - 1) * std::pow(length, 2))
						/ (std::pow(std::pow(length, 3) + 2, 2)));
	double sizeFactor = ((size * size) / 100) * density;

	return 1 / (densityFactor * lengthFactor + sizeFactor);
}

// Sucht einen Pfad durch die Spielfeldabstraktion von einer Batterie zu einer anderen
static std::vector<uint32_t> batteryBFS(uint32_t start, uint32_t end, map_t &map) {
	// Eigenpfad gefragt und verfuegbar?
	if (start == end && map.adjacency[start][end]->available)
		return { end };

	std::queue<uint32_t> queue;
	std::vector<bool> visited = std::vector<bool>(map.area, false);
	std::vector<uint32_t> parent = std::vector<uint32_t>(map.area, map.area);

	queue.push(start);

	while(!queue.empty()) {
		uint32_t u = queue.front();
		queue.pop();

		if (u == end)
			break;

		if (visited[u])
			continue;

		visited[u] = true;

		// Nachbarn durchlaufen
		for (auto v : map.adjacency[u]) {
			if (!v.second->available)
				continue;

			if (visited[v.first])
				continue;

			// Pfad soll nur ueber Batterien gehen abgesehen vom Start und Ziel
			// Die Adjazenzliste beinhaltet naemlich auch Pfade zum Roboter
			if (v.first != start && v.first != end
				&& map.batteries.find(v.first) == map.batteries.end())
				continue;

			visited[v.first] = true;
			parent[v.first] = u;
			queue.push(v.first);
		}
	}

	// Batteriepfad konstruieren
	std::vector<uint32_t> path;
	uint32_t current = end;
	while(current != start)
	{
		if (current == map.area) { // Kein Pfad verfuegbar
			return {};
		}

		path.push_back(current);
		current = parent[current];
	}
	std::reverse(path.begin(), path.end());

	return path;
}

/************************************************
 *    Oeffentliche Methoden Implementationen    *
 ************************************************/

// Generiert eine Spielsituation aus den Spielfeldeigenschaften
map_t generateConfig(difficulty_t difficulty) {
	map_t map;
	map.size = difficulty.size;
	map.area = map.size * map.size;

	// Richtige Batterieanzahl und Loesungslaenge berechnen
	size_t batteryCount = (size_t)std::round((double)map.area * difficulty.density);
	size_t pathLength = (size_t)std::round((double)batteryCount * difficulty.length);

	// Batterieanzahl auf maximale Anzahl beschraenken
	if (batteryCount > map.area - 1)
		batteryCount = map.area - 1;

	std::vector<bool> free = std::vector<bool>(map.area, true);
	
	// Generiert ein zufaelliges Feld aus den noch freien Feldern
	std::function<uint32_t()> getRandom = [&free, &map](){
		// Zufaellig waehlen solange
		// gewaehltes belegt ist
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

	// Zielfeld aussuchen (Kann auf einer Batterie sein)
	uint32_t goalNode = std::rand() % map.area;

	// Startfeld aussuchen (Muss frei sein)
	uint32_t startNode = getRandom();

	// Batterien verteilen
	while(batteryCount--) {
		uint32_t battery = getRandom();
		map.batteries[battery] = 0;
		batteryCounter[battery] = 0;
	}

	// Adjazenzliste generieren
	map.robot.position = startNode;
	parseGraph(map);

	// Falls Zielknoten nicht auf einer Batterie liegt
	// muessen die Pfade zw. dieser und den Batterien generiert werden
	if (map.batteries.find(goalNode) == map.batteries.end()) {
		point_t goalPoint = decode(goalNode, map.size);
		for (auto battery : map.batteries) {
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

	// Roboter auf das Zielfeld setzten
	map.robot.position = goalNode;
	map.robot.charge = 0;

	// Solange der Roboter nicht am Startfeld ist
	// (Ladung ueberprufen, falls das Startfeld auch
	// das Zielfeld ist)
	while(map.robot.position != startNode || map.robot.charge == 0) {
		auto startPath = batteryBFS(map.robot.position, startNode, map);
		std::vector<uint32_t> batteryPath;

		// Pfadlaenge reicht nicht mehr, also zum Start laufen
		bool walkToStart = startPath.size() >= pathLength;
		
		if (walkToStart)
			batteryPath = startPath;
		else // Ansonsten zu einer zufaelligen Batterie gehen
			batteryPath = batteryBFS(map.robot.position, getBattery(), map);

		while (batteryPath.size() == 0) { // Wenn kein Pfad gefunden wurde
			auto iter = map.batteries.begin();
			std::advance(iter, rand() % map.batteries.size());
			batteryPath = batteryBFS(map.robot.position, (*iter).first, map);
		}

		// Fuer jede Batterie im Pfad
		for (auto battery : batteryPath) {
			auto path = map.adjacency[map.robot.position][battery];
			if (!path->available)
				break;

			// Zufaellig kuerzesten oder laengeren Pfad
			// zwischen den Punkten waehlen
			uint32_t distance = path->length;
			if (path->extendable && std::rand() % 10 > 7)
				// Zufaellige Verlaengerung zw. 0 und 10 waehlen
				distance = path->extendedLength + (std::rand() % 6) * 2;

			// Pfad entlanglaufen (Distanz wird zur Ladung addiert)
			if (battery == startNode)
				map.robot.charge += distance;
			else {
				// Batterietausch
				uint32_t oldRobotCharge = map.robot.charge;
				map.robot.charge = map.batteries[battery];
				map.batteries[battery] = oldRobotCharge + distance;
			}
			
			// Roboter auf die Batterie setzen
			map.robot.position = battery;

			// Loesungslange verringern
			if (pathLength > 0)
				pathLength--;

			// Batteriezaehler erhoerhen
			batteryCounter[battery]++;
		}
	}

	// Spielsituation ist fertig generiert
	return map;
}

// Berechnet alle Funktionswerte
void difficulty_t::calculate() {
	di = calculateDI(size, density, length);
	deltaDensity = calculateDeltaDensity(size, density, length);
	deltaLength = calculateDeltaLength(size, density, length);
}

// Berechnet die Spielfeldeigenschaften aus den Beschraenkungen
difficulty_t solveConstraints(constraint_t constraints, bool debug) {
	std::uniform_real_distribution<double> distrib(constraints.minDI, constraints.maxDI);
	std::default_random_engine random;
	random.seed(std::rand());

	// Zielschwierigkeit waehlen
	double targetDI = distrib(random);

	// Mittelpunkt im erlaubten Bereich setzen
	difficulty_t diff;
	diff.size = constraints.size;
	diff.density = (constraints.minDensity + constraints.maxDensity) / 2.0;
	diff.length = (constraints.minLength + constraints.maxLength) / 2.0;
	diff.calculate();

	// Bis der Abstand zur Zielschwierigkeit klein genug ist
	size_t iterations = 0;
	while(iterations++ < MAX_ITERATIONS && std::abs(targetDI - diff.di) > MAX_DELTA) {
		// Gradient-Descent anwenden
		diff.density -= diff.deltaDensity * D_DELTA * sgn(diff.di - targetDI);
		diff.length -= diff.deltaLength * L_DELTA * sgn(diff.di - targetDI);	
	
		// Beim Austreten aus den Grenzen zurueckgehen
		if (diff.density < constraints.minDensity || diff.density > constraints.maxDensity)
			diff.density += diff.deltaDensity * D_DELTA * sgn(diff.di - targetDI);

		if (diff.length < constraints.minLength || diff.length > constraints.maxLength)
			diff.length += diff.deltaLength * L_DELTA * sgn(diff.di - targetDI);	
		
		// Neue Funktionswerte berechnen
		diff.calculate();

		if (debug)
			std::printf("Iter. %-6lu  Target: %-3.2f  DI: %-3.2f  D: %-3.2f  L: %-3.2f \n",
				iterations,
				targetDI,
				diff.di,
				diff.density,
				diff.length
			);
	}

	// Spielfeldeigenschaften zurueckgeben
	return diff;
}

// In die Konsole und in die Datei schreiben
void writeToFile(map_t &map, std::string path) {
	std::ofstream output;
	output.open(path);
	std::cout << "Spielsitation:\n"
			  << "--------------" << std::endl;
	
	output << map.size << '\n';
	std::cout << map.size << '\n';
	
	point_t robot = decode(map.robot.position, map.size);
	output << robot.x << ',' << robot.y << ',' << map.robot.charge << '\n';
	std::cout << robot.x << ',' << robot.y << ',' << map.robot.charge << '\n';

	output << map.batteries.size() << '\n';
	std::cout << map.batteries.size() << '\n';
	for (auto battery : map.batteries) {
		point_t pos = decode(battery.first, map.size);
		output << pos.x << ',' << pos.y << ',' << battery.second << '\n';
		std::cout << pos.x << ',' << pos.y << ',' << battery.second << '\n';
	}

	output.close();
	std::cout << std::endl;
}
