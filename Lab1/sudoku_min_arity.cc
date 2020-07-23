#include <assert.h>
#include <algorithm>
#include <string.h>

#include "sudoku.h"
#include "sudoku_min_arity.h"

void
Mina::find_spaces()
{
  nspaces = 0;
  for (int cell = 0; cell < N; ++cell) {
    if (board[cell] == 0)
      spaces[nspaces++] = cell;
  }
}

void
Mina::input(const char in[])
  {
    for (int cell = 0; cell < N; ++cell) {
      board[cell] = in[cell] - '0';
      assert(0 <= board[cell] && board[cell] <= NUM);
    }
    find_spaces();
  }

bool
Mina::available(int guess, int cell)
{
  for (int i = 0; i < NEIGHBOR; ++i) {
    int neighbor = neighbors[cell][i];
    if (board[neighbor] == guess) {
      return false;
    }
  }
  return true;
}

int
Mina::arity(int cell)
{
  bool occupied[10] = {false};
  for (int i = 0; i < NEIGHBOR; ++i) {
    int neighbor = neighbors[cell][i];
    occupied[board[neighbor]] = true;
  }
  return std::count(occupied+1, occupied+10, false);
}

void
Mina::find_min_arity(int space)
{
  int cell = spaces[space];
  int min_space = space;
  int min_arity = arity(cell);

  for (int sp = space+1; sp < nspaces && min_arity > 1; ++sp) {
    int cur_arity = arity(spaces[sp]);
    if (cur_arity < min_arity) {
      min_arity = cur_arity;
      min_space = sp;
    }
  }

  if (space != min_space) {
    std::swap(spaces[min_space], spaces[space]);
  }
}

bool
Mina::solve_sudoku_min_arity(int which_space)
{
  if (which_space >= nspaces) {
    return true;
  }

  find_min_arity(which_space);
  int cell = spaces[which_space];

  for (int guess = 1; guess <= NUM; ++guess) {
    if (available(guess, cell)) {
      // hold
      assert(board[cell] == 0);
      board[cell] = guess;

      // try
      if (solve_sudoku_min_arity(which_space+1)) {
        return true;
      }

      // unhold
      assert(board[cell] == guess);
      board[cell] = 0;
    }
  }
  return false;
}

Mina::Mina (char *in)
{
  init_neighbors(neighbors);
  input (in);
}

void
Mina::mina_solve (int *solution)
{
  solve_sudoku_min_arity (0);
  if (!solved(chess))
    assert(0);
  memcpy (solution, board, N * 4);
}