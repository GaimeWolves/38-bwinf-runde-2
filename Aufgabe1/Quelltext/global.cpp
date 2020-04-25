#include "global.hpp"

#include <queue>

uint32_t encode(point_t p, uint32_t size)
{
	return p.x + p.y * size;
}

point_t decode(uint32_t n, uint32_t size)
{
	return { (int)(n % size), (int)(n / size) };
}

void parseGraph(map_t &map)
{
	map.adjacency = adjacency_t();
	std::map<uint32_t, uint32_t>::iterator u, v;
	
	point_t robot = decode(map.robot.position, map.size);

	for (u = map.batteries.begin(); u != map.batteries.end(); ++u)
	{
		point_t pU = decode(u->first, map.size);
		map.adjacency[u->first] = std::map<uint32_t, std::shared_ptr<path_t>>();
		
		for (v = map.batteries.begin(); v != map.batteries.end(); ++v)
		{
			point_t pV = decode(v->first, map.size);
			
			if (map.adjacency.find(v->first) == map.adjacency.end())
				map.adjacency[v->first] = std::map<uint32_t, std::shared_ptr<path_t>>();

			auto path = findPath(pU, pV, map);
			map.adjacency[u->first][v->first] = path;
			map.adjacency[v->first][u->first] = path;
		}

		auto path = findPath(pU, robot, map);
		map.adjacency[u->first][map.robot.position] = path;
		map.adjacency[map.robot.position][u->first] = path;
	}

	auto path = findPath(robot, robot, map);
	map.adjacency[map.robot.position][map.robot.position] = path;
}

std::shared_ptr<path_t> findPath(point_t start, point_t end, map_t &map)
{	
	std::vector<point_t> shortest;
	if (start == end) // Pfad zur selben Batterie finden
	{
		// Nachbarpunkte ueberprufen
		point_t dir = { 0, 1 };
		for (int i = 0; i < 4; i++, dir.rotate90())
		{
			point_t pos = start + dir;
			if (pos.x < 0 || pos.x >= map.size || pos.y < 0 || pos.y >= map.size)
				continue;

			if (map.batteries.find(encode(start + dir, map.size)) == map.batteries.end())
			{
				shortest = { start, start + dir, start };
				break;
			}
		}
	}
	else
		shortest = BFS(start, end, map, false);

	std::shared_ptr<path_t> path = std::make_shared<path_t>();

	path->available = shortest.size() >= 2;
	path->shortest = shortest;
	path->length = shortest.size() - 1;

	if (!path->available)
		return path;
	else if (path->length > 2)
	{
		path->extendable = true;
		path->extended = path->shortest;
		path->extendedLength = path->length;
		return path;
	}

	std::vector<point_t> extended;
	if (start == end)
	{
		point_t dir = { 0, 1 };
		for (int i = 0; i < 4; i++, dir.rotate90())
		{
			if (extended.size() > 0)
				break;

			point_t middle = start + dir;
			if (middle.x < 0 || middle.x >= map.size || middle.y < 0 || middle.y >= map.size)
				continue;

			if (map.batteries.find(encode(middle, map.size)) != map.batteries.end())
				continue;

			for (int j = 0; j < 4; j++, dir.rotate90())
			{
				point_t last = middle + dir;
				if (last.x < 0 || last.x >= map.size || last.y < 0 || last.y >= map.size)
					continue;

				if (map.batteries.find(encode(last, map.size)) == map.batteries.end())
				{
					extended = { start, middle, last, middle, start };
					break;
				}
			}
		}
	}
	else
		extended = BFS(start, end, map, true);

	if (extended.size() < 2)
		return path;

	path->extendable = true;
	path->extended = extended;
	path->extendedLength = extended.size() - 1;

	return path;
}

std::vector<point_t> BFS(point_t start, point_t goal, map_t &map, bool extendable)
{
	std::queue<point_t> queue;
	queue.push(start);
	
	std::vector<bool> visited = std::vector<bool>(map.size * map.size);
	visited[encode(start, map.size)] = true;

	std::map<point_t, point_t> parent;

	// Hoechstens zwei zu ignoriererende Kanten
	std::pair<point_t, point_t> skip1 = { };
	std::pair<point_t, point_t> skip2 = { };

	if (extendable)
	{
		// Raute ueberprufen
		point_t parallel = { 0, 1 };
		point_t diagonal = { 1, 1 };

		for (int i = 0; i < 4; i++, parallel.rotate90(), diagonal.rotate90())
		{
			if (start + parallel == goal)
			{
				skip1 = { start, goal };
				break;
			}
			else if (start + parallel * 2 == goal)
			{
				skip1 = { start + parallel, goal };
				break;
			}
			else if (start + diagonal == goal)
			{
				skip1 = { start + point_t{ diagonal.x, 0 }, goal };
				skip2 = { start + point_t{ 0, diagonal.y }, goal };
				break;
			}
		}
	}

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
			uint32_t nodeV = encode(v, map.size);
			
			std::pair<point_t, point_t> edge = { u, v };
			if (edge == skip1 || edge == skip2) continue;
			
			if (v.x < 0 || v.x >= map.size || v.y < 0 || v.y >= map.size) continue;
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
