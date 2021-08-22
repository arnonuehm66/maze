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

// Walls in grid.      //     N          2
#define CELL_NORTH 2   //     |          |
#define CELL_WEST  3   // W --+-- E  3 --+-- 5
#define CELL_SOUTH 5   //     |          |
#define CELL_EAST  7   //     S          7
#define CELL_NONE  0
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

#define TCELL_INIT   {-1, -1, -1}
#define TCELL_NO_VAL (-1)


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  int iGridX;
  int iGridY;
} t_options;

typedef struct s_stack {
  int*   piCell;
  size_t sStackPtr;
} t_stack;

typedef struct s_cell {
  int iX;
  int iY;
  int iCell;
} t_cell;

s_array(cstr);


//******************************************************************************
//* Global variables

// Arguments
t_options     g_tOpts;  // CLI options and arguments.
t_array(cstr) g_tArgs;  // Free arguments.
int*          g_iGrid;  // The maze's grid.
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
   "usage: %s [-x n] [-y n]\n"
   "       %s [-h|--help|-v|--version]\n"
   " Creates a maze with pseudo 3D look.\n"
   " You can walk with the ijkl or wasd keys.\n"
   "  -x n:          x dimension of maze's grid (default 20)\n"
   "  -y n:          y dimension of maze's grid (default 10)\n"
   "  -h|--help:     print this help\n"
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
  g_tOpts.iGridX = 20;
  g_tOpts.iGridY = 10;

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
        if (cOpt == 'h') {
          usage(ERR_NOERR, "");
        }
        if (cOpt == 'v') {
          version();
        }
        if (cOpt == 'x') {
          if (! getArgLong((ll*) &g_tOpts.iGridX, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid x or missing");
          continue;
        }
        if (cOpt == 'y') {
          if (! getArgLong((ll*) &g_tOpts.iGridY, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid y or missing");
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

  if (g_tOpts.iGridX < 0 || g_tOpts.iGridX > GRID_MAX)
    dispatchError(ERR_ARGS, "x dimension out of bounds");
  if (g_tOpts.iGridY < 0 || g_tOpts.iGridY > GRID_MAX)
    dispatchError(ERR_ARGS, "y dimension out of bounds");

  // Grid and max stack.
  g_iGrid         = (int*) malloc(g_tOpts.iGridX * g_tOpts.iGridY * sizeof(int));
  g_tStack.piCell = (int*) malloc(g_tOpts.iGridX * g_tOpts.iGridY * sizeof(int));

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
 * Name:  cellToXY
 * Purpose: Converts a cell index into x and y coordinates.
 *******************************************************************************/
t_cell cellToXY(t_cell tCell) {
  tCell.iX = tCell.iCell % g_tOpts.iGridX;
  tCell.iY = tCell.iCell / g_tOpts.iGridX;
  return tCell;
}

/*******************************************************************************
 * Name:  cellFromXY
 * Purpose: Converts x and y coordinates into a cell index.
 *******************************************************************************/
t_cell cellFromXY(t_cell tCell) {
  tCell.iCell = tCell.iX + g_tOpts.iGridX * tCell.iY;
  return tCell;
}

/*******************************************************************************
 * Name:  cellXY
 * Purpose: Calculates the cell offset from given X and Y coordinates.
 *******************************************************************************/
t_cell cellNXY(t_cell tCell) {
  if (tCell.iX    == TCELL_NO_VAL) tCell = cellToXY(tCell);
  if (tCell.iCell == TCELL_NO_VAL) tCell = cellFromXY(tCell);
  return tCell;
}

/*******************************************************************************
 * Name:  cellXY
 * Purpose: Calculates the cell offset from given X and Y coordinates.
 *******************************************************************************/
int cellXY(int iX, int iY) {
  return iX + iY * g_tOpts.iGridX;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
int turnRight(int iDir) {
  return (iDir + 1) % DIR_MOD;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
int turnBack(int iDir) {
  return (iDir + 2) % DIR_MOD;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
int turnLeft(int iDir) {
  return (iDir + 3) % DIR_MOD;
}

/*******************************************************************************
 * Name:  isBorder
 * Purpose: Returns true if iDir points beyond cells.
 *******************************************************************************/
int isBorder(int iDir, int iX, int iY) {
  if (iDir == DIR_NORTH && iY == 0)                  return 1;
  if (iDir == DIR_WEST  && iX == 0)                  return 1;
  if (iDir == DIR_SOUTH && iY == g_tOpts.iGridY - 1) return 1;
  if (iDir == DIR_EAST  && iX == g_tOpts.iGridX - 1) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  isDirWall
 * Purpose: Returns true if iDir points to an open wall.
 *******************************************************************************/
int isDirWall(int iDir, int iX, int iY) {
  int iCell = g_iGrid[cellXY(iX, iY)];
  if (iDir == DIR_NORTH && iCell % CELL_NORTH == 0) return 1;
  if (iDir == DIR_WEST  && iCell % CELL_WEST  == 0) return 1;
  if (iDir == DIR_SOUTH && iCell % CELL_SOUTH == 0) return 1;
  if (iDir == DIR_EAST  && iCell % CELL_EAST  == 0) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  isDirCellWhole
 * Purpose: Returns true if next cell in iDir is whole.
 *******************************************************************************/
int isDirCellWhole(int iDir, int iX, int iY) {
  if (isBorder(iDir, iX, iY)) return 0;
  if (isDirWall(iDir, iX, iY)) return 0;
  if (iDir == DIR_NORTH && g_iGrid[cellXY(iX,     iY - 1)] == CELL_WHOLE) return 1;
  if (iDir == DIR_WEST  && g_iGrid[cellXY(iX - 1, iY    )] == CELL_WHOLE) return 1;
  if (iDir == DIR_SOUTH && g_iGrid[cellXY(iX,     iY + 1)] == CELL_WHOLE) return 1;
  if (iDir == DIR_EAST  && g_iGrid[cellXY(iX + 1, iY    )] == CELL_WHOLE) return 1;
  return 0;
}

/*******************************************************************************
 * Name:  getOpenWalls
 * Purpose: Returns which walls in cell is open.
 *******************************************************************************/
int getOpenWalls(int iX, int iY) {
  int iCell  = cellXY(iX, iY);
  int iWalls = 1;
  if (g_iGrid[iCell] % CELL_NORTH == 0) iWalls *= CELL_NORTH;
  if (g_iGrid[iCell] % CELL_WEST  == 0) iWalls *= CELL_WEST;
  if (g_iGrid[iCell] % CELL_SOUTH == 0) iWalls *= CELL_SOUTH;
  if (g_iGrid[iCell] % CELL_EAST  == 0) iWalls *= CELL_EAST;
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
void goToCell(int* piDir, int* piX, int* piY) {
  if (*piDir == DIR_NORTH) *piY -= 1;
  if (*piDir == DIR_WEST)  *piX -= 1;
  if (*piDir == DIR_SOUTH) *piY += 1;
  if (*piDir == DIR_EAST)  *piX += 1;
}

/*******************************************************************************
 * Name:  breakIntoCell
 * Purpose: Break into cell in direction.
 *******************************************************************************/
void breakIntoCell(int* piDir, int* piX, int* piY) {
  // Break first wall of cell we come from.
  g_iGrid[cellXY(*piX, *piY)] /= getDirWall(*piDir);

  goToCell(piDir, piX, piY);

  // Break second wall of cell we gone to.
  g_iGrid[cellXY(*piX, *piY)] /= getDirWall(turnBack(*piDir));
}

/*******************************************************************************
 * Name:  goBackAndLookForWholeCell
 * Purpose: .
 *******************************************************************************/
int goBackAndLookForWholeCell(int* piDir, int* piX, int* piY) {
  int iLeft    = turnLeft(*piDir);
  int iBack    = turnBack(*piDir);
  int iRight   = turnRight(*piDir);
  int fLeftOK  = isDirCellWhole(iLeft,  *piX, *piY);
  int fBackOK  = isDirCellWhole(iBack,  *piX, *piY);
  int fRightOK = isDirCellWhole(iRight, *piX, *piY);

  // Go back one step.
  ;

  if (! isDirCellWhole(*piDir, *piX, *piY)) {
    //
  }
  else {
    //
  }



  return 0;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void goToNextNewCell(int* piDir, int* piX, int* piY) {
  // Try to go to next cell in direction dir.
  if (isDirCellWhole(*piDir, *piX, *piY)) {
    // Break wall in direction and go to that cell.
    breakIntoCell(piDir, piX, piY);
  }
  else {
    // Go back one cell, until a whole cell is found..
    while (! isDirCellWhole(*piDir, *piX, *piY)) {
      goBackAndLookForWholeCell(piDir, piX, piY);
    }
  }

// Test git push mit Auth-Token.

  // If hit wall get random dir until a unused cell is in front.
  // If cell is used go one step back and get random dir until a unused cell is in front.
  // Break the walls to tha cell and step into it.
  // Set x and y accordingly to current (next) used cell.
  ;
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
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void generateMaze(int* piGridX, int* piGridY) {
  int iSize      = g_tOpts.iGridX * g_tOpts.iGridY;
  int iCell      = 0;
  int iDir       = 0;
  int iX         = 0;
  int iY         = 0;
  int iCellCount = 0;

  // Init the grid's cells.
  for (int i = 0; i < iSize; ++i) g_iGrid[i] = CELL_WHOLE;

  // Get an entry cell a the edge.
  //   X 0   1   2
  // Y +---+---+---+
  // 0 |   |   |   |       N
  //   +---+---+---+       |
  // 1 |   |   |   |   W --+-- E
  //   +---+---+---+       |
  // 2 |   |   |   |       S
  //   +---+---+---+
  iX   = randI(g_tOpts.iGridX);
  iY   = randI(g_tOpts.iGridY);
  iDir = randI(4);

  // Get the right edge for the starting cell ...
  if (iDir == DIR_NORTH) iY = g_tOpts.iGridY - 1;    // South border
  if (iDir == DIR_WEST)  iX = g_tOpts.iGridX - 1;    // East  border
  if (iDir == DIR_SOUTH) iY = 0;                     // North border
  if (iDir == DIR_EAST)  iX = 0;                     // West  border

  // ... save cell for future use ;o) ...
  iCell = cellXY(iX, iY);

  // ... and break the first wall in appropriate border for the exit.
  if (iDir == DIR_NORTH) g_iGrid[iCell] /= CELL_SOUTH;
  if (iDir == DIR_WEST)  g_iGrid[iCell] /= CELL_EAST;
  if (iDir == DIR_SOUTH) g_iGrid[iCell] /= CELL_NORTH;
  if (iDir == DIR_EAST)  g_iGrid[iCell] /= CELL_WEST;

  // Save first cell in stack.
  pushCell(iCell);

  // Walk through the maze and break walls until no cell is left to break into.
  while (iCellCount < iSize) {
    goToNextNewCell(&iDir, &iX, &iY);
    ++iCellCount;
  }

  // Last cell will be the starting point.
  *piGridX = iX;
  *piGridY = iY;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
int waitForNextKey(int iCellDir) {
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
void moveInGrid(int iCellDir, int* piGridX, int* piGridY) {
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
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void printWallIf(int iX, int iY, int iWall, const char* cWall, const char* cNoWall) {
  if (g_iGrid[cellXY(iX, iY)] % iWall == 0)
    printf("%s", cWall);
  else
    printf("%s", cNoWall);
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void printMaze(int iCellDir, int iGridX, int iGridY) {
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

  // +---+---+---+     N   N   N
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S
  // |   |   |   |   W   E   E   E
  // +---+---+---+     S   S   S

  // First upper cell line
  printf("%s", cCorner);
  for (int x = 0; x < g_tOpts.iGridX; ++x)
    printWallIf(x, 0, CELL_NORTH, cWallNS, cNoWallNS);
  printf("\n");

  for (int y = 0; y < g_tOpts.iGridY; ++y) {

    printWallIf(0, y, CELL_WEST, cWallW, cNoWallW);
    for (int x = 0; x < g_tOpts.iGridX; ++x) {
      if (y == iGridY && x == iGridX) {
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
    for (int x = 0; x < g_tOpts.iGridX; ++x)
      printWallIf(x, y, CELL_SOUTH, cWallNS, cNoWallNS);
    printf("\n");

  }
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void print3DView(int iCellDir, int iGridX, int iGridY) {
}


//******************************************************************************
//* main

int main(int argc, char *argv[]) {
  int iCellDir = 0;
  int iGridX   = 0;
  int iGridY   = 0;

  // Save program's name.
  getMename(&g_csMename, argv[0]);

  // Get options and dispatch errors, if any.
  getOptions(argc, argv);

  initRand();

  // Start game ...
  generateMaze(&iGridX, &iGridY);

  // .. and loop game interactions.
  while (1) {
    iCellDir = waitForNextKey(iCellDir);
    moveInGrid(iCellDir, &iGridX, &iGridY);
    clearScreen();
    printMaze(iCellDir, iGridX, iGridY);
    print3DView(iCellDir, iGridX, iGridY);
  }

  // Free all used memory, prior end of program.
  daFreeEx(g_tArgs, cStr);
  free(g_iGrid);

  return ERR_NOERR;
}
