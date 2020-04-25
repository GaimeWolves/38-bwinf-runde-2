#include "solver.hpp"

#include <map>
#include <deque>
#include <queue>
#include <fstream>
#include <iostream>
#include <functional>
#include <regex>

#include "point.h"

/********************
 *     Typedefs     *
 ********************/

typedef struct path_t
{
	bool available;
	std::vector<point_t> shortest;
	uint32_t length;

	// Längerer möglicher Pfad
	bool extendable;
	std::vector<point_t> extended;
	uint32_t extendedLength;
} path_t;

typedef struct state_t
{
	uint32_t rotation;
	uint32_t length;
	uint32_t position;
} state_t;

typedef struct robot_t
{
	uint32_t position;
	uint32_t charge;
} robot_t;

typedef struct map_t
{
	uint32_t size;
	std::map<uint32_t, std::map<uint32_t, path_t*>> adjacency;
	std::map<uint32_t, uint32_t> batteries;
	std::deque<uint32_t> hamiltonPath;
	robot_t robot;
} map_t;

/*******************************************
 *     Statische Methodendeklarationen     *
 *******************************************/

static inline uint32_t pack(point_t point, uint32_t size);
static inline point_t unpack(uint32_t node, uint32_t size);

static std::deque<uint32_t> oddHamilton(point_t start, uint32_t size);
static std::deque<uint32_t> evenHamilton(point_t start, uint32_t size);
static std::deque<uint32_t> findHamilton(map_t &map);

static std::vector<point_t> BFS(map_t &map, point_t start, point_t goal, std::pair<point_t, point_t> skip);
static path_t* findPath(map_t &map, point_t start, point_t goal);
static void parseGraph(map_t &map);

static std::vector<state_t> solve(map_t &map);
static std::vector<point_t> constructPath(map_t &map, std::vector<point_t> &ordering);

static map_t readFile(const char *path);

/**********************************************
 *     Statische Methodenimplementationen     *
 **********************************************/

static inline uint32_t pack(point_t point, uint32_t size)
{
	return point.x + point.y * size;
}

static inline point_t unpack(uint32_t node, uint32_t size)
{
	return { (int)(node % size), (int)(node / size) };
}

static std::deque<uint32_t> oddHamilton(point_t start, uint32_t size)
{
	std::deque<uint32_t> path = { pack(start, size) };
	path.resize(size * size);
	int index = 1;

	std::vector<bool> visited = std::vector<bool>(size * size);
	visited[pack(start, size)] = true;

	point_t corner;
	corner.x = start.x <= size / 2 ? 0 : size - 1;
	corner.y = start.y <= size / 2 ? 0 : size - 1;

	point_t dir = corner;
	dir.x -= size / 2;
	dir.y -= size / 2;
	dir.normalize();

	point_t pos = start;

	if (dir.x != dir.y) // Zuerst in y-Richtung gehen
		dir.x = 0;
	else                // Zuerst in x-Richtung gehen
		dir.y = 0;

	// Zur Wand laufen
	while((dir.x && pos.x != corner.x) || (dir.y && pos.y != corner.y))
	{
		pos += dir;
		path[index++] = pack(pos, size);
		visited[pack(pos, size)] = true;
	}

	// Zur Ecke laufen
	dir.rotate90();
	while(pos.x != corner.x || pos.y != corner.y)
	{
		pos += dir;
		path[index++] = pack(pos, size);
		visited[pack(pos, size)] = true;
	}

	dir.rotate90();
	pos += dir;

	point_t orth = dir; // Orthogonale bilden
	orth.rotate90();

	std::function<bool(point_t)> bounded = [size](point_t p)
	{
		return p.x >= 0 && p.x < size && p.y >= 0 && p.y < size;
	};

	while(index < size * size)
	{
		path[index++] = pack(pos, size);
		visited[pack(pos, size)] = true;
		
		if (dir.x && (pos.x == start.x + dir.x) && (pos.y == 0 || pos.y == size - 1))
		{
			dir.rotate90();
			orth.rotate90();
		}
		else if (dir.y && (pos.y == start.y + dir.y) && (pos.x == 0 || pos.x == size - 1))
		{
			dir.rotate90();
			orth.rotate90();
		}

		point_t newPos = pos - orth;
		if (bounded(newPos) && !visited[pack(newPos, size)])
		{
			pos = newPos;
			continue;
		}

		newPos = pos + orth;
		if (bounded(newPos) && !visited[pack(newPos, size)])
		{
			pos = newPos;
			continue;
		}

		newPos = pos + dir;
		if (!visited[pack(newPos, size)])
		{
			pos = newPos;
			continue;
		}
	}

	return path;
}

static std::deque<uint32_t> evenHamilton(point_t start, uint32_t size)
{
	point_t pos = start;
	std::deque<uint32_t> path = { pack(start, size) };

	do
	{
		if(pos.x == 0 && pos.y < size - 1)
		{
			pos.y += 1;
			path.push_back(pack(pos, size));
			continue;
		}

		if (pos.y == size - 1)
		{
			if (pos.x < size - 1)
				pos.x += 1;
			else
				pos.y -= 1;

			path.push_back(pack(pos, size));
			continue;
		}

		if (pos.x % 2)
		{
			if (pos.y == 0)
				pos.x -= 1;
			else
				pos.y -= 1;

			path.push_back(pack(pos, size));
		}
		else
		{
			if (pos.y == size - 2)
				pos.x -= 1;
			else
				pos.y += 1;

			path.push_back(pack(pos, size));
		}

	} while (pos != start);

	return path;
}

static std::deque<uint32_t> findHamilton(map_t &map)
{
	point_t start = unpack(map.robot.position, map.size);

	if (map.size % 2)
		return oddHamilton(start, map.size);
	else
		return evenHamilton(start, map.size);
}

static std::vector<point_t> BFS(map_t &map, point_t start, point_t goal, std::pair<point_t, point_t> skip)
{
	std::queue<point_t> queue;
	queue.push(start);
	
	std::vector<bool> visited = std::vector<bool>(map.size * map.size);
	visited[pack(start, map.size)] = true;

	std::map<point_t, point_t> parent;

	while(!queue.empty())
	{
		point_t u = queue.front();
		queue.pop();
		if (u == goal)
			break;

		point_t dir = { 1, 0 };
		for (int i = 0; i < 4; i++, dir.rotate90())
		{
			point_t v = u + dir;
			uint32_t nodeV = pack(v, map.size);
			
			if (v.x < 0 || v.x >= map.size || v.y < 0 || v.y >= map.size) continue;
			if (std::make_pair(u, v) == skip) continue;
			if (visited[nodeV]) continue;
			if (v != goal && map.batteries.find(nodeV) != map.batteries.end()) continue; 
		
			visited[nodeV] = true;
			parent[v] = u;
			queue.push(v);
		}
	}

	std::vector<point_t> path;

	if (parent.find(goal) == parent.end())
		return path;

	point_t u = goal;
	while(u != start)
	{
		path.push_back(u);
		u = parent[u];
	}
	path.push_back(start);
	
	return path;
}

static path_t* findPath(map_t &map, point_t start, point_t goal)
{
	std::vector<point_t> shortest = BFS(map, start, goal, std::make_pair(point_t(), point_t()));

	path_t *path = new path_t { shortest.size() >= 2, shortest, (uint32_t)shortest.size() };

	if (!path->available)
		return path;
	else if (shortest.size() > 3)
	{
		path->extendable = true;
		path->extended = path->shortest;
		path->extendedLength = path->length;
		return path;
	}

	std::pair<point_t, point_t> illegal = std::make_pair(shortest[1], shortest[0]);
	std::vector<point_t> extended = BFS(map, start, goal, illegal);

	if (extended.size() < 2)
		return path;

	path->extendable = true;
	path->extended = extended;
	path->extendedLength = extended.size();

	return path;
}

static void parseGraph(map_t &map)
{
	std::map<uint32_t, uint32_t>::iterator u, v;
	for (u = map.batteries.begin(); u != map.batteries.end(); ++u)
	{
		map.adjacency[u->first] = std::map<uint32_t, path_t*>();
		for (v = map.batteries.begin(); v != map.batteries.end(); ++v)
		{
			if (map.adjacency.find(v->first) == map.adjacency.end())
				map.adjacency[v->first] = std::map<uint32_t, path_t*>();

			path_t *path = findPath(map, unpack(u->first, map.size), unpack(v->first, map.size));
			map.adjacency[u->first][v->first] = path;
		}

		path_t *path = findPath(map, unpack(u->first, map.size), unpack(map.robot.position, map.size));
		map.adjacency[u->first][map.robot.position] = path;
	}
}

static std::vector<state_t> solve(map_t map)
{
	std::stack<state_t> stateStack;
	uint32_t rotations = 0;

	while (std::any_of(map.batteries.begin(), map.batteries.end(), [](std::pair<uint32_t, uint32_t> battery){ return battery.second > 0; }))
	{
		if (rotations >= map.hamiltonPath.size())
			if (stateStack.empty())
				break;

		path_t *path = map.adjacency[map.hamiltonPath[0]][map.robot.position];
		if (path->available)
		{
			if (path->length - 1 <= map.robot.charge)
			{
				stateStack.push({ rotations, path->length - 1, map.robot.position });

				uint32_t robotCharge = map.robot.charge - path->length + 1;
				map.robot.charge = map.batteries[map.hamiltonPath[0]];
				map.batteries[map.hamiltonPath[0]] = robotCharge;

				map.robot.position = map.hamiltonPath[0];

				rotations = 0;
				
				uint32_t front = map.hamiltonPath.front();
				map.hamiltonPath.pop_front();
				map.hamiltonPath.push_back(front);

				continue;
			}
		}

		rotations++;
		uint32_t front = map.hamiltonPath.front();
		map.hamiltonPath.pop_front();
		map.hamiltonPath.push_back(front);

		if (rotations == map.hamiltonPath.size())
		{
			if (stateStack.empty())
				break;

			state_t state = stateStack.top();
			stateStack.pop();

			uint32_t robotCharge = map.batteries[map.robot.position] + state.length;
			map.batteries[map.robot.position] = map.robot.charge;
			map.robot.charge = robotCharge;

			map.robot.position = state.position;
			rotations = state.rotation + 1;
		}
	}

	stateStack.push({ 0, 0, map.robot.position });

	std::vector<state_t> states;
	while(!stateStack.empty())
	{
		states.insert(states.begin(), stateStack.top());
		stateStack.pop();
	}

	return states;
}

static map_t readFile(const char *path)
{
	std::ifstream file(path);
	map_t map;
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

		std::string line = readLine(file);
		std::sregex_iterator it(line.begin(), line.end(), r);
		int x = std::stoi((*it)[0]) - 1;
		int y = std::stoi((*(++it))[0]) - 1;
		uint32_t c = std::stoi((*(++it))[0]);
		map.robot = { pack({ x, y }, map.size), c };

		size_t count = std::stoull(readLine(file));

		while(count--)
		{
			std::string line = readLine(file);
			std::sregex_iterator it(line.begin(), line.end(), r);
			int x = std::stoi((*it)[0]) - 1;
			int y = std::stoi((*(++it))[0]) - 1;
			uint32_t c = std::stoi((*(++it))[0]);
			map.batteries.insert({ pack({ x, y }, map.size), c });
		}
	}
	else
	{
		std::cerr << "Failed to open file " << path << "!" << std::endl;
		exit(-1);
	}

	return map;
}

/*************************************************
 *     Oeffentliche Methodenimplementationen     *
 *************************************************/

std::vector<point_t> solve(const char* path)
{
	map_t map = readFile(path);

	std::deque<uint32_t> hamilton = findHamilton(map);
	for(uint32_t i = 0; i < map.size * map.size; i++)
		if (map.batteries.find(i) == map.batteries.end())
			hamilton.erase(std::find(hamilton.begin(), hamilton.end(), i));

	map.hamiltonPath = hamilton;

	parseGraph(map);

	auto states = solve(map);

	return std::vector<point_t>();
}
