#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>

using namespace std;

#define DEBUG 1

#define LEN_SEQUENCE 8
#define MAX_N_GENERATE 1000
#define N_LAST_POSITIONS 5

#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

struct Point {
	int x;
	int y;
};
struct Pellet {
	int x;
	int y;
	int value;
};
struct BigPelletRushInfo {
	bool isRushing = false;
	Pellet bigPellet;
};
struct PacMan {
	int pacId; // pac number (unique within a team)
	bool mine; // true if this pac is yours
	int x; // position in the grid
	int y; // position in the grid
	string typeId; // unused in wood leagues
	int speedTurnsLeft; // unused in wood leagues
	int abilityCooldown; // unused in wood leagues
	bool isSpeeding = false;
	bool isMoving = false;
	vector<Point> lastPositions;
	BigPelletRushInfo bigPelletRushInfo;
};
struct Gene {
	int direction; // DIR_UP DIR_DOWN DIR_LEFT DIR_RIGHT
};
struct ScoreVars
{
	float opponentSeen = 0;
	int pelletScore = 0;
	float distToBigPellet = 0;
	int unVisitedFrames = 0;
	float distanceToNearestTeamate = 0;
	int lastUnCommonPositions = 0;
};
struct FitScore {
	float opponentSeenScore = 0;
	float pelletScore = 0;
	float distToBigPelletScore = 0;
	float teamateDistanceScore = 0;
	float unVisitedScore = 0;
	float pathMemoryScore = 0;
	float total = 0;
};
struct SequenceEvaluation {
	ScoreVars scoreVars;
	PacMan pac;
	vector<Pellet> bigPellets;
	FitScore fitScore;
};
struct Sequence {
	vector<Gene> genes;
	SequenceEvaluation evaluation;
	vector<Point> positionsSequence;
};


// ================ ALGO FUNCTIONS ================
Sequence initRandomSequence();
void evalSequence(Sequence& sequence, vector<string> map_, const vector<Point> visitedFrames, vector<PacMan> myPacs, int mapWidth, int mapHeight);
void sortSequences(vector<Sequence>& sequences);

// ================ ALGO UTILS ================
float dist2Points(float x1, float y1, float x2, float y2);
float distToNearestBigPellet(float x, float y, vector<Pellet> bigPellets);
FitScore computeFitScore(ScoreVars scoreVars);
float distToNearestPac(int x, int y, vector<PacMan> pacs);

// ================ EXTRA FUNCTIONS ================
void setPacToMap(vector<string>& map, PacMan pac);
void updateDataLoop(vector<string>& map, int mapWidth, vector<PacMan>& myPacs, vector<PacMan>& teamates, vector<PacMan>& opponentPacs, vector<Pellet>& pellets, vector<Pellet>& bigPellets);
void setVisitedToMap(const PacMan pac, vector<string>& map, int mapWidth);
int pacIndexInList(PacMan pac, vector<PacMan> pacs);
void setRushingToBigPelletPacs(vector<PacMan>& myPacs, const vector<Pellet>& bigPellets);

// ================ DEBUG FUNCTIONS ================
void printSequence(const Sequence sequence);
void printMap(const vector<string> map);
void printMapPacs(vector<string> map, vector<PacMan> pacs);
void printScoreVars(ScoreVars scoreVars);
void printFitScore(FitScore fitScore);
void printMapExtra(vector<string> map, vector<PacMan> pacs, vector<Point> visited);



int main()
{
	int width; // size of the grid
	int height; // top left corner is (x=0, y=0)
	cin >> width >> height; cin.ignore();
	//cerr << "width height: " << width << ' ' << height << endl;
	//cerr << "{ ";
	vector<string> startMap(height);
	for (int i = 0; i < height; i++) {
		string row;
		getline(cin, row); // one line of the grid: space " " is floor, pound "#" is wall
		startMap[i] = row;
		//cerr << ", \"" << row << "\"";
	}
	//cerr << "};";
	vector<Point> visitedFrames;
	vector<string> map = startMap;
	vector<PacMan> myPacs;
	vector<PacMan> teamates;
	vector<PacMan> opponentPacs;
	vector<Pellet> pellets;
	vector<Pellet> bigPellets;

	srand(time(NULL));
	// game loop
	while (1) {

		int myScore;
		int opponentScore;
		cin >> myScore >> opponentScore; cin.ignore();
		updateDataLoop(map, width, myPacs, teamates, opponentPacs, pellets, bigPellets);
		if (DEBUG) {
			printMap(map);
		}
		setRushingToBigPelletPacs(myPacs, bigPellets);
		for (int iPac = 0; iPac < myPacs.size(); iPac++) {
			if (myPacs[iPac].bigPelletRushInfo.isRushing) {
				// ================ MANAGE SPEED ================
				if (myPacs[iPac].speedTurnsLeft == 0) {
					myPacs[iPac].isSpeeding = false;
				}
				if (!myPacs[iPac].isSpeeding && myPacs[iPac].abilityCooldown == 0) {
					cout << " SPEED " << myPacs[iPac].pacId << " | ";
					myPacs[iPac].isSpeeding = true;
				}
				else {
					cout << "MOVE " << myPacs[iPac].pacId << " " << myPacs[iPac].bigPelletRushInfo.bigPellet.x << " " << myPacs[iPac].bigPelletRushInfo.bigPellet.y << " | "; // MOVE <pacId> <x> <y>
				}
			}
			else {
				vector<Sequence> sequences(MAX_N_GENERATE / myPacs.size());
				for (int i = 0; i < MAX_N_GENERATE / myPacs.size(); i++) {
					sequences[i] = initRandomSequence();
					sequences[i].evaluation.bigPellets = bigPellets;
					/*
					cerr << "sequence: ";
					printSequence(sequences[i]);
					cerr << "pelletScore: " << sequences[i].evaluation.scoreVars.pelletScore << endl;
					*/
					sequences[i].evaluation.pac = myPacs[iPac];

					evalSequence(sequences[i], map, visitedFrames, teamates, width, height);
				}
				cerr << endl;
				sortSequences(sequences);
				if (DEBUG) {
					for (int i = 0; i < 1; i++) {
						cerr << visitedFrames.size() << endl;
						cerr << "sequence: ";
						printSequence(sequences[i]);
						cerr << "_______ScoreVars_______" << endl;
						printScoreVars(sequences[i].evaluation.scoreVars);
						cerr << "_______FitScore_______" << endl;
						printFitScore(sequences[i].evaluation.fitScore);
						cerr << "lastPositions: " << endl;
						for (int k = 0; k < sequences[i].evaluation.pac.lastPositions.size(); k++) {
							cerr << sequences[i].evaluation.pac.lastPositions[k].x << " " << sequences[i].evaluation.pac.lastPositions[k].y << " | ";
						}
						cerr << endl;
						cerr << "positionsSequence: " << endl;
						for (int k = 0; k < sequences[i].positionsSequence.size(); k++) {
							cerr << sequences[i].positionsSequence[k].x << " " << sequences[i].positionsSequence[k].y << " | ";
						}
						cerr << endl;
						cerr << "pacxy " << myPacs[iPac].x << " " << myPacs[iPac].y << endl;
						vector<PacMan> pacs = { myPacs[iPac], sequences[i].evaluation.pac };
						printMapExtra(map, pacs, visitedFrames);
						cerr << endl;
					}
				}
				// ================ MANAGE SWITCH ================

				bool mustSwitch = false;
				if (myPacs[iPac].typeId != sequences[0].evaluation.pac.typeId) {
					mustSwitch = true;
				}
				myPacs[iPac] = sequences[0].evaluation.pac;

				bool mustSpeed = myPacs[iPac].speedTurnsLeft == 0 && sequences[0].evaluation.scoreVars.pelletScore>4;


                myPacs[iPac].isSpeeding = true;
                if (myPacs[iPac].speedTurnsLeft > 0){
				    myPacs[iPac].isSpeeding = false;
                }
				// ================ MANAGE ACTIONS ================

				/*if (myPacs[iPac].isMoving) {
					myPacs[iPac].isMoving = false;
				}*/
				//cerr << "pac: " << myPacs[iPac].pacId << " speedTurnsLeft " << myPacs[iPac].speedTurnsLeft << endl;
				//cerr << "pac: " << myPacs[iPac].pacId << " abilityCooldown " << myPacs[iPac].abilityCooldown << endl;
				if (mustSwitch && myPacs[iPac].abilityCooldown == 0) {
					cout << "SWITCH " << myPacs[iPac].pacId << " " << myPacs[iPac].typeId << " | ";
				}
				else if (mustSpeed && myPacs[iPac].abilityCooldown == 0) {
					cout << " SPEED " << myPacs[iPac].pacId << " | ";
					myPacs[iPac].isSpeeding = true;
					//myPacs[iPac].isMoving = false;
				}
				else {
                    if (myPacs[iPac].isSpeeding){
                        cout << "MOVE " << myPacs[iPac].pacId << " " << sequences[0].positionsSequence[1].x << " " << sequences[0].positionsSequence[1].x << " | "; // MOVE <pacId> <x> <y>
                    }
                    else{
					cout << "MOVE " << myPacs[iPac].pacId << " " << myPacs[iPac].x << " " << myPacs[iPac].y << " | "; // MOVE <pacId> <x> <y>
					//myPacs[iPac].isMoving = true;
                    }
				}

			}
		}
		cout << endl;
	}
}
struct AssociationPacPelletDist {
	int pacId;
	int iPac;
	int ipellet;
	float distance;
};

void setRushingToBigPelletPacs(vector<PacMan>& myPacs, const vector<Pellet>& bigPellets) {
	vector<PacMan> _myPacs = myPacs;
	vector<Pellet> _bigPellets = bigPellets;
	int cnt = 0;
	while (!_myPacs.empty() && !_bigPellets.empty() && cnt < 100) {
		cnt++;
		cerr << cnt << endl;
		vector<AssociationPacPelletDist> minAssos;
		for (int i = 0; i < _myPacs.size(); i++) {
			float minDist = 100;
			int iPac, iPellet;
			for (int j = 0; j < _bigPellets.size(); j++) {
				float dist = dist2Points(bigPellets[j].x, bigPellets[j].y, myPacs[i].x, myPacs[i].y);
				if (dist < minDist) {
					minDist = dist;
					iPac = i;
					iPellet = j;
				}
			}
			AssociationPacPelletDist asso = { _myPacs[iPac].pacId, iPac , iPellet, minDist };
			minAssos.push_back(asso);
		}
		std::sort(minAssos.begin(), minAssos.end(), \
			[](AssociationPacPelletDist asso1, AssociationPacPelletDist asso2)\
		{return asso1.distance < asso2.distance; });

		//retrieve pac by id
		for (int i = 0; i < myPacs.size(); i++) {
			if (minAssos[0].pacId == myPacs[i].pacId) {
				myPacs[i].bigPelletRushInfo.bigPellet = _bigPellets[minAssos[0].ipellet];
				myPacs[i].bigPelletRushInfo.isRushing = true;
				break;
			}
		}

		_myPacs.erase(_myPacs.begin() + minAssos[0].iPac);
		_bigPellets.erase(_bigPellets.begin() + minAssos[0].ipellet);

	}
}




void updateDataLoop(vector<string>& map, int mapWidth, vector<PacMan>& myPacs, vector<PacMan>& teamates, vector<PacMan>& opponentPacs, vector<Pellet>& pellets, vector<Pellet>& bigPellets) {
	int visiblePacCount; // all your pacs and enemy pacs in sight
	cin >> visiblePacCount; cin.ignore();
	//clean opponents
	for (int i = 0; i < opponentPacs.size(); i++) {
		map[opponentPacs[i].y][opponentPacs[i].x] = 'V';
	}
	//clean teamates
	for (int i = 0; i < teamates.size(); i++) {
		map[teamates[i].y][teamates[i].x] = 'V';
	}
	opponentPacs.clear();
	teamates.clear();

	vector<PacMan> myNewPacs;
	for (int i = 0; i < visiblePacCount; i++) {
		PacMan pac;
		cin >> pac.pacId >> pac.mine >> pac.x >> pac.y >> pac.typeId >> pac.speedTurnsLeft >> pac.abilityCooldown; cin.ignore();
		setPacToMap(map, pac);
		pac.bigPelletRushInfo.isRushing = false;
		if (pac.mine) {
			// ================= UPDATE PREV PACS =================
			vector<Point> lastPositions;
			int pacIndex = pacIndexInList(pac, myPacs);
			if (pacIndex != -1) {
				lastPositions = myPacs[pacIndex].lastPositions;
			}
			Point position = { pac.x, pac.y };
			lastPositions.push_back(position);
			if (lastPositions.size() > N_LAST_POSITIONS) {
				lastPositions.erase(lastPositions.begin());
			}
			pac.lastPositions = lastPositions;
			myNewPacs.push_back(pac);
			teamates.push_back(pac);

			setVisitedToMap(pac, map, mapWidth);
		}
		else {
			opponentPacs.push_back(pac);
		}
	}
	myPacs = myNewPacs;
	//clean pellets
	for (int i = 0; i < pellets.size(); i++) {
		if (map[pellets[i].y][pellets[i].x] == '.' || map[pellets[i].y][pellets[i].x] == '*') {
			map[pellets[i].y][pellets[i].x] = ' ';
		}
	}
	int visiblePelletCount; // all pellets in sight
	cin >> visiblePelletCount; cin.ignore();
	pellets.clear();
	bigPellets.clear();
	for (int i = 0; i < visiblePelletCount; i++) {
		Pellet pellet;
		cin >> pellet.x >> pellet.y >> pellet.value; cin.ignore();
		pellets.push_back(pellet);
		//set pellets to map
		if (pellet.value == 1) {
			map[pellet.y][pellet.x] = '.';
		}

		else if (pellet.value == 10) {
			bigPellets.push_back(pellet);
			map[pellet.y][pellet.x] = '*';
		}
	}
}
int pacIndexInList(PacMan pac, vector<PacMan> pacs) {
	for (int i = 0; i < pacs.size(); i++) {
		if (pac.pacId == pacs[i].pacId) {
			return i;
		}
	}
	return -1;
}


void setPacToMap(vector<string>& map, PacMan pac) {
	if (pac.mine) {
		map[pac.y][pac.x] = pac.typeId[0];
	}
	else {
		map[pac.y][pac.x] = pac.typeId[1];
	}
}

void removePelletsMap(vector<PacMan> prevMyPacs, vector<string>& map, int mapWidth) {
	for (int j = 0; j < prevMyPacs.size(); j++) {
		int pacX = prevMyPacs[j].x;
		int pacY = prevMyPacs[j].y;
		for (int x = pacX + 1; x < mapWidth; x++) {
			if (map[pacY][x] == '#') {
				break;
			}
			else if (map[pacY][x] != 'V') {
				map[pacY][x] = ' ';
			}
		}
		for (int x = pacX - 1; x > 0; x--) {
			if (map[pacY][x] == '#') {
				break;
			}
			else if (map[pacY][x] != 'V') {
				map[pacY][x] = ' ';
			}
		}
		for (int y = pacY + 1; y < map.size(); y++) {
			if (map[y][pacX] == '#') {
				break;
			}
			else if (map[y][pacX] != 'V') {
				map[y][pacX] = ' ';
			}
		}
		for (int y = pacY - 1; y > 0; y--) {
			if (map[y][pacX] == '#') {
				break;
			}
			else if (map[y][pacX] != 'V') {
				map[y][pacX] = ' ';
			}
		}
	}
}

void setVisitedToMap(const PacMan pac, vector<string>& map, int mapWidth) {
	int pacX = pac.x;
	int pacY = pac.y;

	for (int x = pacX + 1; x < mapWidth; x++) {
		if (map[pacY][x] == '#') {
			break;
		}
		else if (map[pacY][x] == '#') {
			map[pacY][x] = 'V';
		}
	}
	for (int x = pacX - 1; x > 0; x--) {
		if (map[pacY][x] == '#') {
			break;
		}
		else if (map[pacY][x] != '#') {
			map[pacY][x] = 'V';
		}
	}
	for (int y = pacY + 1; y < map.size(); y++) {
		if (map[y][pacX] == '#') {
			break;
		}
		else if (map[y][pacX] != '#') {
			map[y][pacX] = 'V';
		}
	}
	for (int y = pacY - 1; y > 0; y--) {
		if (map[y][pacX] == '#') {
			break;
		}
		else if (map[y][pacX] != '#') {
			map[y][pacX] = 'V';
		}
	}
}



Sequence initRandomSequence()
{
	Sequence sequence;
	sequence.genes.resize(LEN_SEQUENCE);
	int nDirection = 4;
	for (int i = 0; i < LEN_SEQUENCE; i++) {
		int direction = rand() / float(RAND_MAX) * nDirection;
		Gene gene;
		gene.direction = direction;
		sequence.genes[i] = gene;
	}
	return sequence;
}


void evalSequence(Sequence& sequence, vector<string> map, const vector<Point> visitedFrames, vector<PacMan> myPacs, int mapWidth, int mapHeight)
{
	sequence.evaluation.scoreVars = { 0 };
	sequence.evaluation.scoreVars.distToBigPellet = 100;
	int pacX = sequence.evaluation.pac.x;
	int pacY = sequence.evaluation.pac.y;
	Point pacPos;
	vector<Point> positionsSequence;

	// ============ SIMUL MOVEMENTS ============
	for (int i = 0; i < sequence.genes.size(); i++) {
		switch (sequence.genes[i].direction)
		{
		case DIR_UP:
			pacY--;
			break;
		case DIR_DOWN:
			pacY++;
			break;
		case DIR_LEFT:
			pacX--;
			break;
		case DIR_RIGHT:
			pacX++;
			break;
		default:
			cerr << "error gene direction " << sequence.genes[i].direction << endl;
			break;
		}

		// ============ MANAGE BORDERS ============
		if (pacY < 0 || pacY >= mapHeight) {
			cerr << pacX << ' ' << pacY << " Out of Map." << endl;
			return;
		}
		if (pacX == -1) {
			pacX = mapWidth - 1;
		}
		else if (pacX == mapWidth) {
			pacX = 0;
		}


		bool isInWall = false;
		// ============ MAP SCORE ============
		switch (map[pacY][pacX])
		{
		case '#':
			isInWall = true;
			//cerr << pacX << ' ' << pacY << " In a wall." << endl;
			break;
		case '.':
			if (sequence.positionsSequence.empty()) {
				sequence.evaluation.scoreVars.pelletScore += 2;
			}
			sequence.evaluation.scoreVars.pelletScore++;
			map[pacY][pacX] = 'V';
			break;
		case '*':
			sequence.evaluation.scoreVars.pelletScore += 10;
			map[pacY][pacX] = 'V';
			break;
		case ' ':
			sequence.evaluation.scoreVars.unVisitedFrames++;
			break;
		case 'V':
			break;
		case 'A':
			if (sequence.evaluation.pac.typeId != "SCISSORS") {
				if (sequence.positionsSequence.size() < 5) {
					if (sequence.evaluation.pac.abilityCooldown == 0) {
						sequence.evaluation.pac.typeId = "SCISSORS";
					}
					else {
						sequence.evaluation.scoreVars.opponentSeen += 1;
					}
				}
			}
			break;
		case 'O':
			if (sequence.evaluation.pac.typeId != "PAPER") {
				if (sequence.positionsSequence.size() < 5) {
					if (sequence.evaluation.pac.abilityCooldown == 0) {
						sequence.evaluation.pac.typeId = "PAPER";
					}
					else {
						sequence.evaluation.scoreVars.opponentSeen += 1;
					}
				}
			}
			break;
		case 'C':
			if (sequence.evaluation.pac.typeId != "ROCK") {
				if (sequence.positionsSequence.size() < 5) {
					if (sequence.evaluation.pac.abilityCooldown == 0) {
						sequence.evaluation.pac.typeId = "ROCK";
					}
					else {
						sequence.evaluation.scoreVars.opponentSeen += 1;
					}
				}
			}
			break;
		default:
			//cerr << "Wrong map data" << endl;
			break;
		}
		// ============ PATH MEMORY SCORE ============

		if (!isInWall) {
			sequence.positionsSequence.push_back({ pacX, pacY });
			vector<Point> lastPositions = sequence.evaluation.pac.lastPositions;
			bool isIn = false;
			for (int k = 0; k < lastPositions.size(); k++) {
				if (lastPositions[k].x == pacX && lastPositions[k].y == pacY) {
					isIn = true;
					break;
				}
			}
			if (isIn) {
				sequence.evaluation.scoreVars.lastUnCommonPositions--;
			}
			else if (!isIn) {
				sequence.evaluation.scoreVars.lastUnCommonPositions++;
			}
			if (sequence.positionsSequence[0].x == 0 || sequence.positionsSequence[0].x == mapWidth - 1) {
				sequence.evaluation.pac.x = sequence.positionsSequence.back().x;
				sequence.evaluation.pac.y = sequence.positionsSequence.back().y;
			}
			else { //if (sequence.positionsSequence.size() == 1) {
				sequence.evaluation.pac.x = sequence.positionsSequence[0].x;
				sequence.evaluation.pac.y = sequence.positionsSequence[0].y;
			}
			/*else if (sequence.positionsSequence.size() > 1) {
				sequence.evaluation.pac.x = sequence.positionsSequence[1].x;
				sequence.evaluation.pac.y = sequence.positionsSequence[1].y;
			}*/
		}
		else if (isInWall) {
			break;
		}
	}

	// ============ EVAL HISTORY PATH ============
	if (sequence.positionsSequence.size() == 0) {
		//sequence.evaluation.scoreVars.lastUnCommonPositions -= LEN_SEQUENCE;
	}
	else {
		// ============ EVAL DISTANCE TO BIG PELLET ============
		sequence.evaluation.scoreVars.distToBigPellet = distToNearestBigPellet(
			sequence.positionsSequence.back().x, \
			sequence.positionsSequence.back().y, \
			sequence.evaluation.bigPellets);

		// ============ EVAL DISTANCE TO TEAMATE ============
		sequence.evaluation.scoreVars.distanceToNearestTeamate = distToNearestPac(\
			sequence.positionsSequence[0].x, \
			sequence.positionsSequence[0].y, \
			myPacs);
	}

	// ============ EVAL TOLTAL SCORE ============
	sequence.evaluation.fitScore = computeFitScore(sequence.evaluation.scoreVars);
}

float distToNearestPac(int x, int y, vector<PacMan> pacs) {
	float minDist = 100;
	for (int iPac = 0; iPac < pacs.size(); iPac++) {
		if (pacs[iPac].mine) {
			float x1 = float(pacs[iPac].x);
			float y1 = float(pacs[iPac].y);
			float dist = dist2Points(x, y, x1, y1);
			if (dist < minDist) {
				minDist = dist;
			}
		}
	}
	return minDist;
}

float distToNearestBigPellet(float x, float y, vector<Pellet> bigPellets)
{
	float minDist = 100;
	for (int iBPellet = 0; iBPellet < bigPellets.size(); iBPellet++) {
		float x1 = float(bigPellets[iBPellet].x);
		float y1 = float(bigPellets[iBPellet].y);
		float dist = dist2Points(x, y, x1, y1);
		if (dist < minDist) {
			minDist = dist;
		}
	}
	return minDist;
}
float dist2Points(float x1, float y1, float x2, float y2)
{
	float dist = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return dist;
}

FitScore computeFitScore(ScoreVars scoreVars)
{
	FitScore fitScore = { 0 };
	float maxDistToBigPellet = 6.;
	if (scoreVars.distToBigPellet > maxDistToBigPellet) {
		scoreVars.distToBigPellet = maxDistToBigPellet;
	}
	float maxDistToNearestTeamate = 6.;
	if (scoreVars.distanceToNearestTeamate > maxDistToNearestTeamate) {
		scoreVars.distanceToNearestTeamate = maxDistToNearestTeamate;
	}

	float maxUnVisitedFrames = LEN_SEQUENCE;

	float pelletScoreWeight = 1.2;
	float distToBigPelletWeight = 1;
	float UnVisitedFramesWeight = 1. / LEN_SEQUENCE;
	float teamateDistanceWeight = 5;
	float pathMemoryWeight = 0.33;
	float opponnentSeenScore = 2;

	fitScore.pelletScore = float(scoreVars.pelletScore) * pelletScoreWeight;
	fitScore.distToBigPelletScore = 1. / (0.17 * scoreVars.distToBigPellet) * distToBigPelletWeight;
	fitScore.unVisitedScore = float(scoreVars.unVisitedFrames) / maxUnVisitedFrames * UnVisitedFramesWeight;
	fitScore.teamateDistanceScore = -1. / (scoreVars.distanceToNearestTeamate) * teamateDistanceWeight;
	fitScore.pathMemoryScore = float(scoreVars.lastUnCommonPositions) * pathMemoryWeight;
	fitScore.opponentSeenScore = -float(scoreVars.opponentSeen) * opponnentSeenScore;

	fitScore.total = fitScore.pelletScore \
		+ fitScore.distToBigPelletScore \
		+ fitScore.unVisitedScore \
		+ fitScore.teamateDistanceScore \
		+ fitScore.pathMemoryScore \
		+ fitScore.opponentSeenScore;

	return fitScore;
}


void sortSequences(vector<Sequence>& sequences)
{
	std::sort(sequences.begin(), sequences.end(), [](Sequence seq1, Sequence seq2) {return seq1.evaluation.fitScore.total > seq2.evaluation.fitScore.total; });
}



void printSequence(const Sequence sequence)
{
	for (int i = 0; i < sequence.genes.size(); i++) {
		cerr << sequence.genes[i].direction << ' ';
	}
	cerr << endl;
}

void printMap(vector<string> map) {
	for (int i = 0; i < map.size(); i++) {
		cerr << map[i] << endl;
	}
}

void printMapPacs(vector<string> map, vector<PacMan> pacs) {
	for (int i = 0; i < pacs.size(); i++) {
		//cout << "pac: " << pacs[i].x << pacs[i].y << endl;
		map[pacs[i].y][pacs[i].x] = 'O';
	}
	for (int i = 0; i < map.size(); i++) {
		cerr << map[i] << endl;
	}
}

void printMapExtra(vector<string> map, vector<PacMan> pacs, vector<Point> visited) {
	for (int i = 0; i < pacs.size(); i++) {
		map[pacs[i].y][pacs[i].x] = 'O';
	}
	for (int i = 0; i < visited.size(); i++) {
		map[pacs[i].y][pacs[i].x] = 'v';
	}
	for (int i = 0; i < map.size(); i++) {
		cerr << map[i] << endl;
	}
}


void printScoreVars(ScoreVars scoreVars) {
	cerr << "pelletScore:\t\t\t" << scoreVars.pelletScore << endl;
	cerr << "distToBigPellet:\t\t" << scoreVars.distToBigPellet << endl;
	cerr << "unVisitedFrames:\t\t" << scoreVars.unVisitedFrames << endl;
	cerr << "distanceToNearestTeamate:\t" << scoreVars.distanceToNearestTeamate << endl;
	cerr << "lastUnCommonPositions:\t" << scoreVars.lastUnCommonPositions << endl;
}

void printFitScore(FitScore fitScore) {
	cerr << "pelletScore:\t\t\t" << fitScore.pelletScore << endl;
	cerr << "distToBigPelletScore:\t\t" << fitScore.distToBigPelletScore << endl;
	cerr << "unVisitedScore:\t\t" << fitScore.unVisitedScore << endl;
	cerr << "teamateDistanceScore:\t\t" << fitScore.teamateDistanceScore << endl;
	cerr << "pathMemoryScore:\t\t" << fitScore.pathMemoryScore << endl;
	cerr << "total:\t\t\t" << fitScore.total << endl;
}

