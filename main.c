#include <ncurses.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msleep.h"

#include "bucketqueue.h"

int gridY = -1, gridX = -1;
WINDOW* cellw = NULL;
WINDOW* pressurew = NULL;
int** cells = NULL;
int** pressures = NULL;
int** checkedCells = NULL;
// CheckedCells:
// -2 = Floating fluid
// -1 = Nonfluid cell
// 0 = Nonchecked cell
// n = nth connected body of water (currWaterCnt)
int evenFrame = 0;
int currWaterCnt = 0;
int minPressure = 0;

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

typedef enum CellType {
    AIR = 0,
    SOLID,
    FLUID
} CellType;


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

// Assume current cell has not been checked and is FLUID
// Sets pressure for current cell and checks it
void updatePressure(int r, int c) {
    if (checkedCells[r][c]) return;
    checkedCells[r][c] = currWaterCnt;

    // Update cell pressure -- precedence: down, up, left, right
    if (r < gridY-1 && checkedCells[r+1][c] == currWaterCnt) {
        pressures[r][c] = pressures[r+1][c]-1;
    } else if (r > 0 && checkedCells[r-1][c] == currWaterCnt) {
        pressures[r][c] = pressures[r-1][c]+1;
    } else if (c > 0 && checkedCells[r][c-1] == currWaterCnt) {
        pressures[r][c] = pressures[r][c-1];
    } else if (c < gridX - 1 && checkedCells[r][c+1] == currWaterCnt) {
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

// Add absolute value of minPressure to all current body of water
void balancePressures(int num) {
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (checkedCells[r][c] == currWaterCnt) {
                pressures[r][c] += num;
            }
        }
    }
}

void updateAllPressures() {
    clearPressures();
    clearCheckedCells();
    currWaterCnt = 1;

    // First pass, 0 pressure everything above air
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (!checkedCells[r][c] && cells[r][c] == AIR) {
                checkedCells[r][c] = -1;
                for (int rr = r-1; rr >= 0; rr--) {
                    if (cells[rr][c] == SOLID) {
                        break;
                    } else if (cells[rr][c] == FLUID) {
                        checkedCells[rr][c] = -2;
                        pressures[rr][c] = -1;
                    } else {
                        checkedCells[rr][c] = -1;
                        pressures[rr][c] = 0;
                    }
                }
            }
        }
    }
    
    // Update pressures for each body of water
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
                if (minPressure < 0) {
                    balancePressures(-minPressure);
                }

                currWaterCnt++;
            } else if (cells[r][c] != FLUID) {
                checkedCells[r][c] = -1;
                pressures[r][c] = -1;
            }
        }
    }
}

void step() {
    updateAllPressures();
    int r, c, direction;
    for (r = gridY-1; r >= 0; r--) {
        direction = r % 2 ^ evenFrame; // determinstic simulation that flips
                                       // which side to start on based on row
                                       // and frame number
        for ((direction) ? (c = 0) : (c = gridX-1); (direction) ? c < gridX : c >= 0; (direction) ? c++ : c--) {
            switch(cells[r][c]) {
                case SOLID:
                    break;
                case FLUID:
                    if (r < gridY-1 && cells[r+1][c] == AIR) {
                        // Check below for air
                        cells[r+1][c] = FLUID;
                        cells[r][c] = AIR;
                    } else if (r > 0 && cells[r-1][c] == AIR && pressures[r][c] >= 1) {
                        // Siphon pressure 0 water to 
                        // surface pressure >= 2 water from same body of water
                        // TODO: Limit water siphoned per body of water based
                        // on pressure and opening size
                        // Note: can optimize by keeping track of 0
                        // pressure surface water cells for each body of water
                        for (int rr = 0; rr < r; rr++) {
                            for (int cc = 0; cc < gridX; cc++) {
                                if (checkedCells[rr][cc] == checkedCells[r][c] 
                                    && pressures[rr][cc] == 0)
                                {
                                    cells[rr][cc] = AIR;
                                    checkedCells[rr][cc] = 0;
                                    cells[r-1][c] = FLUID;
                                    checkedCells[r-1][c] = checkedCells[r][c];
                                    pressures[r-1][c] = pressures[r][c]-1;

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
                case AIR:
                default:
                    break;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize map
    FILE* fpMap = fopen("./map", "r");
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

    pressureQueue = createBucketQueue(DIRPRIORITYMAX, gridY * gridX);

    char ch;
    while(ch != 'q') {
        redrawCells();
        step();
        redrawPressures();
        wrefresh(cellw);
        wrefresh(pressurew);
        ch = getch();
        evenFrame ^= 1;
        msleep(50);
    }

    endwin(); 
    delBucketQueue(pressureQueue);
    return 0;
}
