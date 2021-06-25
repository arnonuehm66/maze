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
#define CELL_EAST  3   // W --+-- E  3 --+-- 5
#define CELL_WEST  5   //     |          |
#define CELL_SOUTH 7   //     S          7
#define CELL_NONE -1
// The prime numbers multiplied equal 210 = 2 * 3 * 5 * 7

#define GO_WEST  0x00
#define GO_NORTH 0x01
#define GO_SOUTH 0x02
#define GO_EAST  0x03

#define MOVE_FRONT 0x00
#define MOVE_LEFT  0x01
#define MOVE_RIGHT 0x02
#define MOVE_BACK  0x03


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  size_t sGridX;
  size_t sGridY;
} t_options;

s_array(cstr);


//******************************************************************************
//* Global variables

// Arguments
t_options     g_tOpts;  // CLI options and arguments.
t_array(cstr) g_tArgs;  // Free arguments.
int*          g_iGrid;  // The maze's grid.


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
  g_tOpts.sGridX = 20;
  g_tOpts.sGridY = 10;

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
          if (! getArgLong((ll*) &g_tOpts.sGridX, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid x or missing");
          continue;
        }
        if (cOpt == 'y') {
          if (! getArgLong((ll*) &g_tOpts.sGridY, &iArg, argc, argv, ARG_CLI, NULL))
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

  // Grid.
  g_iGrid = (int*) malloc(g_tOpts.sGridX * g_tOpts.sGridY * sizeof(int));

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
 * Name:  cellFromXY
 * Purpose: Calculates the cell offset from given X and Y coordinates.
 *******************************************************************************/
int cellFromXY(int iX, int iY) {
  return iX + iY * g_tOpts.sGridX;
}

/*******************************************************************************
 * Name:  getOpenWall
 * Purpose: Returns which wall in cell is open.
 *******************************************************************************/
int getOpenWall(int iX, int iY) {
  int iCell = cellFromXY(iX, iY);
  if (g_iGrid[iCell] % CELL_NORTH == 0) return CELL_NORTH;
  if (g_iGrid[iCell] % CELL_EAST  == 0) return CELL_EAST;
  if (g_iGrid[iCell] % CELL_WEST  == 0) return CELL_WEST;
  if (g_iGrid[iCell] % CELL_SOUTH == 0) return CELL_SOUTH;
  return CELL_NONE;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void goToNextNewCell(int* piX, int* piY) {
  int iDir = randI(4);

  // Try to go to next cell in direction dir.
  if (
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
void generateMaze(int* piGridX, int* piGridY) {
  int    iSize      = g_tOpts.sGridX * g_tOpts.sGridY;
  int    iInit      = CELL_EAST * CELL_NORTH * CELL_SOUTH * CELL_WEST;
  int    iCell      = 0;
  int    iDir       = 0;
  int    iX         = 0;
  int    iY         = 0;
  int    iCellCount = 0;

  // Init the grid's cells.
  for (int i = 0; i < iSize; ++i) g_iGrid[i] = iInit;

  // Get an entry cell a the edge.
  //   X 0   1   2
  // Y +---+---+---+
  // 0 |   |   |   |       N
  //   +---+---+---+       |
  // 1 |   |   |   |   W --+-- E
  //   +---+---+---+       |
  // 2 |   |   |   |       S
  //   +---+---+---+
  iX   = randI(g_tOpts.sGridX);
  iY   = randI(g_tOpts.sGridY);
  iDir = randI(4);

  // Get the right edge for the starting cell ...
  if (iDir == GO_EAST)  iX = 0;                     // West  border
  if (iDir == GO_SOUTH) iY = 0;                     // North border
  if (iDir == GO_WEST)  iX = g_tOpts.sGridX - 1;    // East  border
  if (iDir == GO_NORTH) iY = g_tOpts.sGridY - 1;    // South border
  // ... and break the first wall in appropriate border for the exit.
  if (iDir == GO_EAST)  g_iGrid[cellFromXY(iX, iY)] /= CELL_WEST;
  if (iDir == GO_SOUTH) g_iGrid[cellFromXY(iX, iY)] /= CELL_NORTH;
  if (iDir == GO_WEST)  g_iGrid[cellFromXY(iX, iY)] /= CELL_EAST;
  if (iDir == GO_NORTH) g_iGrid[cellFromXY(iX, iY)] /= CELL_SOUTH;

  // Walk through the maze and break walls until no cell is left to break into.
  while (iCellCount < iSize) {
    goToNextNewCell(&iX, &iY);
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
  if (g_iGrid[cellFromXY(iX, iY)] % iWall == 0)
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
  for (int x = 0; x < g_tOpts.sGridX; ++x)
    printWallIf(x, 0, CELL_NORTH, cWallNS, cNoWallNS);
  printf("\n");

  for (int y = 0; y < g_tOpts.sGridY; ++y) {

    printWallIf(0, y, CELL_WEST, cWallW, cNoWallW);
    for (int x = 0; x < g_tOpts.sGridX; ++x) {
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
    for (int x = 0; x < g_tOpts.sGridX; ++x)
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
