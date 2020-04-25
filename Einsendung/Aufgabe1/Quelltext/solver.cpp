#include "solver.hpp"

#include <map>
#include <deque>
#include <queue>
#include <fstream>
#include <iostream>
#include <functional>
#include <regex>

#include "point.hpp"
#include "global.hpp"

/********************
 *     Typedefs     *
 ********************/

// Repraesentiert einen
// Knoten im Hamiltonkreis
typedef struct node_t {
	node_t *prev;
	node_t *next;

	uint32_t point;

	~node_t();
	
	int distance(uint32_t p);
	node_t* find(uint32_t p);
} node_t;

// Repraesentiert einen Zug
// im Pfad
typedef struct distance_pair_t {
	uint32_t position; // Neue Position
	uint32_t distance; // Gegangene Strecke
} distance_pair_t;

// Repraesentiert einen Pfad
typedef struct worker_t {
	std::vector<distance_pair_t> path;
	int score;
	int prevDirection = 0;

	void calculateScore(state_t &state);
	bool operator<(const worker_t &rhs) const;
} worker_t;

/*******************************
 *     Statische Variablen     *
 *******************************/

static node_t *hamiltonCycle; // Hamiltonkreis
static map_t map; // Spielfeld

/*******************************************
 *     Statische Methodendeklarationen     *
 *******************************************/

static node_t* oddHamilton(node_t *start);
static node_t* evenHamilton(node_t *start);
static void findHamilton();

static std::vector<uint32_t> batteryBFS(uint32_t start, uint32_t end, state_t &state);
static bool checkClique(state_t &state);

static state_t reconstructState(worker_t &path);

static std::pair<worker_t, state_t> solveConfig(bool debug);
static std::vector<point_t> constructPath(worker_t &solution);

static void readFile(const char *path);

/**********************************************
 *     Statische Methodenimplementationen     *
 **********************************************/

// Sucht den Punkt im Ring und gibt
// dessen Distanz zurueck
int node_t::distance(uint32_t p) {
	int best;

	// In Ringrichtung gehen
	node_t* current = this;
	int dist = 0;
	do {
		if (!current->next) break;
		dist++; // Positive Distanz
		current = current->next;
	} while(current->point != p && current != this);

	best = dist;

	// Entgegen der Ringrichtung gehen
	current = this;
	dist = 0;
	do {
		if (!current->prev) break;
		dist--; // Negative Distanz
		current = current->prev;
	} while(current->point != p && current != this);

	// Betragsmaessig kleinste
	// Distanz nehmen
	if (std::abs(dist) < best)
		best = dist;

	return best;
}

// Schliesst die Luecke im Ring wenn noetig
node_t::~node_t() {
	if (prev) prev->next = next;
	if (next) next->prev = prev;
}

// Findet die Node im Ring mit dem angegebenen Wert
node_t* node_t::find(uint32_t p) {
	if (point == p)
		return this;

	node_t *start = this;
	node_t *current = this;

	do {
		if (!current->next) break;
		current = current->next;
	} while(current != start && current->point != p);

	return current;
}

// Sortiert die Pfade nach ihrer Punktzahl
bool worker_t::operator<(const worker_t &rhs) const {
	return score < rhs.score;
}

// Berechnet die Punktzahl des Pfades
// Beachte, dass der State noch auf dem alten Stand ist
void worker_t::calculateScore(state_t &state) {
	// Entladene Ladung berechnen
	score = map.chargeSum - (state.chargeSum - this->path.back().distance);
	score *= map.area / state.batteries.size(); // Ladungswert skalieren

	// Hamiltonkreis zur alten Position rotieren
	hamiltonCycle = hamiltonCycle->find(state.robot.position);
	
	// Abweichung zur neuen Position berechnen
	int signedDistance = hamiltonCycle->distance(this->path.back().position);
	int sign = sgn(signedDistance);

	// Fand ein Richtungswechsel statt?
	if (sign != prevDirection) {
		// "Strafe" fuers umdrehen
		signedDistance *= 2;
		prevDirection = sign;
	}

	// Abweichung von der Punktzahl abziehen
	score -= std::abs(signedDistance);
}

// Bildet den Hamiltonkreis bei ungerader Groesse
static node_t* oddHamilton(node_t *start) {
	size_t count = 1; // Anzahl hinzugefügter Knoten

	std::vector<bool> visited = std::vector<bool>(map.area);
	visited[start->point] = true;

	point_t origin = decode(start->point, map.size);
	point_t pos = origin;
	node_t *current = start;

	// Ecke finden
	point_t corner;
	corner.x = pos.x <= map.size / 2 ? 1 : map.size;
	corner.y = pos.y <= map.size / 2 ? 1 : map.size;

	// Richtung zur Ecke bestimmen
	point_t dir = corner;
	dir.x -= map.size / 2;
	dir.y -= map.size / 2;
	dir.normalize();

	// Sicherstellen, dass dannach rotiert werden kann
	if (dir.x != dir.y) // Zuerst in y-Richtung gehen
		dir.x = 0;
	else                // Zuerst in x-Richtung gehen
		dir.y = 0;

	// Helfermethode fuegt einen Punkt zum Hamiltonkreis hinzu
	std::function<void(point_t)> addPoint = [&current, &visited, &count](point_t p) {
		current = (current->next = new node_t { current, NULL, encode(p, map.size) });
		visited[encode(p, map.size)] = true;
		count++;
	};

	// Zur Wand laufen (oder nicht im Edge-case)
	while ((dir.x && pos.x != corner.x && pos.y != 1 && pos.y != map.size)
		|| (dir.y && pos.y != corner.y && pos.x != 1 && pos.x != map.size))
	{
		pos += dir;
		addPoint(pos);
	}

	// Zur Ecke laufen
	dir.rotate90();
	while((dir.x && pos.x != corner.x) || (dir.y && pos.y != corner.y)) {
		pos += dir;
		addPoint(pos);
	}

	dir.rotate90();
	pos += dir;

	point_t orth = dir; // Orthogonale bilden
	orth.rotate90();

	while(count < map.area) {
		addPoint(pos);
		
		// Ueber den Ursprung getreten?
		// --> Um 90 Grad drehen
		if (dir.x && (pos.x == origin.x + dir.x) && (pos.y == 1 || pos.y == map.size)) {
			dir.rotate90();
			orth.rotate90();
		}
		else if (dir.y && (pos.y == origin.y + dir.y) && (pos.x == 1 || pos.x == map.size)) {
			dir.rotate90();
			orth.rotate90();
		}

		// Entlang der Orthogonalen gehen
		point_t newPos = pos - orth;
		if (newPos.bounded(1, 1, map.size, map.size) && !visited[encode(newPos, map.size)]) {
			pos = newPos;
			continue;
		}

		newPos = pos + orth;
		if (newPos.bounded(1, 1, map.size, map.size) && !visited[encode(newPos, map.size)]) {
			pos = newPos;
			continue;
		}

		// Entlang der Blickrichtung gehen
		newPos = pos + dir;
		// Edge-case mit einen fehlenden Feld
		if (!newPos.bounded(1, 1, map.size, map.size)) {
			for (int missing = 0; missing < map.area; missing++) {
				if (!visited[missing]) {
					addPoint(decode(missing, map.size));
					break;
				}
			}
			break;
		}
		else if (!visited[encode(newPos, map.size)]) {
			pos = newPos;
			continue;
		}
	}

	return current;
}

// Bildet den Hamiltonkreis bei gerader Groesse
static node_t* evenHamilton(node_t *start) {
	point_t pos = decode(start->point, map.size);
	point_t origin = pos;

	do {
		// An der linken Wand entlang
		if(pos.x == 1 && pos.y < map.size) {
			pos.y += 1;
			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
			continue;
		}

		// An der unteren Wand eintlang
		if (pos.y == map.size) {
			if (pos.x < map.size)
				pos.x += 1;
			else
				pos.y -= 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
			continue;
		}

		// Je nach Position nach oben
		// oder unten schlaengeln
		if (pos.x % 2) {
			if (pos.y == map.size - 1)
				pos.x -= 1;
			else
				pos.y += 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
		}
		else {
			if (pos.y == 1)
				pos.x -= 1;
			else
				pos.y -= 1;

			start = (start->next = new node_t { start, NULL, encode(pos, map.size) });
		}
	} while (pos != origin);

	// Pointer ist wieder am Anfang, so waere
	// der Anfang zweimal enthalten
	start = start->prev;
	delete start->next;
	return start;
}

// Bildet den Hamiltonkreis der Spielsituation
static void findHamilton() {
	hamiltonCycle = new node_t { NULL, NULL, map.robot.position };

	if (map.size % 2)
		hamiltonCycle->prev = oddHamilton(hamiltonCycle);
	else
		hamiltonCycle->prev = evenHamilton(hamiltonCycle);

	hamiltonCycle->prev->next = hamiltonCycle;
}

// Testet ob die Batterien in getrennten Gruppen gebuendelt sind.
// Dies macht naemlich den Zustand unloesbar.
static bool checkClique(state_t &state) {
	// Stellt eine Gruppe dar
	typedef std::pair<uint32_t, std::vector<uint32_t>> group_t;

	// Gruppenliste mit erster Gruppe initialisieren
	std::vector<group_t> groups = {{state.robot.charge, {state.robot.position}}};

	for (auto battery : state.batteries) {
		// Leere Batterien ignorieren
		if (battery.second == 0)
			continue;

		// Alle zugehoerigen Gruppen der Batterie
		std::vector<int> foundGroups;

		for (int i = 0; i < groups.size(); i++) {
			// Alle Batterien in der Gruppe durchgehen
			for (auto member : groups[i].second) {
				auto path = map.adjacency[battery.first][member];

				if (path->available) {
					// Pfadlaenge kleiner als Batterieladung
					// oder maximale Gruppenladung
					if (path->length <= battery.second || path->length <= groups[i].first) {
						foundGroups.push_back(i);
						break;
					}
				}
			}
		}

		// Gruppen zusammenfuehren
		if (foundGroups.size() > 1) {
			group_t newGroup = groups[foundGroups[0]];

			// Zusammenfuehren
			for (int i = 1; i < foundGroups.size(); i++) {
				group_t otherGroup = groups[foundGroups[i]];
				newGroup.second.insert(
					newGroup.second.end(),
					otherGroup.second.begin(),
					otherGroup.second.end()
				);

				if (newGroup.first < otherGroup.first)
					newGroup.first = otherGroup.first;
			}

			// Batterie hinzufuegen
			newGroup.second.push_back(battery.first);

			// Rekordladung erneuern
			if (newGroup.first < battery.second)
				newGroup.first = battery.second;

			// Alte Gruppe loeschen
			uint32_t deleted = 0;
			for (auto index : foundGroups)
				groups.erase(groups.begin() + index - (deleted++));

			// Neue Gruppe hinzufuegen
			groups.push_back(newGroup);
		}
		// Zur Gruppe hinzufuegen
		else if (foundGroups.size() == 1) {
			// Batterie hinzufuegen und Rekordladung erneuern
			groups[foundGroups[0]].second.push_back(battery.first);
			if (groups[foundGroups[0]].first < battery.second)
				groups[foundGroups[0]].first = battery.second;
		}
		else // Neue Gruppe erstellen
			groups.push_back({ battery.second, { battery.first }});
	}

	// Eine Grosse Gruppe vorhanden?
	return groups.size() == 1;
}

// Rekonstruiert das Spielfeld aus der Zugfolge
static state_t reconstructState(worker_t &path) {
	// Ausganssituation kopieren
	state_t state = {map.robot, map.batteries, map.chargeSum};

	for (auto move : path.path) {
		// Zug durchfuehren
		uint32_t oldCharge = state.robot.charge - move.distance;
		state.robot.charge = state.batteries[move.position];
		state.batteries[move.position] = oldCharge;

		state.chargeSum -= move.distance;
	}

	// Position des Roboters setzten
	if (path.path.size() > 0)
		state.robot.position = path.path.back().position;

	return state;
}

// Implementation des Bruteforce-Heuristik Algorithmus
static std::pair<worker_t, state_t> solveConfig(bool debug)
{
	// Prioritaetswarteschlange an Pfaden
	std::priority_queue<worker_t> queue;
	queue.push(worker_t());

	if (debug)
		std::cout << "Iterationen:" << std::endl;

	while(!queue.empty()) {
		worker_t worker = queue.top();
		queue.pop();

		// Spielfeld rekonstruieren
		state_t state = reconstructState(worker);

		// Loesbarkeit ueberprufen
		if (!checkClique(state))
			continue;

		if (debug)
			std::printf("Score: %-5i rem. charge: %-5lu\n", worker.score, state.chargeSum);

		// Alle Batterien leer?
		if (state.chargeSum - state.robot.charge == 0) {
			if (state.robot.charge == 0)
				return { worker, state }; // Schon fertig

			// Pfad generieren
			point_t robotPos = decode(state.robot.position, map.size);
			auto path = findPath(robotPos, robotPos, map);

			if (state.robot.charge == 2) {
				if (path->available) {
					worker.path.push_back({ encode(path->shortest[1], map.size), 1});
					return {worker, state};
				}
			}
			else if (state.robot.charge > 2) {
				if (path->extendable) {
					worker.path.push_back({ encode(path->shortest[1], map.size), 1});
					worker.path.push_back({ encode(path->shortest[2], map.size), 1});
					return {worker, state};
				}
			}
			else {
				// Umliegendes Feld suchen
				point_t dir = { 0, 1 };
				for (int i = 0; i < 4; i++, dir.rotate90()) {
					point_t middle = robotPos + dir;
					if (!middle.bounded(1, 1, map.size, map.size))
						continue;

					worker.path.push_back({ encode(middle, map.size), 1});
					return {worker, state};
				}
			}

			continue; // Doch keine Loesung
		}

		// Benachbarte Batterien durchlaufen
		for (auto neighbour : map.adjacency[state.robot.position]) {
			if (!neighbour.second->available) //Pfad nicht verfuegbar?
				continue;
			if (neighbour.first == map.robot.position) // Keine Batterie?
				continue;
			if (state.batteries[neighbour.first] == 0) // Batterie ist leer?
				continue;

			// Laenge des kuerzesten Pfades
			uint32_t min = neighbour.second->length;
			if (state.robot.charge < min) continue;

			// Kuerzesten Pfad gehen
			worker_t newWorker = worker;
			newWorker.path.push_back({neighbour.first, min});
			newWorker.calculateScore(state);
			queue.push(newWorker);

			// Eventuell den verlaengerbaren Pfad gehen
			if (neighbour.second->extendable) {
				min = neighbour.second->extendedLength;
				uint32_t max = state.robot.charge;

				// Denselben Pfad nicht zweimal gehen
				if (min == neighbour.second->length)
					min += 2;

				// Pfad kann immer um 2 verlaengert werden
				// Entspricht dann einmal hin und her fahren
				for (uint32_t distance = min; distance <= max; distance += 2) {
					// Verlaengerten Pfad gehen
					worker_t newWorker = worker;
					newWorker.path.push_back({ neighbour.first, distance });
					newWorker.calculateScore(state);
					queue.push(newWorker);
				}
			}
		}
	}

	if (debug)
		std::cout << '\n' << std::endl;

	return {};
}

// Rekonstruiert die Schrittfolge aus der Zugfolge
static std::vector<point_t> constructPath(worker_t &solution, state_t &state) {
	// Keine Loesung
	if (solution.path.size() == 0)
		return {};

	std::vector<point_t> path;

	for(int i = 0; i < solution.path.size(); i++) {
		// Letzte 2 Punkte im Pfad geben die freien Felder um den Roboter an
		if (state.robot.charge > 2 && i >= solution.path.size() - 2)
			break;
 
		// Letzer Punkt im Pfad gibt das freie Feld beim Roboter an
		if (state.robot.charge > 0 && i >= solution.path.size() - 1)
			break;

		// Start- und Endknoten des Zuges abrufen
		uint32_t nodeA;
		if (i == 0) // Urspruengliche Position des Roboters nehmen
			nodeA = map.robot.position;
		else
			nodeA = solution.path[i - 1].position;

		uint32_t nodeB = solution.path[i].position;

		point_t pA = decode(nodeA, map.size);
		point_t pB = decode(nodeB, map.size);

		// Pfad zwischen beiden Knoten abrufen
		auto edge = map.adjacency[nodeA][nodeB];
		uint32_t distance = solution.path[i].distance;

		// Pfad zur Schrittfolge hinzufuegen.
		// Dabei immer den ersten Punkt ueberspringen,
		// da der Roboter dort schon ist.

		if (distance == edge->length) {
			// Kuerzesten Pfad kopieren
			// Muss der Pfad umgedreht werden?
			if (edge->shortest[0] == pA)
				path.insert(path.end(), edge->shortest.begin() + 1, edge->shortest.end());
			else
				// Pfad rueckwaerts einfuegen
				path.insert(path.end(), edge->shortest.rbegin() + 1, edge->shortest.rend());
		}
		else {
			// Verlaengerbaren Pfad entsprechend
			// verlaengern und einfuegen
			std::vector<point_t> extended;
			
			// Noetige Verlaengerung berechnen
			uint32_t extension = distance - edge->extendedLength;
			size_t end = edge->extended.size() - 1;

			// Verlaengerung zum Pfad hinzufuegen
			uint32_t current = 0;
			while(extension > 0) {
				if (edge->extended[0] == pA)
					extended.push_back(edge->extended[1 + current]);
				else
					extended.push_back(edge->extended[end - 1 - current]);

				current = (current + 1) % 2; // Immer hin und her
				extension--;
			}
			path.insert(path.end(), extended.begin(), extended.end());

			// Restlichen Pfad hinzufuegen
			if (edge->extended[0] == pA)
				path.insert(path.end(), edge->extended.begin() + 1, edge->extended.end());
			else
				path.insert(path.end(), edge->extended.rbegin() + 1, edge->extended.rend());
		}
	}

	if (state.robot.charge > 2) {
		// Letzte beiden Felder so oft wie noetig wiederholen
		size_t size = solution.path.size();
		int current = 1;
		while(state.robot.charge > 0) {
			path.push_back(decode(solution.path[size - 1 - current].position, map.size));
			current = (current + 1) % 2; // Immer hin und her gehen
			state.robot.charge--;
		}
	}
	else if (state.robot.charge > 0) {
		// Das freie Feld zum Pfad hinzufuegen
		path.push_back(decode(solution.path.back().position, map.size));

		// Eventuell wieder auf das vorherige Feld gehen
		if (state.robot.charge == 2)
			path.push_back(decode(state.robot.position, map.size));
	}

	return path;
}

// Liesst die Eingabedatei ein
static void readFile(const char *path) {
	std::ifstream file(path);
	std::regex r("\\d+");

	if (file.is_open()) {
		std::function<std::string(std::ifstream&)> readLine = [](std::ifstream &file) {
			std::string line;
			if (!std::getline(file, line)) {
				std::cerr << "Invalid file format!" << std::endl;
				exit(-1);
			}
			return line;
		};

		map.size = std::stoi(readLine(file));
		map.area = map.size * map.size;
		map.chargeSum = 0;

		std::string line = readLine(file);
		std::sregex_iterator it(line.begin(), line.end(), r);
		int x = std::stoi((*it)[0]);
		int y = std::stoi((*(++it))[0]);
		uint32_t c = std::stoi((*(++it))[0]);
		map.robot = { encode({ x, y }, map.size), c };
		map.chargeSum += c;

		size_t count = std::stoull(readLine(file));

		while(count--) {
			std::string line = readLine(file);
			std::sregex_iterator it(line.begin(), line.end(), r);
			int x = std::stoi((*it)[0]);
			int y = std::stoi((*(++it))[0]);
			uint32_t c = std::stoi((*(++it))[0]);
			map.batteries.insert({ encode({ x, y }, map.size), c });
			map.chargeSum += c;
		}
	}
	else {
		std::cerr << "Fehlerhafte oder korrupte Datei:  " << path << "!" << std::endl;
		exit(-1);
	}
}

/*************************************************
 *     Oeffentliche Methodenimplementationen     *
 *************************************************/

// Loesst die gegebene Spielsituation
std::vector<point_t> solveGame(const char* path, bool debug) {
	readFile(path); // Eingabedatei parsen

	findHamilton(); // Hamiltonkreis kreieren

	if (debug) {
		std::cout << "Hamiltonkreis:" << std::endl;

		node_t *current = hamiltonCycle->next;

		std::cout << "X---->" << hamiltonCycle->point << "-->";

		while(current != hamiltonCycle->prev) {
			std::cout << current->point << "-->";
			current = current->next;
		}

		std::cout << hamiltonCycle->prev->point << "---->X"
				  << '\n' <<  std::endl;
	}

	parseGraph(map); // Adjazenzliste kreieren
	auto solution = solveConfig(debug); // Loesen

	// Schrittfolge erstellen und zurueckgeben
	return constructPath(solution.first, solution.second);
}
