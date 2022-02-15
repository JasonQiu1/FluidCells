# Fluid Cells
[![asciicast](https://asciinema.org/a/EBtZJEoXWW0EIQi6H28NKMWQ2.svg)](https://asciinema.org/a/EBtZJEoXWW0EIQi6H28NKMWQ2)
A cellular automata sim for incompressible fluids built with an ncurses display.

Supports fast pressure equalization for multiple bodies of water.

## Requirements
- C compiler 
- ncurses

## Build
`make` or compile manually.

## Run
Open up a terminal in fullscreen and run `./fluidcells map` in the root directory.

If parts of the map are cutting out, then try decreasing your terminal's font size with Ctrl+- until you can see the entire display.

You can swap out `map` with one of the maps in the root directory (e.g. `map2`, `map3`, etc.) or make your own.

## Too slow?
If you think the pressure equalization is too slow, it may be because the the equalization is intentionally limited by the size of the opening it is moving through for a bit more realism.

You can change this in 4 ways:
1. Increase the opening in the map.
2. Decrease the timestep.
3. Change the max flow calculation.
4. Remove the max flow limit.

## Make your own map
A map is a very simple text document so you can easily modify it and make your own.

The first line contains two space separated numbers that gives the program information about the grid size of the map.

For example, `45 29` means a grid of 45 rows and 29 columns.

After that should be y rows of x digits, where y and x are the numbers from earlier.

The digits currently supported are:

0: Air
1: Solid
2: Fluid

Take a look at the example maps in the repo and run them to get a better idea!
