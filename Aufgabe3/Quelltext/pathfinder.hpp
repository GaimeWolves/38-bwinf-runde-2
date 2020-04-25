#include <vector>
#include <map>
#include <set>

/*
 * Struct Definitionen für die jeweiligen Methoden
 */

// Speichert einen Punkt für die Lookup Table
// und Kurven-/Distanzberechnung
struct Point
{
	int x;
	int y;

	Point(int x, int y);
	Point();

	bool operator==(const Point &b) const;
};

// Speichert von einem Knoten den Vorgaenger und die Gesamtdistanz
// fuer die Dijkstra Implementation
struct Predecessor
{
	uint32_t predecessor; // Vorgaenger
	float distance;       // Gesamtdistanz

	Predecessor(uint32_t predecessor, float distance);
	Predecessor();
};

// Stellt einen "Arbeiter" in der "BFS" Implementation
struct Worker
{
	uint32_t position;          // Derzeitige Position
	float distance;             // Gesamtdistanz
	int turns;                  // Kurvenanzahl
	std::vector<uint32_t> path; // Gegangener Pfad

	Worker(uint32_t start);
	Worker(uint32_t pos, float dist, int turns, std::vector<uint32_t> path);
	Worker();

	// Sortiert zuerst nach Kurvenanzahl, dann nach Distanz
	bool operator>(const Worker &rhs) const;
};


/*
 * Globale Variablen
 */

// Stellt eine Lookup Table dar, die
// allen Punkten im Graph eine Nummer zuordnet
extern std::vector<Point> lookup;

// Speichert den Graphen in einer Adjazenzliste ab
extern std::map<uint32_t, std::set<uint32_t>> adjList;

// Nummern des Start- und Endknoten
extern uint32_t start, end;

// Maximale Verlaengerung in Prozent (z.B. 130% = 1,30)
extern float maxPercentage;


/*
 * Methodendefinitionen
 */

// Zeigt die Adjazenzliste und die Start- und Endknoten auf der Konsole an
void printNodes();

// Berechnet für alle Knoten den kürzesten Pfad zum Startknoten
std::vector<Predecessor> findShortestPath();

// Berechnet die angegebene Anzahl an Pfaden mit den wenigsten Kurven
std::vector<Worker> findFewestTurnPaths(std::vector<Predecessor> &predecessor, size_t pathCap);

// Liest die Eingabedatei aus
void parseFile(char* filePath);