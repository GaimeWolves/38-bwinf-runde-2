#include "global.hpp"

#include <queue>

uint32_t encode(point_t p, uint32_t size) {
	return (p.x - 1) + (p.y - 1) * size;
}

point_t decode(uint32_t n, uint32_t size) {
	return { (int)(n % size) + 1, (int)(n / size) + 1 };
}

// Abstrahiert das Spielfeld zu einem Graphen
void parseGraph(map_t &map) {
	map.adjacency = adjacency_t();
	std::map<uint32_t, uint32_t>::iterator u, v;
	
	point_t robot = decode(map.robot.position, map.size);

	// Fuer jede Batterie ...
	for (u = map.batteries.begin(); u != map.batteries.end(); ++u) {
		point_t pU = decode(u->first, map.size);
		map.adjacency[u->first] = std::map<uint32_t, std::shared_ptr<path_t>>();
		
		// ... zu jeder Batterie ... (auch sich selbst)
		for (v = map.batteries.begin(); v != map.batteries.end(); ++v) {
			point_t pV = decode(v->first, map.size);
			
			if (map.adjacency.find(v->first) == map.adjacency.end())
				map.adjacency[v->first] = std::map<uint32_t, std::shared_ptr<path_t>>();

			// ... einen Pfad generieren.
			auto path = findPath(pU, pV, map);
			map.adjacency[u->first][v->first] = path;
			map.adjacency[v->first][u->first] = path;
		}

		// Und auch zum Roboter
		auto path = findPath(pU, robot, map);
		map.adjacency[u->first][map.robot.position] = path;
		map.adjacency[map.robot.position][u->first] = path;
	}

	// Zur Vollstaendigkeit halber auch vom Roboter zum Roboter
	auto path = findPath(robot, robot, map);
	map.adjacency[map.robot.position][map.robot.position] = path;
}

// Findet den kuerzesten und verlaengerbaren Pfad zw. zwei Knoten
std::shared_ptr<path_t> findPath(point_t start, point_t end, map_t &map) {
	std::vector<point_t> shortest;
	// Schleifenpfad ist gefragt
	if (start == end) {
		// Freie Nachbarfelder suchen
		point_t dir = { 0, 1 };
		for (int i = 0; i < 4; i++, dir.rotate90()) {
			// Nachbarfeld
			point_t pos = start + dir;
			if (!pos.bounded(1, 1, map.size, map.size))
				continue;

			// Ist es frei?
			if (map.batteries.find(encode(start + dir, map.size)) == map.batteries.end()) {
				// Pfad generieren
				shortest = { start, start + dir, start };
				break;
			}
		}
	}
	else // Ansonsten BFS zum Pfad suchen benutzen
		shortest = BFS(start, end, map, false);

	std::shared_ptr<path_t> path = std::make_shared<path_t>();

	path->available = shortest.size() >= 2;
	path->shortest = shortest;
	path->length = shortest.size() - 1;

	// Pfad nicht verfuegbar?
	if (!path->available)
		return path;
	else if (path->length > 2) { // Schon verlaengerbar?
		path->extendable = true;
		path->extended = path->shortest;
		path->extendedLength = path->length;
		return path;
	}

	std::vector<point_t> extended;

	// Verlaengerbarer Schlaufenpfad gefragt
	// Also komplette Raute untersuchen
	if (start == end) {
		// Nachbarfelder untersuchen
		point_t dir = { 0, 1 };
		for (int i = 0; i < 4; i++, dir.rotate90()) {
			if (extended.size() > 0)
				break;

			// Nachbarfeld
			point_t middle = start + dir;
			if (!middle.bounded(1, 1, map.size, map.size))
				continue;

			if (map.batteries.find(encode(middle, map.size)) != map.batteries.end())
				continue;

			// Randfelder der Raute untersuchen
			for (int j = 0; j < 4; j++, dir.rotate90()) {
				// Randfeld der Raute
				point_t last = middle + dir;
				if (!last.bounded(1, 1, map.size, map.size))
					continue;

				// Ist es frei?
				if (map.batteries.find(encode(last, map.size)) == map.batteries.end()) {
					// Verlaengerbaren Pfad generieren
					extended = { start, middle, last, middle, start };
					break;
				}
			}
		}
	}
	else // Ansonsten wieder BFS benutzen
		extended = BFS(start, end, map, true);

	// Pfad nicht verfuegbar?
	if (extended.size() < 2)
		return path;

	path->extendable = true;
	path->extended = extended;
	path->extendedLength = extended.size() - 1;

	return path;
}

// Sucht einen Pfad zw. zwei Knoten
std::vector<point_t> BFS(point_t start, point_t goal, map_t &map, bool extendable) {
	std::queue<point_t> queue;
	queue.push(start);
	
	std::vector<bool> visited = std::vector<bool>(map.size * map.size);
	visited[encode(start, map.size)] = true;

	std::map<point_t, point_t> parent;

	// Hoechstens zwei zu ignoriererende Kanten
	std::pair<point_t, point_t> skip1 = { };
	std::pair<point_t, point_t> skip2 = { };

	// Verlaengerbarer Pfad gefragt?
	// Zu blockierende Kanten suchen
	if (extendable) {
		// Raute ueberprufen
		point_t parallel = { 0, 1 }; // Parallele Felder
		point_t diagonal = { 1, 1 }; // Diagonale Felder

		for (int i = 0; i < 4; i++, parallel.rotate90(), diagonal.rotate90()) {
			// Zu blockierende Kanten ueberpruefen
			if (start + parallel == goal) { // Direkt benachbart?
				// Eine Kante blockieren
				skip1 = { start, goal };
				break;
			}
			else if (start + parallel * 2 == goal) { // Zwei Felder entfernt?
				// Eine Kante blockieren
				skip1 = { start + parallel, goal };
				break;
			}
			else if (start + diagonal == goal) { // Diagonal zur Batterie?
				// zwei Kanten blockieren
				skip1 = { start + point_t{ diagonal.x, 0 }, goal };
				skip2 = { start + point_t{ 0, diagonal.y }, goal };
				break;
			}
		}
	}

	// Normalen BFS durchfuehren
	while(!queue.empty()) {
		point_t u = queue.front();
		queue.pop();
		if (u == goal)
			break;

		// Benachbarte Felder untersuchen
		point_t dir = { 1, 0 };
		for (int i = 0; i < 4; i++, dir.rotate90()) {
			point_t v = u + dir;
			uint32_t nodeV = encode(v, map.size);
			
			// Ist die Kante blockiert worden?
			std::pair<point_t, point_t> edge = { u, v };
			if (edge == skip1 || edge == skip2)
				continue;
			
			// Ist das Feld belegt oder schon besucht worden?
			if (!v.bounded(1, 1, map.size, map.size)) continue;
			if (visited[nodeV]) continue;
			if (v != goal && map.batteries.find(nodeV) != map.batteries.end()) continue;
		
			// Feld als besucht markieren und Vorgaengerkarte erneuern
			visited[nodeV] = true;
			parent[v] = u;
			queue.push(v);
		}
	}

	std::vector<point_t> path;

	// Kein Pfad vorhanden?
	if (parent.find(goal) == parent.end())
		return path;

	// Pfad aus Vorgaengerkarte generieren
	point_t u = goal;
	while(u != start) {
		path.push_back(u);
		u = parent[u];
	}
	path.push_back(start);
	
	return path;
}
