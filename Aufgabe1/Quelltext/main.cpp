#include "solver.hpp"
#include "generator.hpp" 

#include <iostream>
#include <string>
#include <getopt.h>
#include <random>

enum Mode {
	Generate, Solve
};

struct flags_t
{
	Mode mode;
	char* file;
	bool debug;

	// Generator options
	constraint_t constraints;

	bool size;

	bool minDiff;
	bool maxDiff;

	bool minDensity;
	bool maxDensity;

	bool minLength;
	bool maxLength;
} flags;

static struct option longOptions[] =
{
	{ "debug",          no_argument,       NULL, 'i' },
	{ "size",           required_argument, NULL, 's' },
	{ "min-difficulty", required_argument, NULL, 'd' },
	{ "max-difficulty", required_argument, NULL, 'D' },
	{ "min-length",     required_argument, NULL, 'l' },
	{ "max-length",     required_argument, NULL, 'L' },
	{ "min-density",    required_argument, NULL, 'b' },
	{ "max-density",    required_argument, NULL, 'B' },
	{ NULL, 0, NULL, 0 }
};

static std::string shortOptions = "is:d:D:l:L:b:B:";

int main(int argc, char *argv[])
{
	// Random seeden
	std::srand(time(NULL));
	for (int i = 0; i < 100; i++)
		std::rand();

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <Modus> <Datei>" << std::endl;
		exit(-1);
	}

	std::string modeString = std::string(argv[optind++]);

	if (modeString == "generate")
		flags.mode = Generate;
	else if (modeString == "solve")
		flags.mode = Solve;
	else
	{
		std::cerr << "Unbekannter Modus: " << modeString << "\n"
				  << "Benutze entweder 'generate' um Spielsituationen zu generieren,\n"
				  << "            oder 'solve'    um Spielsituationen zu lösen." << std::endl;
		exit(-1);
	}

	flags.file = argv[optind++];

	char opt;
	while((opt = getopt_long(argc, argv, shortOptions.c_str(), longOptions, NULL)) != -1)
	{
		switch(opt)
		{
			case 'i':
				flags.debug = true;
				break;
			case 's':
				flags.constraints.size = std::stoul(argv[optind]);
				flags.size = true;
				break;
			case 'd':
				flags.constraints.minDI = std::stod(argv[optind]);
				flags.minDiff = true;
				break;
			case 'D':
				flags.constraints.maxDI = std::stod(argv[optind]);
				flags.maxDiff = true;
				break;
			case 'b':
				flags.constraints.minDensity = std::stod(argv[optind]);
				flags.minDensity = true;
				break;
			case 'B':
				flags.constraints.maxDensity = std::stod(argv[optind]);
				flags.maxDensity = true;
				break;
			case 'l':
				flags.constraints.minLength = std::stod(argv[optind]);
				flags.minLength = true;
				break;
			case 'L':
				flags.constraints.maxLength = std::stod(argv[optind]);
				flags.maxLength = true;
				break;
			default:
				std::cerr << "Unbekannte Option: " << argv[optind] << std::endl;
				exit(-1);
				break;
		}
	}

	if (flags.mode == Solve && argc > optind)
	{
		std::cerr << "Zu viele Argumente: " << argv[optind] << std::endl;
		std::cerr << "Usage: " << argv[0] << " solve <Datei>" << std::endl;
		exit(-1);
	}

	if (!flags.size && argc > optind)
		flags.constraints.size = std::stoull(argv[optind++]);
	
	if (!flags.minDiff && argc > optind)
		flags.constraints.minDI = std::stod(argv[optind++]);

	if (!flags.maxDiff && argc > optind)
		flags.constraints.maxDI = std::stod(argv[optind++]);

	if (!flags.minDensity && argc > optind)
		flags.constraints.minDensity = std::stod(argv[optind++]);

	if (!flags.maxDensity && argc > optind)
		flags.constraints.maxDensity = std::stod(argv[optind++]);

	if (!flags.minLength && argc > optind)
		flags.constraints.minLength = std::stod(argv[optind++]);

	if (!flags.maxLength && argc > optind)
		flags.constraints.maxLength = std::stod(argv[optind++]);

	if (flags.mode == Generate)
	{
		difficulty_t difficulty = solveConstraints(flags.constraints, flags.debug);
	
		std::cout << "Eigenschaften der Spielsituation\n"
				  << "--------------------------------\n"
				  << "Schwierigkeitsgrad: " << difficulty.di << '\n'
				  << "Batteriedichte    : " << difficulty.density << '\n'
				  << "Lösungslänge      : " << difficulty.length
				  << std::endl;

		map_t map = generateConfig(difficulty);

		std::cout << "Anzahl Batterien  : " << map.batteries.size() << '\n' << std::endl;

		writeToFile(map, flags.file);

		std::cout << "Datei wurde erfolgreich erstellt: " << flags.file << std::endl;
	}
	else
	{
		std::vector<point_t> path = solveGame(flags.file, flags.debug);
		
		if (path.size() == 0)
			std::cout << "Die Spielsituation ist unlösbar!" << std::endl;
		else
		{
			std::cout << "Lösungsweg:" << std::endl;
			for (auto point : path)
				std::cout << "(" << point.x + 1 << "|" << point.y + 1 << ") ";
			std::cout << std::endl;
		}
	}

	return 0;
}
