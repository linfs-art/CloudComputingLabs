#include <assert.h>

#include <algorithm>

#include "sudoku.h"

struct Mina
{
  int board[N];
  int spaces[N];
  int neighbors[N][NEIGHBOR];
  int nspaces;
  int (*chess)[COL] = (int (*)[COL])board;
  
  void find_spaces();
  void input(const char in[]);
  bool available(int guess, int cell);
  int arity(int cell);
  void find_min_arity(int space);
  bool solve_sudoku_min_arity(int which_space);
  Mina (char *in);
  void mina_solve (int *solution);
};
