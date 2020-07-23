#include <assert.h>
#include <memory.h>
#include <map>
#include <vector>

#include "sudoku.h"
using namespace std;

struct Node;
typedef Node Column;
struct Node
{
    Node* left;
    Node* right;
    Node* up;
    Node* down;
    Column* col;
    int name;
    int size;
};

const int kMaxNodes = 1 + 81*4 + 9*9*9*4;
const int kMaxColumns = 400;
const int kRow = 100, kCol = 200, kBox = 300;


struct Dance
{
    int board[N];
    int spaces[N];
    int neighbors[N][NEIGHBOR];
    int nspaces;
    int (*chess)[COL] = (int (*)[COL])board;

    Column* root_;
    int*    inout_;
    Column* columns_[400];
    vector<Node*> stack_;
    Node    nodes_[kMaxNodes];
    int     cur_node_;

    void find_spaces ();
    void input(const char in[]);
    Column* new_column(int n);
    void append_column(int n);
    Node* new_row(int col);
    int get_row_col(int row, int val);
    int get_col_col(int col, int val);
    int get_box_col(int box, int val);
    Dance(char *in);
    Column* get_min_column();
    void cover(Column* c);
    void uncover(Column* c);
    bool solve();
    void put_left(Column* old, Column* nnew);
    void put_up(Column* old, Node* nnew);
    void dance_solve (int *solution);
    void sudoku_dancing_links_solve(int inout[81]);
};