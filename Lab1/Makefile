CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_test

sudoku_test: sudoku_main.cc neighbor.cc sudoku_basic.cc sudoku_dancing_links.cc sudoku_min_arity.cc sudoku_min_arity_cache.cc sudoku_thread.cc
	g++ -O2 -o $@ $^ -pthread -std=c++11
