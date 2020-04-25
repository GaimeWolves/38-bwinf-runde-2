#include <vector>
#include <iostream>
#include <algorithm>
#include <getopt.h>

#include "pathfinder.hpp"

struct ProgramFlags
{
	int debug;
	int input;
	int percentage;
	int pathCount;
	int noprint;
} flags;

static struct option longOptions[] =
{
	{ "debug",      no_argument,       NULL, 'd' },
	{ "input",      required_argument, NULL, 'i' },
	{ "percentage", required_argument, NULL, 'p' },
	{ "count",      required_argument, NULL, 'c' },
	{ "no-print",   no_argument,       NULL, 'n' },
	{ NULL, 0, NULL, 0 }
};

static void printUsage(char** argv)
{
	std::cerr << "Usage: " << argv[0] << " <Eingabedatei> <Verlängerung>" << std::endl;
	exit(-1);
}

int main(int argc, char* argv[])
{
	/*
	 * Kommandozeilenargumente parsen
	 */
	if (argc < 3)
		printUsage(argv);

	char* filePath;
	size_t pathCap = 1;

	char c;
	while ((c = getopt_long(argc, argv, "di:p:c:n", longOptions, NULL)) != -1)
	{
		switch (c)
		{
			case 'i':
				filePath = optarg;
				flags.input = 1;
				break;
			case 'p':
				maxPercentage = std::stof(optarg) / 100.f + 1;
				flags.percentage = 1;
				break;
			case 'c':
				pathCap = std::stoll(optarg);
				flags.pathCount = 1;
				break;
			case 'd':
				flags.debug = 1;
				break;
			case 'n':
				flags.noprint = 1;
				break;

			default:
				printUsage(argv);
				break;
		}
	}

	if (!flags.input)
	{
		if (argc <= optind + 1)
			printUsage(argv);

		// Eingabedatei einlesen
		filePath = argv[optind++];
	}

	if (!flags.percentage)
	{
		if (argc <= optind)
			printUsage(argv);

		// Eingegebene Verlaengerung in Multiplikationsfaktor umrechnen
		maxPercentage = std::stof(argv[optind++]) / 100.f + 1;
	}

	// Datei parsen
	parseFile(filePath);

	// Vorgaengerkarte berechnen
	auto predecessor = findShortestPath();

	// Die gewuenschte Anzahl an Pfaden berechnen
	auto paths = findFewestTurnPaths(predecessor, pathCap);

	if (flags.noprint)
		return 0;

	// Adjazenzliste und kürzester Pfad ausgeben zu Debugzwecken
	if (flags.debug)
	{
		std::cout << filePath << std::endl;

		printNodes();

		std::cout << "Vorgängerkarte: " << std::endl;
		for (int node = 0; node < lookup.size(); node++)
		{
			Predecessor pred = predecessor[node];
			if (pred.predecessor == -1)
				std::cout << "(" << lookup[node].x << "," << lookup[node].y << "): Vorgänger: -1 Distanz zum Start: inf" << std::endl;
			else
			{
				std::cout
					<< "(" << lookup[node].x << "," << lookup[node].y << ") Vorgänger: "
					<< "(" << lookup[pred.predecessor].x << "," << lookup[pred.predecessor].y << ") Distanz zum Start: "
					<< pred.distance << std::endl;
			}
		}
		std::cout << std::endl;
	
		std::vector<uint32_t> path;
		path.push_back(end);
		uint32_t current = end;
		do
		{
			current = predecessor[current].predecessor;
			path.push_back(current);
		} while(current != start);
		std::reverse(path.begin(), path.end());
		
		std::cout << "\nShortest path:" << std::endl;
		for (auto node : path)
			std::cout << "(" << lookup[node].x << "," << lookup[node].y << ") ";
		std::cout << std::endl;
	}

	// Konsolenausgabe
	// Optimaler Pfad ist der erste in der Liste
	// Alle Pfade werden vom Ziel aus berechnet also rückwärts durchiterieren
	// [mit rbegin() und rend()]
	std::cout << "\nPfad mit den wenigsten Kurven:" << std::endl;
	for (auto node = paths[0].path.rbegin(); node != paths[0].path.rend(); ++node)
		std::cout << "(" << lookup[*node].x << "," << lookup[*node].y << ") ";
	std::cout << std::endl;

	Predecessor endPred = predecessor[end];
	float factor = paths[0].distance / endPred.distance;
	std::cout << "Kurven (bester Pfad): " << paths[0].turns << std::endl;
	std::cout << "Länge (bester Pfad) : " << paths[0].distance << std::endl;
	std::cout << "Länge (kürest. Pfad): " << endPred.distance << std::endl;
	std::cout << "Verlängerung        : " << factor << " < " << maxPercentage << std::endl;

	if (pathCap > 1)
	{
		std::cout << "\nAlternative Pfade:" << std::endl;

		// Die restlichen Pfade auf gleicher Weise ausgeben
		for (size_t i = 1; i < pathCap && i < paths.size(); i++)
		{
			for (auto node = paths[i].path.rbegin(); node != paths[i].path.rend(); ++node)
				std::cout << "(" << lookup[*node].x << "," << lookup[*node].y << ") ";
			std::cout << std::endl;
		
			float factor = paths[i].distance / endPred.distance;
			std::cout << "Kurven : " << paths[i].turns << std::endl;
			std::cout << "Länge  : " << paths[i].distance << std::endl;
			std::cout << "Faktor : " << factor << " < " << maxPercentage << "\n" << std::endl;
		}
	}
}