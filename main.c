/*******************************************************************************
 ** Name: maze
 ** Purpose: Creates a maze with pseudo 3D look.
 ** Author: (JE) Jens Elstner <jens.elstner@bka.bund.de>
 *******************************************************************************
 ** Date        User  Log
 **-----------------------------------------------------------------------------
 ** 01.06.2021  JE    Created program.
 *******************************************************************************/


//******************************************************************************
//* includes & namespaces

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "c_string.h"
#include "c_dynamic_arrays_macros.h"


//******************************************************************************
//* defines & macros

#define ME_VERSION "0.1.1"
cstr g_csMename;

#define ERR_NOERR 0x00
#define ERR_ARGS  0x01
#define ERR_FILE  0x02
#define ERR_ELSE  0xff

#define sERR_ARGS  "Argument error"
#define sERR_FILE  "File error"
#define sERR_ELSE  "Unknown error"

// Walls in grid.        //     N          2
#define CELL_NORTH   2   //     |          |
#define CELL_WEST    3   // W --+-- E  3 --+-- 5
#define CELL_SOUTH   5   //     |          |
#define CELL_EAST    7   //     S          7
#define CELL_NONE    0
#define CELL_BORDER -1
#define CELL_WHOLE (CELL_NORTH * CELL_WEST * CELL_SOUTH * CELL_EAST)
// The prime numbers multiplied equal 210 = 2 * 3 * 5 * 7

#define DIR_NORTH 0x00
#define DIR_WEST  0x01
#define DIR_SOUTH 0x02
#define DIR_EAST  0x03
#define DIR_MOD   4

#define MOVE_FRONT 0x00
#define MOVE_LEFT  0x01
#define MOVE_BACK  0x02
#define MOVE_RIGHT 0x03
#define MOVE_MOD   4

#define GRID_MAX 100

#define STACK_EMPTY (~0)


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  int iMazeW;
  int iMazeH;
} t_options;

// Arguments and options.
typedef struct s_grid {
  int  iMazeW;      // Width of maze
  int  iMazeH;      // Height of maze
  int  iGridW;      // = iMazeW + border (2)
  int  iGridH;      // = iMazeH + border (2)
  int  iMazeCount;  // = iMazeW * iMazeH
  int  iGridCount;  // = iGridW * iGridH
  int* piCell;
} t_grid;

typedef struct s_stack {
  int*   piCell;
  size_t sStackPtr;
} t_stack;

s_array(cstr);


//******************************************************************************
//* Global variables

// Arguments
t_options     g_tOpts;  // CLI options and arguments.
t_array(cstr) g_tArgs;  // Free arguments.
t_grid        g_tMaze;  // The maze's grid.
t_stack       g_tStack; // Stack for back-propagating while maze's creation.


//******************************************************************************
//* Functions

/*******************************************************************************
 * Name:  usage
 * Purpose: Print help text and exit program.
 *******************************************************************************/
void usage(int iErr, const char* pcMsg) {
  cstr csMsg = csNew(pcMsg);

  // Print at least one newline with message.
  if (csMsg.len != 0)
    csCat(&csMsg, csMsg.cStr, "\n\n");

  csSetf(&csMsg, "%s"
//|************************ 80 chars width ****************************************|
   "usage: %s [-w n] [-h n]\n"
   "       %s [--help|-v|--version]\n"
   " Creates a maze with pseudo 3D look.\n"
   " You can walk with the ijkl or wasd keys.\n"
   "  -w n:          width of maze's grid (default 20)\n"
   "  -h n:          height of maze's grid (default 10)\n"
   "  --help:        print this help\n"
   "  -v|--version:  print version of program\n"
//|************************ 80 chars width ****************************************|
         ,csMsg.cStr,
         g_csMename.cStr, g_csMename.cStr
        );

  if (iErr == ERR_NOERR)
    printf("%s", csMsg.cStr);
  else
    fprintf(stderr, "%s", csMsg.cStr);

  csFree(&csMsg);

  exit(iErr);
}

/*******************************************************************************
 * Name:  dispatchError
 * Purpose: Print out specific error message, if any occurres.
 *******************************************************************************/
void dispatchError(int rv, const char* pcMsg) {
  cstr csMsg = csNew(pcMsg);
  cstr csErr = csNew("");

  if (rv == ERR_NOERR) return;

  if (rv == ERR_ARGS) csSet(&csErr, sERR_ARGS);
  if (rv == ERR_FILE) csSet(&csErr, sERR_FILE);
  if (rv == ERR_ELSE) csSet(&csErr, sERR_ELSE);

  // Set to '<err>: <message>', if a message was given.
  if (csMsg.len != 0) csSetf(&csErr, "%s: %s", csErr.cStr, csMsg.cStr);

  usage(rv, csErr.cStr);
}

/*******************************************************************************
 * Name:  getOptions
 * Purpose: Filters command line.
 *******************************************************************************/
void getOptions(int argc, char* argv[]) {
  cstr csArgv = csNew("");
  cstr csRv   = csNew("");
  cstr csOpt  = csNew("");
  int  iArg   = 1;  // Omit program name in arg loop.
  int  iChar  = 0;
  char cOpt   = 0;

  // Set defaults.
  g_tOpts.iMazeW = 20;
  g_tOpts.iMazeH = 10;

  // Init free argument's dynamic array.
  daInit(cstr, g_tArgs);

  // Loop all arguments from command line POSIX style.
  while (iArg < argc) {
next_argument:
    shift(&csArgv, &iArg, argc, argv);
    if(strcmp(csArgv.cStr, "") == 0)
      continue;

    // Long options:
    if (csArgv.cStr[0] == '-' && csArgv.cStr[1] == '-') {
      if (!strcmp(csArgv.cStr, "--help")) {
        usage(ERR_NOERR, "");
      }
      if (!strcmp(csArgv.cStr, "--version")) {
        version();
      }
      dispatchError(ERR_ARGS, "Invalid long option");
    }

    // Short options:
    if (csArgv.cStr[0] == '-') {
      for (iChar = 1; iChar < csArgv.len; ++iChar) {
        cOpt = csArgv.cStr[iChar];
        if (cOpt == 'v') {
          version();
        }
        if (cOpt == 'w') {
          if (! getArgLong((ll*) &g_tOpts.iMazeW, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid width or missing");
          continue;
        }
        if (cOpt == 'h') {
          if (! getArgLong((ll*) &g_tOpts.iMazeH, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid height or missing");
          continue;
        }
        dispatchError(ERR_ARGS, "Invalid short option");
      }
      goto next_argument;
    }
    // Else, it's just a filename.
    daAdd(cstr, g_tArgs, csNew(csArgv.cStr));
  }

  // Sanity check of arguments and flags.
  if (g_tArgs.sCount != 0) dispatchError(ERR_ARGS, "No file needed");

  if (g_tOpts.iMazeW < 0 || g_tOpts.iMazeW > GRID_MAX)
    dispatchError(ERR_ARGS, "x dimension out of bounds");
  if (g_tOpts.iMazeH < 0 || g_tOpts.iMazeH > GRID_MAX)
    dispatchError(ERR_ARGS, "y dimension out of bounds");

  // Set grid values.
  g_tMaze.iMazeW = g_tOpts.iMazeW;
  g_tMaze.iMazeH = g_tOpts.iMazeH;
  g_tMaze.iGridW = g_tOpts.iMazeW + 2;
  g_tMaze.iGridH = g_tOpts.iMazeH + 2;
  g_tMaze.iMazeCount  = g_tMaze.iMazeW * g_tMaze.iMazeH;
  g_tMaze.iGridCount = g_tMaze.iGridW * g_tMaze.iGridH;

  // Grid and max stack. Grid will have a border with special value.
  g_tStack.piCell = (int*) malloc(g_tMaze.iMazeCount  * sizeof(int));
  g_tMaze.piCell  = (int*) malloc(g_tMaze.iGridCount * sizeof(int));

  // Init stack pointer.
  g_tStack.sStackPtr = STACK_EMPTY;

  // Free string memory.
  csFree(&csArgv);
  csFree(&csRv);
  csFree(&csOpt);
}

/*******************************************************************************
 * Name:  getch
 * Purpose: Reads key pressed from keyboards.
 *******************************************************************************/
int getch(void) {
  struct termios oldattr = {0};
  struct termios newattr = {0};
  int            ch      = 0;

  tcgetattr(STDIN_FILENO, &oldattr);
  newattr = oldattr;
  newattr.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

  return ch;
}

/*******************************************************************************
 * Name:  initRand
 * Purpose: Initialise random generator with current time.
 *******************************************************************************/
void initRand(void) {
  time_t t;
  srand((unsigned) time(&t));
}

/*******************************************************************************
 * Name:  randF
 * Purpose: Generates a random float between 0 and 0.99.
 *******************************************************************************/
float randF(void) {
  return (float) rand() / (float) RAND_MAX;
}

/*******************************************************************************
 * Name:  randI
 * Purpose: Generates an int between 0 and iTo (exclusive).
 *******************************************************************************/
int randI(int iTo) {
  return randF() * (float) iTo;
}

/*******************************************************************************
 * Name:  randIab
 * Purpose: Generates an int between iFrom and iTo (exclusive).
 *******************************************************************************/
int randIab(int iFrom, int iTo) {
  return randF() * (float) (iTo - iFrom) + iFrom;
}

/*******************************************************************************
 * Name:  xy2cell
 * Purpose: Calculates the cell offset from given X and Y coordinates.
 *******************************************************************************/
int xy2cell(int iX, int iY) {
  return iX + iY * g_tMaze.iGridW;
}

/*******************************************************************************
 * Name:  cell2xy
 * Purpose: Converts a cell index into x and y coordinates.
 *******************************************************************************/
void cell2xy(int iCell, int* piX, int* piY) {
  *piX = iCell % g_tMaze.iGridW;
  *piY = iCell / g_tMaze.iGridW;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
int pullCell(void) {
  if (g_tStack.sStackPtr == STACK_EMPTY) return -1;
  return g_tStack.piCell[--g_tStack.sStackPtr];
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void pushCell(int iCell) {
  g_tStack.piCell[g_tStack.sStackPtr++] = iCell;
}

/*******************************************************************************
 * Name:  turnRight
 * Purpose: Turn direction one step to the right.
 *******************************************************************************/
int turnRight(int iDir) {
  return (iDir + 1) % DIR_MOD;
}

/*******************************************************************************
 * Name:  turnBack
 * Purpose: Turn direction to the oposit direction.
 *******************************************************************************/
int turnBack(int iDir) {
  return (iDir + 2) % DIR_MOD;
}

/*******************************************************************************
 * Name:  turnLeft
 * Purpose: Turn direction one step to the left.
 *******************************************************************************/
int turnLeft(int iDir) {
  return (iDir + 3) % DIR_MOD;
}

/*******************************************************************************
 * Name:  getCellInDir
 * Purpose: Returns content of next cell in direction iDir.
 *******************************************************************************/
int getCellInDir(int iDir, int iCell) {
  if (iDir == DIR_NORTH) return g_tMaze.piCell[iCell - g_tMaze.iGridW];
  if (iDir == DIR_WEST)  return g_tMaze.piCell[iCell - 1];
  if (iDir == DIR_SOUTH) return g_tMaze.piCell[iCell + g_tMaze.iGridW];
  if (iDir == DIR_EAST)  return g_tMaze.piCell[iCell + 1];
  return -1;
}

/*******************************************************************************
 * Name:  isBorder
 * Purpose: Returns true if iDir points beyond cells.
 *******************************************************************************/
int isBorder(int iDir, int iCell) {
  if (getCellInDir(iDir, iCell) == CELL_BORDER) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  isDirWall
 * Purpose: Returns true if iDir points to a wall.
 *******************************************************************************/
int isDirWall(int iDir, int iCell) {
  iCell = g_tMaze.piCell[iCell];
  if (iDir == DIR_NORTH && iCell % CELL_NORTH != 0) return 1;
  if (iDir == DIR_WEST  && iCell % CELL_WEST  != 0) return 1;
  if (iDir == DIR_SOUTH && iCell % CELL_SOUTH != 0) return 1;
  if (iDir == DIR_EAST  && iCell % CELL_EAST  != 0) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  isDirCellWhole
 * Purpose: Returns true if next cell in iDir is whole.
 *******************************************************************************/
int isDirCellWhole(int iDir, int iCell) {
  if (isBorder(iDir, iCell))  return 0;
  if (isDirWall(iDir, iCell)) return 0;

  if (getCellInDir(iDir, iCell) == CELL_WHOLE) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  getOpenWalls
 * Purpose: Returns which walls in cell is open.
 *******************************************************************************/
int getOpenWalls(int iCell) {
  int iWalls = 1;
  if (g_tMaze.piCell[iCell] % CELL_NORTH == 0) iWalls *= CELL_NORTH;
  if (g_tMaze.piCell[iCell] % CELL_WEST  == 0) iWalls *= CELL_WEST;
  if (g_tMaze.piCell[iCell] % CELL_SOUTH == 0) iWalls *= CELL_SOUTH;
  if (g_tMaze.piCell[iCell] % CELL_EAST  == 0) iWalls *= CELL_EAST;
  return (iWalls == 1) ? CELL_NONE : iWalls;
}

/*******************************************************************************
 * Name:  getDirWall
 * Purpose: Returns wall in direction.
 *******************************************************************************/
int getDirWall(int iDir) {
  if (iDir == DIR_NORTH) return CELL_NORTH;
  if (iDir == DIR_WEST)  return CELL_WEST;
  if (iDir == DIR_SOUTH) return CELL_SOUTH;
  if (iDir == DIR_EAST)  return CELL_EAST;
  return CELL_NONE;
}

/*******************************************************************************
 * Name:  goToCell
 * Purpose: Just go to cell in direction.
 *******************************************************************************/
void goToCell(int iDir, int* piCell) {
  if (iDir == DIR_NORTH) *piCell -= g_tMaze.iGridW;
  if (iDir == DIR_WEST)  *piCell -= 1;
  if (iDir == DIR_SOUTH) *piCell += g_tMaze.iGridW;
  if (iDir == DIR_EAST)  *piCell += 1;
}

/*******************************************************************************
 * Name:  breakIntoCell
 * Purpose: Break into cell in direction.
 *******************************************************************************/
void breakIntoCell(int iDir, int* piCell) {
  // Break first wall of cell we come from.
  g_tMaze.piCell[*piCell] /= getDirWall(iDir);

  goToCell(iDir, piCell);

  // Break second wall of cell we gone to.
  g_tMaze.piCell[*piCell] /= getDirWall(turnBack(iDir));
}

/*******************************************************************************
 * Name:  isACellAroundWhole
 * Purpose: Looks if a neighbour cell is whole.
 *******************************************************************************/
int isACellAroundWhole(int* piDir, int iCell) {
  int iDir   = *piDir;
  int iWhole = 0;
  int iLeft  = randI(2);

  // Find a whole cell around this cell in all directions.
  for (int i = 0; i < DIR_MOD; ++i) {
    if (iLeft)
      iDir = turnLeft(iDir);
    else
      iDir = turnRight(iDir);

    if (isDirCellWhole(iDir, iCell)) {
      iWhole = 1;
      *piDir = iDir;
      break;
    }
  }

  return iWhole;
}

/*******************************************************************************
 * Name:  goneToNextWholeCell
 * Purpose: Search for the next whole cell to break into or signals finish.
 *******************************************************************************/
int goneToNextWholeCell(int* piDir, int* piCell) {
  if (isACellAroundWhole(piDir, *piCell))
    breakIntoCell(*piDir, piCell);
  else {
    // Go back one cell, until a whole cell is found.
    while (! isACellAroundWhole(piDir, *piCell)) {
      if ((*piCell = pullCell()) == -1)
        return 0;
    }
    breakIntoCell(*piDir, piCell);
  }
  return 1;
}

/*******************************************************************************
 * Name:  waitForNextKey
 * Purpose: Waits until a key is pressed.
 *******************************************************************************/
int waitForNextKey(int iDir) {
  int c = 0;

  while (1) {
    c = getch();

    if (c == 'i') return MOVE_FRONT;
    if (c == 'j') return MOVE_LEFT;   //   i
    if (c == 'k') return MOVE_BACK;   // j k l
    if (c == 'l') return MOVE_RIGHT;

    if (c == 'w') return MOVE_FRONT;
    if (c == 'a') return MOVE_LEFT;   //   w
    if (c == 's') return MOVE_BACK;   // a s d
    if (c == 'd') return MOVE_RIGHT;
  }
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void moveInGrid(int iDir, int* piCell) {
  ;
}

/*******************************************************************************
 * Name:  clearScreen
 * Purpose: Clears screen prior printing.
 *******************************************************************************/
void clearScreen(void) {
  system("clear");
}

/*******************************************************************************
 * Name:  printWallIf
 * Purpose: Prints a wall if cell contains one in wanted direction.
 *******************************************************************************/
void printWallIf(int iX, int iY, int iWall, const char* cWall, const char* cNoWall) {
  if (g_tMaze.piCell[xy2cell(iX, iY)] % iWall == 0)
    printf("%s", cWall);
  else
    printf("%s", cNoWall);
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void printMaze(int iDir, int iCell) {
  char* cCorner     = "+";
  char* cWallNS     = "---+";
  char* cNoWallNS   = "   +";
  char* cWallE      = "   |";
  char* cNoWallE    = "    ";
  char* cWallEPos   = " x |";
  char* cNoWallEPos = " x  ";
  char* cWallW      = "|";
  char* cNoWallW    = " ";
  char* cWall       = NULL;
  char* cNoWall     = NULL;
  int   iMazeW      = 0;
  int   iMazeH      = 0;

  // Get maze coordinates.
  cell2xy(iCell, &iMazeW, &iMazeH);

  // +---+---+---+     N   N   N
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S

  // First upper cell line
  printf("%s", cCorner);
  for (int x = 1; x < g_tMaze.iMazeW + 1; ++x)
    printWallIf(x, 1, CELL_NORTH, cWallNS, cNoWallNS);
  printf("\n");

  for (int y = 1; y < g_tMaze.iMazeH + 1; ++y) {

    printWallIf(1, y, CELL_WEST, cWallW, cNoWallW);
    for (int x = 1; x < g_tMaze.iMazeW + 1; ++x) {
      if (y == iMazeH && x == iMazeW) {
        cWall   = cWallEPos;
        cNoWall = cNoWallEPos;
      }
      else {
        cWall   = cWallE;
        cNoWall = cNoWallE;
      }
      printWallIf(x, y, CELL_EAST, cWall, cNoWall);
    }
    printf("\n");

    printf("%s", cCorner);
    for (int x = 1; x < g_tMaze.iMazeW + 1; ++x)
      printWallIf(x, y, CELL_SOUTH, cWallNS, cNoWallNS);
    printf("\n");
  }
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void print3DView(int iDir, int iCell) {
}

/*******************************************************************************
 * Name:  generateMaze
 * Purpose: Generates a complete maze within the border of the grid.
 *******************************************************************************/
void generateMaze(int* piCell) {
  int iCell = 0;
  int iDir  = 0;
  int iX    = 0;
  int iY    = 0;

  // Init the grid's cells and the border.
  for (int i = 0; i < g_tMaze.iGridCount; ++i)
    g_tMaze.piCell[i] = CELL_BORDER;

  for (int y = 1; y < g_tMaze.iMazeH + 1; ++y)
    for (int x = 1; x < g_tMaze.iMazeW + 1; ++x)
      g_tMaze.piCell[xy2cell(x, y)] = CELL_WHOLE;

  // Get an entry cell a the edge.
  //   X 1   2   3
  // Y +---+---+---+
  // 1 |   |   |   |       N
  //   +---+---+---+       |
  // 2 |   |   |   |   W --+-- E
  //   +---+---+---+       |
  // 3 |   |   |   |       S
  //   +---+---+---+
  iX   = randIab(1, g_tOpts.iMazeW + 1);
  iY   = randIab(1, g_tOpts.iMazeH + 1);
  iDir = randI(DIR_MOD);

  // Get the right edge for the starting cell ...
  if (iDir == DIR_NORTH) iY = g_tOpts.iMazeH;    // South border
  if (iDir == DIR_WEST)  iX = g_tOpts.iMazeW;    // East  border
  if (iDir == DIR_SOUTH) iY = 1;                 // North border
  if (iDir == DIR_EAST)  iX = 1;                 // West  border

  // ... save cell for future use ;o) ...
  iCell = xy2cell(iX, iY);

  // ... and break the first wall in appropriate border for the exit.
  if (iDir == DIR_NORTH) g_tMaze.piCell[iCell] /= CELL_SOUTH;
  if (iDir == DIR_WEST)  g_tMaze.piCell[iCell] /= CELL_EAST;
  if (iDir == DIR_SOUTH) g_tMaze.piCell[iCell] /= CELL_NORTH;
  if (iDir == DIR_EAST)  g_tMaze.piCell[iCell] /= CELL_WEST;

  // Save first cell in stack.
  pushCell(iCell);

  // Walk through the maze and break walls until no cell is left to break into.
  while (1) {
    clearScreen();
    printMaze(iDir, iCell);
// sleep(1); // DEBUG XXX
    if (! goneToNextWholeCell(&iDir, &iCell)) break;
    pushCell(iCell);
  }
exit(-1); // DEBUG XXX
  // Last cell will be the starting point.
  *piCell = iCell;
}


//******************************************************************************
//* main

int main(int argc, char *argv[]) {
  int iDir  = 0;
  int iCell = 0;

  // Save program's name.
  getMename(&g_csMename, argv[0]);

  // Get options and dispatch errors, if any.
  getOptions(argc, argv);

  initRand();

  // Start game ...
  generateMaze(&iCell);

  // .. and loop game interactions.
  while (1) {
    iDir = waitForNextKey(iDir);
    moveInGrid(iDir, &iCell);
    clearScreen();
    printMaze(iDir, iCell);
    print3DView(iDir, iCell);
  }

  // Free all used memory, prior end of program.
  daFreeEx(g_tArgs, cStr);
  free(g_tMaze.piCell);
  free(g_tStack.piCell);

  return ERR_NOERR;
}
