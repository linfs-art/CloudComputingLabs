#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <string.h>

#include "sudoku.h"

struct Basic 
{
  int board[N];
  int spaces[N];
  int neighbors[N][NEIGHBOR];
  int nspaces;
  int (*chess)[COL] = (int (*)[COL])board;

  void find_spaces();
  void input(const char in[]);

  bool available(int guess, int cell);

  bool solve_sudoku_basic(int which_space);

  Basic (char *in);

  void basic_solve (int *solution);
};
