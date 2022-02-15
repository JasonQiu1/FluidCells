#include <ncurses.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msleep.h"

#include "bucketqueue.h"

int gridY = -1, gridX = -1;

typedef enum CellType {
    AIR = 0,
    SOLID,
    FLUID
} CellType;
int** cells = NULL;

int** checkedCells = NULL;
// CheckedCells:
// -2 = Floating fluid
// -1 = Nonfluid cell
// 0 = Nonchecked cell
// n = nth connected body of water (waterBodyID)
int** pressures = NULL;
int waterBodyID = 0;
int* maxFlow;
int* currFlow;
int minPressure = 0;

WINDOW* cellw = NULL;
WINDOW* pressurew = NULL;

// Bucket queue setup for pressures
typedef struct Cell {
    int r;
    int c;
} Cell;

Cell* createCell(int r, int c) {
    Cell* newC = malloc(sizeof *newC);
    newC->r = r;
    newC->c = c;
    return newC;
}

void delCell(Cell* c) {
    free(c);
}

int cellCmp(void* a, void* b) {
    return (((Cell*)a)->r - ((Cell*)b)->r) +
           (((Cell*)a)->c - ((Cell*)b)->c);
}

enum DirPriority {
    RIGHT = 0,
    LEFT,
    UP,
    DOWN,
    DIRPRIORITYMAX
};

BucketQueue* pressureQueue = NULL;
// end BQ setup

// Draws the cells on the ncurses window
void redrawCells() {
    werase(cellw); 
    wmove(cellw, 0,0);

    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            switch(cells[r][c]) {
                case SOLID:
                    waddch(cellw, '#');
                    break;
                case FLUID:
                    wattron(cellw, A_DIM);
                    waddch(cellw, 'v'); 
                    wattroff(cellw, A_DIM);
                    break;
                case AIR:
                default:
                    waddch(cellw, '.');
                    break;
            }
        }
    }
}

// Draws the pressures on the pressure ncurses window
void redrawPressures() {
    werase(pressurew); 
    wmove(pressurew, 0,0);

    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            char ch;
            if (pressures[r][c] == -1) ch = '.';
            else if (pressures[r][c] < 10) ch = ((char) pressures[r][c]) + '0';
            else ch = ((char) (pressures[r][c]-10) + 'A');
            waddch(pressurew, ch);
        }
    }
}

void clearCheckedCells() {
    for (int r = 0; r < gridY; r++) {
        memset(checkedCells[r], 0, gridX * sizeof **checkedCells);
    }
}

void clearPressures() {
    for (int r = 0; r < gridY; r++) {
        memset(pressures[r], 0, gridX * sizeof **pressures);
    }
}

void clearFlow() {
    memset(currFlow, 0, gridX * gridY / 2 * sizeof *currFlow);
}

// Updates pressure for a particular cell and adds valid neighbors to the queue.
void updatePressure(int r, int c) {
    if (checkedCells[r][c]) return;
    checkedCells[r][c] = waterBodyID;

    // Update cell pressure -- precedence: down, up, left, right
    if (r < gridY-1 && checkedCells[r+1][c] == waterBodyID) {
        pressures[r][c] = pressures[r+1][c]-1;
    } else if (r > 0 && checkedCells[r-1][c] == waterBodyID) {
        pressures[r][c] = pressures[r-1][c]+1;
    } else if (c > 0 && checkedCells[r][c-1] == waterBodyID) {
        pressures[r][c] = pressures[r][c-1];
    } else if (c < gridX-1 && checkedCells[r][c+1] == waterBodyID) {
        pressures[r][c] = pressures[r][c+1];
    }
    
    // Update min pressure (for balancing later)
    if (pressures[r][c] < minPressure) 
        minPressure = pressures[r][c];

    // Add neighbors to queue: right, left, up, down
    if (c < gridX-1 && !checkedCells[r][c+1] && cells[r][c+1] == FLUID)
        bucketQueueInsert(createCell(r, c+1), RIGHT, pressureQueue, &cellCmp);
    if (c > 0 && !checkedCells[r][c-1] && cells[r][c-1] == FLUID)
        bucketQueueInsert(createCell(r, c-1), LEFT, pressureQueue, &cellCmp);
    if (r > 0 && !checkedCells[r-1][c] && cells[r-1][c] == FLUID)
        bucketQueueInsert(createCell(r-1, c), DOWN, pressureQueue, &cellCmp);
    if (r < gridY-1 && !checkedCells[r+1][c] && cells[r+1][c] == FLUID)
        bucketQueueInsert(createCell(r+1, c), UP, pressureQueue, &cellCmp);
}

// Add num to all pressures in current body of water
void balancePressures(int num) {
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (checkedCells[r][c] == waterBodyID) {
                pressures[r][c] += num;
            }
        }
    }
}

// Runs multiple passes through the grid to update pressures and related structs
void updateAllPressures() {
    clearPressures();
    clearCheckedCells();
    waterBodyID = 1;

    // First pass, 0 pressure everything above air until a solid cell
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (!checkedCells[r][c] && cells[r][c] == AIR) {
                checkedCells[r][c] = -1;
                for (int rr = r-1; rr >= 0; rr--) {
                    if (cells[rr][c] == SOLID) {
                        break;
                    } else if (cells[rr][c] == FLUID) {
                        checkedCells[rr][c] = -2;
                        pressures[rr][c] = 0;
                    } else {
                        checkedCells[rr][c] = -1;
                        pressures[rr][c] = 0;
                    }
                }
            }
        }
    }
    
    // Finds each body of water and updates their water cells
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (!checkedCells[r][c] && cells[r][c] == FLUID) {
                minPressure = 0;

                // Second pass (preliminary pressure update for current water)
                bucketQueueInsert(createCell(r, c), RIGHT, 
                                  pressureQueue, &cellCmp);
                Cell* next;
                while ((next = (Cell*)bucketQueueExtractMin(pressureQueue))) {
                    updatePressure(next->r, next->c);
                    delCell(next);
                }

                // Third pass (balance negative pressure in current water)
                // This is for when water is higher on the right in a body
                if (minPressure < 0) {
                    balancePressures(-minPressure);
                }

                // Fourth pass (find max flow of current water)
                // In current water, find the minimum section of vertical water
                // that is in a horizontal "pipe"
                // A "pipe" is a column of water cells sandwiched by solid cells
                // This is for limiting siphoning to be a bit more realistic.
                // It won't be correct for a body of water siphoning through
                // different size "pipes" at the same exact frame, and does not
                // take into account vertical pipes, but it's good enough
                maxFlow[waterBodyID] = gridY;
                int flow;
                int pipeStart = 0;
                for (int cc = 0; cc < gridX; cc++) {
                    for (int rr = 0; rr < gridY; rr++) {
                        if (!pipeStart && cells[rr][cc] == SOLID) {
                            // start of pipe
                            pipeStart = 1; 
                            flow = 0;
                        } else if (pipeStart) {
                            if (cells[rr][cc] != FLUID) {
                                // end of "pipe", if actually a pipe, then
                                // try updating maxflow
                                if (cells[rr][cc] == AIR) {
                                    pipeStart = 0;
                                }

                                if (cells[rr][cc] == SOLID && flow > 0 
                                    && flow < maxFlow[waterBodyID]) 
                                {
                                    maxFlow[waterBodyID] = flow;
                                }
                            } else {
                                flow++;
                            }
                        }                     
                    }
                    // treat bottom of map as solid
                    if (pipeStart && flow > 0 && flow < maxFlow[waterBodyID]) {
                        maxFlow[waterBodyID] = flow;
                    }
                }

                waterBodyID++;
            } else if (cells[r][c] != FLUID) {
                checkedCells[r][c] = -1;
                pressures[r][c] = -1;
            }
        }
    }
}

// Does 1 step for the cellular automata.
void step() {
    updateAllPressures();
    clearFlow();

    int r, c, direction;
    for (r = gridY-1; r >= 0; r--) {
        direction = (r & 1); // deterministic simulation that flips
                             // which side to start on based on row
        for ((direction) ? (c = 0) : (c = gridX-1); 
                (direction) ? c < gridX : c >= 0; (direction) ? c++ : c--) 
        {
            if (pressures[r][c] == -1) continue; // skip siphoned cells
                                                 // I think the sim should work
                                                 // without this, but bugs pop
                                                 // up when I don't use it
            // Switch case in case I add more mobile cells in the future
            switch(cells[r][c]) {
                case FLUID:
                    if (r < gridY-1 && cells[r+1][c] == AIR) {
                        // Check below for air
                        cells[r+1][c] = FLUID;
                        cells[r][c] = AIR;
                    } else if (r > 0 && cells[r-1][c] == AIR 
                               && pressures[r][c] >= 1) 
                    {
                        // Siphon lower pressure water cell from same body of
                        // water to the air cell above this one
                        //
                        // Amount of water siphoned per body of water is limited
                        // to the minimum height of a pipe it travels through
                        const int currWaterBodyID = checkedCells[r][c];
                        for (int rr = 0; rr < r; rr++) {
                            for (int cc = 0; cc < gridX; cc++) {
                                if (checkedCells[rr][cc] == currWaterBodyID 
                                    && pressures[rr][cc] < pressures[r][c]
                                    && currFlow[currWaterBodyID] < 
                                        maxFlow[currWaterBodyID])
                                {
                                    // Move low pressure water to above this
                                    cells[rr][cc] = AIR;
                                    checkedCells[rr][cc] = 0;
                                    cells[r-1][c] = FLUID;
                                    checkedCells[r-1][c] = currWaterBodyID;
                                    pressures[r-1][c] = pressures[r][c]-1;

                                    // mark siphoned water as stepped
                                    pressures[r-1][c] = -1;

                                    // increment amount siphoned for this body
                                    currFlow[currWaterBodyID]++;

                                    // break out of both loops
                                    rr = r;
                                    break;
                                }
                            }
                        }
                    } else if (c > 0 && cells[r][c-1] == AIR) {
                        // Check left for air
                        if (r < gridY-1 && cells[r+1][c-1] == AIR) {
                            // Check left-down for air
                            cells[r+1][c-1] = FLUID;
                            cells[r][c] = AIR;
                        } else {
                            cells[r][c-1] = FLUID;
                            cells[r][c] = AIR;
                        }
                    } else if (c < gridX-1 && cells[r][c+1] == AIR) {
                        // Check right for air
                        if (r < gridY-1 && cells[r+1][c+1] == AIR) {
                            // Check right-down for air
                            cells[r+1][c+1] = FLUID;
                            cells[r][c] = AIR;
                        } else {
                            cells[r][c+1] = FLUID;
                            cells[r][c] = AIR;
                        }
                    }
                    break;
                case SOLID:
                case AIR:
                default:
                    break;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s mapFile\n", argv[0]);
        return 0;
    }

    // Initialize map and related structures
    FILE* fpMap = fopen(argv[1], "r");
    if (!fpMap) { 
        fprintf(stderr, "Map not found!\n");
        return -1;
    }
    fscanf(fpMap, "%i %i\n", &gridY, &gridX);
    fprintf(stderr, "Map size: %ix%i\n", gridY, gridX);

    cells = malloc(gridY * sizeof *cells);
    pressures = malloc(gridY * sizeof *pressures);
    checkedCells = malloc(gridY * sizeof *checkedCells);
    char* line = malloc(gridX * sizeof *line + 2);
    for (int r = 0; r < gridY; r++) {
        cells[r] = malloc(gridX * sizeof **cells);
        pressures[r] = malloc(gridX * sizeof **cells);
        checkedCells[r] = malloc(gridX * sizeof **checkedCells);
        if (!fgets(line, gridX+2, fpMap)) {
            fprintf(stderr, "Error reading map input!\n");
            return -1;
        }
        for (int c = 0; c < gridX; c++) {
            cells[r][c] = line[c] - '0';
        }
    }
    free(line);
    fclose(fpMap);

    // Initialize ncurses
    initscr(); noecho(); 
    nodelay(stdscr, TRUE); // Change this to FALSE for interactive stepping.
    box(stdscr, 0, 0); refresh();

    cellw = newwin(gridY, gridX, 1, 1);
    pressurew = newwin(gridY, gridX, 1, gridX+2);

    // Initialize pressure structures
    // TODO should be able to optimize space better
    pressureQueue = createBucketQueue(DIRPRIORITYMAX, gridY * gridX);
    maxFlow = malloc(gridX * gridY / 2 * sizeof *maxFlow);
    currFlow = calloc(gridX * gridY / 2, sizeof *maxFlow);

    // Main loop
    char ch;
    while(ch != 'q') {
        redrawCells();
        step();
        redrawPressures();
        wrefresh(cellw);
        wrefresh(pressurew);
        ch = getch();
        msleep(25); // Change the timestep if you want it to go faster/slower.
    }

    // TODO: free everything
    endwin(); 
    delBucketQueue(pressureQueue);
    return 0;
}
