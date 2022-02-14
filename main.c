#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "msleep.h"

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
                    waddch(cellw, ACS_CKBOARD);
                    break;
                case FLUID:
                    wattron(cellw, A_DIM);
                    waddch(cellw, ACS_CKBOARD); 
                    wattroff(cellw, A_DIM);
                    break;
                case AIR:
                default:
                    waddch(cellw, ACS_BULLET);
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
            if (pressures[r][c] < 10) ch = ((char) pressures[r][c]) + '0';
            //else ch = ((char) (pressures[r][c]-10) + 'A');
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

int currWaterCnt;
int minPressure;
// Assume current cell has not been checked and is FLUID
// Sets pressure for current cell and checks it
void updatePressuresDFS(int r, int c) {
    checkedCells[r][c] = currWaterCnt;

    // Update cell pressure precedence: bottom, top, right, left
    // For right and left, the water must be grounded
    if (r < gridY-1 && checkedCells[r+1][c] == currWaterCnt) {
        pressures[r][c] = pressures[r+1][c]-1;
    } else if (r > 0 && checkedCells[r-1][c] == currWaterCnt) {
        pressures[r][c] = pressures[r-1][c]+1;
    } else if (r == gridY-1 || cells[r+1][c] == SOLID) {
        if (c < gridX-1 && checkedCells[r][c+1] == currWaterCnt 
            && (r == gridY-1 || cells[r+1][c+1] == SOLID))
        {
            pressures[r][c] = pressures[r][c+1];
        } else if (c > 0 && checkedCells[r][c-1] == currWaterCnt 
                && (r == gridY-1 || cells[r+1][c-1] == SOLID))
        {
            pressures[r][c] = pressures[r][c-1];
        }
    }
    
    if (pressures[r][c] < minPressure) minPressure = pressures[r][c];

    // Check bottom depth then right, left, and up
    if (r < gridY-1 && !checkedCells[r+1][c] && cells[r+1][c] == FLUID)
        updatePressuresDFS(r+1, c);
    if (c < gridX-1 && !checkedCells[r][c+1] && cells[r][c+1] == FLUID)
        updatePressuresDFS(r, c+1);
    if (c > 0 && !checkedCells[r][c-1] && cells[r][c-1] == FLUID)
        updatePressuresDFS(r, c-1);
    if (r > 0 && !checkedCells[r-1][c] && cells[r-1][c] == FLUID)
        updatePressuresDFS(r-1, c);
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

void updatePressures() {
    clearPressures();
    clearCheckedCells();
    currWaterCnt = 1;
    
    // DFS every body of water
    for (int r = 0; r < gridY; r++) {
        for (int c = 0; c < gridX; c++) {
            if (!checkedCells[r][c] && cells[r][c] == AIR) {
                // First pass, 0 pressure everything above air
                checkedCells[r][c] = -1;
                for (int rr = r-1; rr >= 0; rr--) {
                    if (cells[rr][c] == FLUID) checkedCells[rr][c] = -2;
                    else checkedCells[rr][c] = -1;
                    pressures[rr][c] = 0;
                }
            } else if (!checkedCells[r][c] && cells[r][c] == FLUID) {
                minPressure = 0;

                // Second pass (preliminary pressure update for current water)
                updatePressuresDFS(r, c);

                // Third pass (balance negative pressure in current water)
                if (minPressure < 0) {
                    balancePressures(-minPressure);
                }
                currWaterCnt++;
            } else {
                checkedCells[r][c] = -1;
            }
        }
    }
}

void step() {
    updatePressures();
    int r, c, randint;
    for (r = gridY-1; r >= 0; r--) {
        randint = rand() % 2;
        for ((randint) ? (c = 0) : (c = gridX-1); (randint) ? c < gridX-1 : c >= 0; (randint) ? c++ : c--) {
            switch(cells[r][c]) {
                case SOLID:
                    break;
                case FLUID:
                    if (r < gridY-1 && cells[r+1][c] == AIR) {
                        // Check below for air
                        cells[r+1][c] = FLUID;
                        cells[r][c] = AIR;
                    } else if (r > 0 && cells[r-1][c] == AIR && pressures[r][c] > 0) {
                        // TODO:
                        // If pressure is more than it should be, siphon surface
                        // water that belongs to the same body of water
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
    time_t t;
    srand((unsigned) time(&t));

    initscr(); noecho(); 
    nodelay(stdscr, TRUE);
    box(stdscr, 0, 0); refresh();

    gridY = 40;
    gridX = gridY * 2;
    cellw = newwin(gridY, gridX, 1, 1);
    pressurew = newwin(gridY, gridX, 1, gridX + 2);

    cells = malloc(gridY * sizeof *cells);
    pressures = malloc(gridY * sizeof *pressures);
    checkedCells = malloc(gridY * sizeof *checkedCells);
    for (int r = 0; r < gridY; r++) {
        cells[r] = malloc(gridX * sizeof **cells);
        pressures[r] = malloc(gridX * sizeof **cells);
        checkedCells[r] = malloc(gridX * sizeof **checkedCells);
        for (int c = 0; c < gridX; c++) {
            if (r > (gridY / 5) && r < (gridY / 7 * 5 + 3) && c > gridX / 3 && c < gridX / 3 * 2) {
                cells[r][c] = SOLID;
            } else if (r > (gridY / 10) && r < (gridY / 4 * 3) && c > gridX / 2 && c < gridX / 4 * 3) {
                cells[r][c] = AIR;
            } else if (r > 0 && r < (gridY / 4 * 3) && c > gridX / 4 && c < gridX / 4 * 3) {
                cells[r][c] = FLUID;
            } else if (r < (gridY / 3)) {
                cells[r][c] = AIR;
            } else {
                cells[r][c] = SOLID;
            }
        }
    }

    char ch;
    while(ch != 'q') {
        redrawCells();
        step();
        redrawPressures();
        wrefresh(cellw);
        wrefresh(pressurew);
        ch = getch();
        msleep(10);
    }

    endwin(); 
    return 0;
}
