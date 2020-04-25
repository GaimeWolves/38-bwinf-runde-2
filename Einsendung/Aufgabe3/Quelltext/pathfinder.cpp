#include "pathfinder.hpp"

#include <map>
#include <vector>
#include <regex>
#include <fstream>
#include <iostream>
#include <cmath>
#include <set>
#include <limits>
#include <queue>

/*
 * Implementation der Structs
 */

// Point Struct Implementation
Point::Point(int x, int y)
	: x(x), y(y)
{}

Point::Point() {}

bool Point::operator==(const Point &b) const
{
	return x == b.x && y == b.y;
}

// Predecessor Struct Implementation
Predecessor::Predecessor(uint32_t predecessor, float distance)
	: predecessor(predecessor), distance(distance)
{}

Predecessor::Predecessor()
	: predecessor(-1), distance(std::numeric_limits<float>::infinity())
{}

// Worker Struct Implementation
Worker::Worker(uint32_t start)
	: position(start), distance(0), turns(0), path(std::vector<uint32_t>())
{}

Worker::Worker(uint32_t pos, float dist, int turns, std::vector<uint32_t> path)
	: position(pos), distance(dist), turns(turns), path(path)
{}

Worker::Worker()
	: position(0), distance(0), turns(0), path(std::vector<uint32_t>())
{}

// Sortiert zuerst nach Kurvenanzahl, dann nach Distanz
bool Worker::operator>(const Worker &rhs) const
{
	if (turns == rhs.turns)
			return distance > rhs.distance;
	return turns > rhs.turns;
}


/*
 * Implementation der globalen Variablen
 */

std::vector<Point> lookup;
std::map<uint32_t, std::set<uint32_t>> adjList;

uint32_t end;
uint32_t start;

float maxPercentage;


/*
 * Definition und Implementation der statischen Methoden
 */

static inline float getDist(uint32_t a, uint32_t b);
static inline int isCurve(uint32_t c, uint32_t a, uint32_t b);

// Berechnet die Distanz zwischen zwei Punkten
static inline float getDist(uint32_t a, uint32_t b)
{
	//Punkte aus der Lookup-Table holen
	auto pA = lookup[a];
	auto pB = lookup[b];

	if (adjList[a].find(b) == adjList[a].end()) // Keine Kante
		return std::numeric_limits<float>::infinity();

	return std::sqrt(std::pow(pA.x - pB.x, 2) + std::pow(pA.y - pB.y, 2));
}

// Testet ob drei Punkte im Pfad kollinear sind
// c: Derzeitige Position/Punkt
// a, b: nachfolgender bzw. voriger Punkt
static inline int isCurve(uint32_t c, uint32_t a, uint32_t b)
{
	if (a == b)
		return 1;

	//Punkte aus der Lookup-Table holen
	auto pA = lookup[a];
	auto pB = lookup[b];
	auto pC = lookup[c];

	return (pC.x - pA.x) * (pB.y - pA.y) - (pC.y - pA.y) * (pB.x - pA.x) != 0; 
}


/*
 * Implementation der öffentlichen Methoden
 */

// Zeigt die Adjazenzliste und die Start- und Endknoten auf der Konsole an
void printNodes()
{
	for (auto adjVector : adjList)
	{
		auto node = lookup[adjVector.first];
		std::cout << "(" << node.x << "," << node.y << ") : { ";

		for (auto neighbour : adjVector.second)
		{
			node = lookup[neighbour];
			std::cout << "(" << node.x << "," << node.y << ") ";
		}

		std::cout << "}" << std::endl;
	}

	auto startNode = lookup[start];
	std::cout << "Start: (" << startNode.x << "," << startNode.y << ")" << std::endl;

	auto endNode = lookup[end];
	std::cout << "Ziel: (" << endNode.x << "," << endNode.y << ")" << std::endl;
}

// Berechnet für alle Knoten den kuerzesten Pfad zum Startknoten
std::vector<Predecessor> findShortestPath()
{
	// Speichert die Gesamtdistanz von einem Knoten
	typedef std::pair<float, uint32_t> Distance;

	// Priority queue zur Dijkstra Implementation, sortiert nach Distanz eines Knoten
	std::priority_queue<Distance, std::vector<Distance>, std::greater<Distance>> queue;
	
	// Speichert fuer alle Knoten, ob diese schon besucht wurden
	std::vector<bool> visited(lookup.size(), false);

	// Speichert fuer alle Knoten den Vorgänger und die Gesamtdistanz
	std::vector<Predecessor> predecessor(lookup.size());

	// Startknoten und Queue initialisieren
	queue.push(std::make_pair(0, start));
	predecessor[start].distance = 0;

	// Bis die Queue leer ist
	while(!queue.empty())
	{
		// Zu bearbeitender Knoten wird aus der Queue genommen
		uint32_t u = queue.top().second;
		queue.pop();

		// Schon besuchte Knoten ueberspringen
		if (visited[u])
			continue;

		// Knoten als besucht markieren
		visited[u] = true;

		// Alle benachbarten Knoten durchlaufen
		for (auto v : adjList[u])
		{
			// Schon besuchte ueberspringen
			if (visited[v])
				continue;

			// Neue Distanz berechnen
			float distance = predecessor[u].distance + getDist(u, v);

			// Mit alter Distanz vergleichen
			if (distance < predecessor[v].distance)
			{
				// Vorgaenger und Distanz updaten
				predecessor[v].predecessor = u;
				predecessor[v].distance = distance;

				// Nachbarknoten in die Queue pushen, da es in C++ keinen leichten
				// Weg gibt Elemente einer std::priority_queue zu aendern
				queue.push(std::make_pair(distance, v));
			}
		}
	}

	// Vorgaengerliste zurückgeben
	return predecessor;
}

// Berechnet die angegebene Anzahl an Pfaden mit den wenigsten Kurven
std::vector<Worker> findFewestTurnPaths(
	std::vector<Predecessor> &predecessor,
	size_t pathCap
)
{
	// Priority Queue an Arbeitern (werden nach Kurvenanzahl und Gesamtdistanz sortiert)
	std::priority_queue<Worker, std::vector<Worker>, std::greater<Worker>> queue;

		// Anfangsarbeiter in die Queue pushen
	queue.push(Worker(end));
	
	// Speichert die gefundenen Pfade ab
	std::vector<Worker> paths;

	// Maximaldistanz berechnen (Kuerzester Pfad * Verlaengerungsfaktor)
	float maxDistance = predecessor[end].distance * maxPercentage;

	// Solange die Queue nicht leer ist
	while (!queue.empty())
	{
		// Den nächsten Arbeiter aus der Queue nehmen
		Worker worker = queue.top();
		queue.pop();

		// Arbeiter mit zu langem Weg ueberspringen
		if (worker.distance + predecessor[worker.position].distance > maxDistance)
			continue;

		// Derzeitige Position zum Pfad hinzufuegen
		worker.path.push_back(worker.position);
		
		// Ist der Arbeiter am "Ziel"?
		if (worker.position == start)
		{
			// Arbeiter/Pfad abspeichern
			paths.push_back(worker);
			
			// Wenn die gewuenschte Anzahl an Pfaden erreicht wurde abbrechen
			if (paths.size() >= pathCap)
				break;

			// Zum naechsten Arbeiter gehen
			continue;
		}

		// Alle Nachbarknoten durchgehen
		for (uint32_t v : adjList[worker.position])
		{
			// Keine schon besuchten Knoten besuchen
			if (std::find(worker.path.begin(), worker.path.end(), v) != worker.path.end())
				continue;

			// Neue Distanz berechnen
			float dist = worker.distance + getDist(worker.position, v);

			// Neue Kurvenanzahl berechnen
			int turns = worker.turns;
			if (worker.path.size() > 1) // Sind 3 Punkte verfuegbar?
				turns += isCurve(worker.position, *(worker.path.end() - 2), v);

			// Neuen Arbeiter in die Queue pushen
			queue.push(Worker(v, dist, turns, worker.path));
		}
	}

	//Pfade zurueckgeben
	return paths;
}

// Liest die Datei aus
void parseFile(char* filePath)
{
	std::ifstream file(filePath);
	std::regex r("\\d+");

	if (file.is_open())
	{
		std::string line;

		std::getline(file, line);
		int count = std::stoi(line);

		// Naechste Zeile mithilfe des Regex einlesen
		std::getline(file, line);
		std::sregex_iterator it(line.begin(), line.end(), r);
		int x = std::stoi((*it)[0]);      // X-Wert aus der Datei lesen
		int y = std::stoi((*(++it))[0]);  // Y-Wert aus der Datei lesen
		start = lookup.size();            // Eine Zahl dem Startknoten vergeben
		lookup.push_back({x, y});         // In die Lookup Table einspeichern

		std::getline(file, line);
		it = std::sregex_iterator(line.begin(), line.end(), r);
		x = std::stoi((*it)[0]);
		y = std::stoi((*(++it))[0]);
		end = lookup.size();
		lookup.push_back({x, y});

		// Alle restlichen Kanten einlesen
		while(count-- > 0)
		{
			std::getline(file, line);
			it = std::sregex_iterator(line.begin(), line.end(), r);

			int x1 = std::stoi((*it)[0]);      // X-Wert des ersten Knoten
			int y1 = std::stoi((*(++it))[0]);  // Y-Wert des ersten Knoten
			int x2 = std::stoi((*(++it))[0]);  // X-Wert des zweiten Knoten
			int y2 = std::stoi((*(++it))[0]);  // Y-Wert des zweiten Knoten

			int nodeA;
			int nodeB;

			Point p = {x1, y1};

			// Ist der Knoten schon in der Lookup Table vorhanden?
			auto it = std::find(lookup.begin(), lookup.end(), p);
			if (it == lookup.end())
			{
				// Falls nicht, dann Punkt und Index speichern
				nodeA = lookup.size();
				lookup.push_back(p);
			}
			else
				// Andernfalls nur den Index des Knoten auslesen
				nodeA = std::distance(lookup.begin(), it);

			p = {x2, y2};
			it = std::find(lookup.begin(), lookup.end(), p);
			if (it == lookup.end())
			{
				nodeB = lookup.size();
				lookup.push_back(p);
			}
			else
				nodeB = std::distance(lookup.begin(), it);

			// Kante in der Adjazenzliste speichern
			adjList[nodeA].insert(nodeB);
			adjList[nodeB].insert(nodeA);
		}
	}
	else
	{
		std::cerr << "Falsche oder korrupte Datei" << std::endl;
		exit(-1);
	}
	
	file.close();
}