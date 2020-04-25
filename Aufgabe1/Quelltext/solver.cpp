#include "solver.hpp"

#include <map>
#include <deque>
#include <queue>
#include <fstream>
#include <iostream>
#include <functional>
#include <regex>

#include "point.h"
#include "global.hpp"

/********************
 *     Typedefs     *
 ********************/

typedef struct node_t
{
	node_t *prev;
	node_t *next;

	uint32_t point;

	~node_t();
	
	int distance(uint32_t p);
	node_t* find(uint32_t p);
} node_t;

typedef struct distance_pair_t
{
	uint32_t position;
	uint32_t distance;
} distance_pair_t;

typedef struct worker_t
{
	robot_t robot;
	std::map<uint32_t, uint32_t> batteries;
	std::vector<distance_pair_t> path;
	int score;
	size_t chargeSum;
	int prevDirection = 0;

	void calculateScore();
	bool operator<(const worker_t &rhs) const;
	bool operator==(const worker_t &rhs) const;
} worker_t;

/*******************************
 *     Statische Variablen     *
 *******************************/

static size_t totalCharge;      // Gesamtsumme der Batterieladungen
static node_t *hamiltonCycle; // Hamiltonkreis

static map_t map; // Spielfeld

/*******************************************
 *     Statische Methodendeklarationen     *
 *******************************************/

static node_t* oddHamilton(node_t *start);
static node_t* evenHamilton(node_t *start);
static void findHamilton();

static std::vector<uint32_t> batteryBFS(uint32_t start, uint32_t end, worker_t &state);
static bool checkClique(worker_t &state);

static worker_t solveConfig(bool debug);
static std::vector<point_t> constructPath(worker_t &solution);

static void readFile(const char *path);

/**********************************************
 *     Statische Methodenimplementationen     *
 **********************************************/

// Sucht den Punkt im Ring und gibt
// dessen Distanz zurueck
int node_t::distance(uint32_t p)
{
	int best;

	node_t* current = this;
	int dist = 0;
	do
	{
		if (!current->next) break;
		dist++;
		current = current->next;
	} while(current->point != p && current != this);

	best = dist;

	current = this;
	dist = 0;
	do
	{
		if (!current->prev) break;
		dist--;
		current = current->prev;
	} while(current->point != p && current != this);

	if (std::abs(dist) < best)
		best = dist;

	return best;
}

// Schliesst die Luecke im Ring wenn noetig
node_t::~node_t()
{
	if (prev) prev->next = next;
	if (next) next->prev = prev;
}

// Findet die Node im Ring mit dem angegebenen Wert
node_t* node_t::find(uint32_t p)
{
	if (point == p)
		return this;

	node_t *start = this;
	node_t *current = this;

	do
	{
		if (!current->next) break;
		current = current->next;
	} while(current != start && current->point != p);

	return current;
}

bool worker_t::operator<(const worker_t &rhs) const
{
	return score < rhs.score;
}

bool worker_t::operator==(const worker_t &rhs) const
{
	return score == rhs.score;
}

void worker_t::calculateScore()
{
	// Entladene Ladung berechnen
	score = (totalCharge - this->chargeSum) * (map.area / batteries.size());

	// Hamiltonkreis zur derzeitigen Position rotieren
	hamiltonCycle = hamiltonCycle->find(this->robot.position);
	
	// Abweichung zur letzten Position berechnen
	int signedDistance = hamiltonCycle->distance(this->path.back().position);
	int sign = sgn(signedDistance);

	// Fand ein Richtungswechsel statt?
	if (sign != prevDirection)
	{
		// "Strafe" fuers umdrehen
		signedDistance *= 2;
		prevDirection = sign;
	}

	// Abweichung von der Punktzahl abziehen
	score -= std::abs(signedDistance);
}

static node_t* oddHamilton(node_t *start)
{
	std::vector<bool> visited = std::vector<bool>(map.area);
	visited[start->point] = true;

	point_t origin = decode(start->point, map.size);
	point_t pos = origin;
	node_t *current = start;

	point_t corner;
	corner.x = pos.x <= map.size / 2 ? 0 : map.size - 1;
	corner.y = pos.y <= map.size / 2 ? 0 : map.size - 1;

	point_t dir = corner;
	dir.x -= map.size / 2;
	dir.y -= map.size / 2;
	dir.normalize();

	if (dir.x != dir.y) // Zuerst in y-Richtung gehen
		dir.x = 0;
	else                // Zuerst in x-Richtung gehen
		dir.y = 0;

	size_t count = 1;
	std::function<void(point_t)> addPoint = [&current, &visited, &count](point_t p)
	{
		current = (current->next = new node_t { current, NULL, encode(p, map.size) });
		visited[encode(p, map.size)] = true;
		count++;
	};

	// Zur Wand laufen
	while ((dir.x && pos.x != corner.x && pos.y != 0 && pos.y != map.size - 1)
		|| (dir.y && pos.y != corner.y && pos.x != 0 && pos.x != map.size - 1))
	{
		pos += dir;
		addPoint(pos);
	}

	// Zur Ecke laufen
	dir.rotate90();
	while((dir.x && pos.x != corner.x) || (dir.y && pos.y != corner.y))
	{
		pos += dir;
		addPoint(pos);
	}

	dir.rotate90();
	pos += dir;

	point_t orth = dir; // Orthogonale bilden
	orth.rotate90();

	std::function<bool(point_t)> bounded = [](point_t p)
	{
		return p.x >= 0 && p.x < map.size && p.y >= 0 && p.y < map.size;
	};

	while(count < map.area)
	{
		addPoint(pos);
		
		if (dir.x && (pos.x == origin.x + dir.x) && (pos.y == 0 || pos.y == map.size - 1))
		{
			dir.rotate90();
			orth.rotate90();
		}
		else if (dir.y && (pos.y == origin.y + dir.y) && (pos.x == 0 || pos.x == map.size - 1))
		{
			dir.rotate90();
			orth.rotate90();
		}

		point_t newPos = pos - orth;
		if (bounded(newPos) && !visited[encode(newPos, map.size)])
		{
			pos = newPos;
			continue;
		}

		newPos = pos + orth;
		if (bounded(newPos) && !visited[encode(newPos, map.size)])
		{
			pos = newPos;
			continue;
		}

		newPos = pos + dir;
		if (!bounded(newPos)) // Edge-case mit einen fehlenden Feld
		{
			for (int missing = 0; missing < map.area; missing++)
			{
				if (!visited[missing])
				{
					addPoint(decode(missing, map.size));
					break;
				}
			}
			break;

		}
		else if (!visited[encode(newPos, map.size)])
		{
			pos = newPos;
			continue;
		}
	}

	return current;
}

static node_t* evenHamilton(node_t *start)
{
	point_t pos = decode(start->point, map.size);
	point_t origin = pos;

	do
	{
		if(pos.x == 0 && pos.y < map.size - 1)
		{
			pos.y += 1;
			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
			continue;
		}

		if (pos.y == map.size - 1)
		{
			if (pos.x < map.size - 1)
				pos.x += 1;
			else
				pos.y -= 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
			continue;
		}

		if (pos.x % 2)
		{
			if (pos.y == 0)
				pos.x -= 1;
			else
				pos.y -= 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
		}
		else
		{
			if (pos.y == map.size - 2)
				pos.x -= 1;
			else
				pos.y += 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
		}

	} while (pos != origin);

	// Start ist wieder am Anfang, so waere der Anfang
	// zweimal enthalten
	start = start->prev;
	delete start->next;
	return start;
}

static void findHamilton()
{
	hamiltonCycle = new node_t { NULL, NULL, map.robot.position };

	if (map.size % 2)
		hamiltonCycle->prev = oddHamilton(hamiltonCycle);
	else
		hamiltonCycle->prev = evenHamilton(hamiltonCycle);

	hamiltonCycle->prev->next = hamiltonCycle;
}

// Testet ob die Batterien in getrennten Gruppen gebuendelt sind.
// Dies macht naemlich den Zustand unloesbar.
static bool checkClique(worker_t &state)
{
	typedef std::pair<uint32_t, std::vector<uint32_t>> clique_t;
	std::vector<clique_t> cliques = {{state.robot.charge, {state.robot.position}}};

	for (auto battery : state.batteries)
	{
		if (battery.second == 0) continue;

		std::vector<int> foundCliques;

		for (int i = 0; i < cliques.size(); i++)
		{
			for (auto member : cliques[i].second)
			{
				auto path = map.adjacency[battery.first][member];

				if (path->available)
				{
					if (path->length <= battery.second || path->length <= cliques[i].first)
					{
						foundCliques.push_back(i);
						break;
					}
				}
			}
		}

		if (foundCliques.size() > 1) // Gruppen zusammenfuehren
		{
			clique_t newClique = cliques[foundCliques[0]];

			for (int i = 1; i < foundCliques.size(); i++)
			{
				clique_t otherClique = cliques[foundCliques[i]];
				newClique.second.insert(newClique.second.end(), otherClique.second.begin(), otherClique.second.end());

				if (newClique.first < otherClique.first)
					newClique.first = otherClique.first;
			}

			newClique.second.push_back(battery.first);
			if (newClique.first < battery.second)
				newClique.first = battery.second;

			uint32_t deleted = 0;
			for (auto index : foundCliques)
				cliques.erase(cliques.begin() + index - (deleted++));


			cliques.push_back(newClique);
		}
		else if (foundCliques.size() == 1)
		{
			cliques[foundCliques[0]].second.push_back(battery.first);

			if (cliques[foundCliques[0]].first < battery.second)
				cliques[foundCliques[0]].first = battery.second;
		}
		else
		{
			cliques.push_back({ battery.second, { battery.first }});
		}
	}

	return cliques.size() == 1;
}

static worker_t solveConfig(bool debug)
{
	std::priority_queue<worker_t> queue;
	queue.push({map.robot, map.batteries, {}, 0, totalCharge});

	if (debug)
		std::cout << "Iterationen:" << std::endl;

	while(!queue.empty())
	{
		worker_t worker = queue.top();
		queue.pop();

		if (!checkClique(worker)) continue;

		if (debug)
			std::printf("Score: %-5i rem. charge: %-5lu\n", worker.score, worker.chargeSum);

		if (worker.chargeSum - worker.robot.charge == 0)
		{
			if (worker.robot.charge == 0)
				return worker; // Schon fertig

			bool dechargeable = false;
			point_t robotPos = decode(worker.robot.position, map.size);

			point_t dir = { 0, 1 };
			for (int i = 0; i < 4; i++, dir.rotate90())
			{
				if (dechargeable)
					break;
	
				point_t middle = robotPos + dir;
				if (middle.x < 0 || middle.x >= map.size || middle.y < 0 || middle.y >= map.size)
					continue;
	
				// Feld belegt?
				if (map.batteries.find(encode(middle, map.size)) != map.batteries.end())
					continue;

				if (worker.robot.charge < 3)
				{
					dechargeable = true;
					worker.path.push_back({ encode(middle, map.size), 1});
					break;
				}
	
				for (int j = 0; j < 4; j++, dir.rotate90())
				{
					point_t last = middle + dir;
					if (last.x < 0 || last.x >= map.size || last.y < 0 || last.y >= map.size)
						continue;
	
					if (map.batteries.find(encode(last, map.size)) == map.batteries.end())
					{
						dechargeable = true;
						worker.path.push_back({ encode(middle, map.size), 1});
						worker.path.push_back({ encode(last, map.size), 1});
						break;
					}
				}
			}

			if (dechargeable)
				return worker;
		}

		for (auto neighbour : map.adjacency[worker.robot.position])
		{
			if (!neighbour.second->available) continue;
			if (neighbour.first == map.robot.position) continue;
			if (worker.batteries[neighbour.first] == 0) continue;

			size_t min = neighbour.second->length;
			if (worker.robot.charge < min) continue;

			size_t max = neighbour.second->extendable ?
					std::max(worker.robot.charge, neighbour.second->extendedLength) : min;

			for (uint32_t distance = min; distance <= max; distance += 2)
			{
				if (worker.robot.charge < distance) break;

				worker_t newWorker = worker;

				uint32_t oldRobotCharge = newWorker.robot.charge - distance;
				newWorker.robot.charge = newWorker.batteries[neighbour.first];
				newWorker.batteries[neighbour.first] = oldRobotCharge;

				// Die Ladungsdifferenz abziehen
				newWorker.chargeSum -= distance;

				newWorker.path.push_back({ neighbour.first, distance });
				newWorker.calculateScore();
				
				// Die alte Position muss fuer die Scoreberechnung
				// erhalten bleiben
				newWorker.robot.position = neighbour.first;

				queue.push(newWorker);
			}
		}
	}

	if (debug)
		std::cout << '\n' << std::endl;

	return {};
}

static std::vector<point_t> constructPath(worker_t &solution)
{
	// Keine Loesung
	if (solution.path.size() == 0)
		return {};

	std::vector<point_t> path;

	for(int i = 0; i < solution.path.size(); i++)
	{
		// Letzte 2 Punkte im Pfad geben die freien Felder um den Roboter an
		if (solution.robot.charge > 2 && i >= solution.path.size() - 2)
			break;
 
		// Letzer Punkt im Pfad gibt das freie Feld beim Roboter an
		if (solution.robot.charge > 0 && i >= solution.path.size() - 1)
			break;

		uint32_t nodeA;
		if (i == 0) // Urspruengliche Position des Roboters nehmen
			nodeA = map.robot.position;
		else
			nodeA = solution.path[i - 1].position;

		uint32_t nodeB = solution.path[i].position;

		point_t pA = decode(nodeA, map.size);
		point_t pB = decode(nodeB, map.size);

		auto edgePath = map.adjacency[nodeA][nodeB];
		uint32_t distance = solution.path[i].distance;

		if (distance == edgePath->length)
		{
			// Kuerzesten Pfad kopieren
			
			// Muss der Pfad umgedreht werden
			if (edgePath->shortest[0] == pA)
				path.insert(path.end(), edgePath->shortest.begin() + 1, edgePath->shortest.end());
			else
				// Pfad rueckwaerts einfuegen
				path.insert(path.end(), edgePath->shortest.rbegin() + 1, edgePath->shortest.rend());
		}
		else
		{
			// Verlaengerbaren Pfad entsprechend
			// verlaengern und einfuegen
			
			std::vector<point_t> extended;
			
			// Noetige Verlaengerung berechnen
			uint32_t extension = distance - edgePath->extendedLength;

			size_t end = edgePath->extended.size() - 1;

			// Verlaengerung zum Pfad hinzufuegen
			uint32_t current = 0;
			while(extension > 0)
			{
				if (edgePath->extended[0] == pA)
					extended.push_back(edgePath->extended[1 + current]);
				else
					extended.push_back(edgePath->extended[end - 1 - current]);

				current = (current + 1) % 2;
				extension--;
			}
			path.insert(path.end(), extended.begin(), extended.end());

			// Restlichen Pfad hinzufuegen
			if (edgePath->extended[0] == pA)
				path.insert(path.end(), edgePath->extended.begin() + 1, edgePath->extended.end());
			else
				path.insert(path.end(), edgePath->extended.rbegin() + 1, edgePath->extended.rend());
		}
	}
	
	if (solution.robot.charge > 2)
	{
		size_t size = solution.path.size();
		int current = 1;
		while(solution.robot.charge > 0)
		{
			path.push_back(decode(solution.path[size - 1 - current].position, map.size));
			current = (current + 1) % 2; // Immer zwischen beiden Positionen wechseln
			solution.robot.charge--;
		}
	}
	else if (solution.robot.charge > 0)
	{
		path.push_back(decode(solution.path.back().position, map.size));
		if (solution.robot.charge == 2)
			path.push_back(decode(solution.robot.position, map.size));
	}

	return path;
}

static void readFile(const char *path)
{
	std::ifstream file(path);
	std::regex r("\\d+");

	if (file.is_open())
	{
		std::function<std::string(std::ifstream&)> readLine = [](std::ifstream &file)
		{
			std::string line;
			if (!std::getline(file, line))
			{
				std::cerr << "Invalid file format!" << std::endl;
				exit(-1);
			}
			return line;
		};

		map.size = std::stoi(readLine(file));
		map.area = map.size * map.size;
		totalCharge = 0;

		std::string line = readLine(file);
		std::sregex_iterator it(line.begin(), line.end(), r);
		int x = std::stoi((*it)[0]) - 1;
		int y = std::stoi((*(++it))[0]) - 1;
		uint32_t c = std::stoi((*(++it))[0]);
		map.robot = { encode({ x, y }, map.size), c };
		totalCharge += c;

		size_t count = std::stoull(readLine(file));

		while(count--)
		{
			std::string line = readLine(file);
			std::sregex_iterator it(line.begin(), line.end(), r);
			int x = std::stoi((*it)[0]) - 1;
			int y = std::stoi((*(++it))[0]) - 1;
			uint32_t c = std::stoi((*(++it))[0]);
			map.batteries.insert({ encode({ x, y }, map.size), c });
			totalCharge += c;
		}
	}
	else
	{
		std::cerr << "Fehlerhafte oder korrupte Datei:  " << path << "!" << std::endl;
		exit(-1);
	}
}

/*************************************************
 *     Oeffentliche Methodenimplementationen     *
 *************************************************/

std::vector<point_t> solveGame(const char* path, bool debug)
{
	readFile(path);

	findHamilton();

	if (debug)
	{
		std::cout << "Hamiltonkreis:" << std::endl;

		node_t *current = hamiltonCycle->next;

		std::cout << "X---->" << hamiltonCycle->point << "-->";

		while(current != hamiltonCycle->prev)
		{
			std::cout << current->point << "-->";
			current = current->next;
		}

		std::cout << hamiltonCycle->prev->point << "---->X"
				  << '\n' <<  std::endl;
	}

	parseGraph(map);
	worker_t solution = solveConfig(debug);
	return constructPath(solution);
}
