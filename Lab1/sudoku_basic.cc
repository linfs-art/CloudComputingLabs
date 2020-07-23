#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <string.h>
#include <iostream>

#include "sudoku.h"
#include "sudoku_basic.h"

void
Basic::find_spaces()
{
  nspaces = 0;
  for (int cell = 0; cell < N; ++cell) {
    if (board[cell] == 0)
      spaces[nspaces++] = cell;
  }
}

void
Basic::input(const char in[])
{
  for (int cell = 0; cell < N; ++cell) {
    board[cell] = in[cell] - '0';
    assert(0 <= board[cell] && board[cell] <= NUM);
  }
  find_spaces();
}

bool
Basic::available(int guess, int cell)
{
  for (int i = 0; i < NEIGHBOR; ++i) {
    int neighbor = neighbors[cell][i];
    if (board[neighbor] == guess) {
      return false;
    }
  }
  return true;
}

bool
Basic::solve_sudoku_basic(int which_space)
{
  if (which_space >= nspaces) {
    return true;
  }
  // find_min_arity(which_space);
  int cell = spaces[which_space];
  for (int guess = 1; guess <= NUM; ++guess) {
    if (available(guess, cell)) {
      // hold
      assert(board[cell] == 0);
      board[cell] = guess;
       // try
      if (solve_sudoku_basic(which_space+1)) {
        return true;
      }
         // unhold
      assert(board[cell] == guess);
      board[cell] = 0;
    }
 }
  return false;
  }

Basic::Basic (char *in)
{
  init_neighbors(neighbors);
  input (in);
}

void
Basic::basic_solve (int *solution)
{
  solve_sudoku_basic (0);
  if (!solved(chess))
    assert(0);
  memcpy (solution, board, N * 4);
}
