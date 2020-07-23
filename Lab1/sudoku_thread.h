#include <queue>
#include <map>
#include <vector>
#include <condition_variable>

using namespace std;

#ifndef _SUDOKU_THREAD_H
#define _SUDOKU_THREAD_H

#define _FUNC_TYPE int
#define _MODE_TYPE int

//basic/advanced mode
#define BASIC_MODE    0
#define ADVANCED_MODE 1

//sudoku solving algorithm
#define BASIC 0
#define DANCE 1
#define	MINA  2
#define MINAC 3

#define FILE_READ_THRD     0
#define PUZZLE_ASSIGN_THRD 1
#define SUDOKU_SOLVE_THRD  2
#define MERGE_THRD         3
#define PRINT_THRD         4

struct Cv_lk
{
	condition_variable cv;
	mutex cv_mtx;
};

struct File_stream_desc
{
	FILE* file_stream;
	char* p_buf;
	long int file_size;
};

struct Sudoku_puzzle_desc
{
	long int ith_puzzle;
	char sudoku_puzzle [90];
};

struct Sudoku_solution_desc
{
	long int ith_puzzle;
	int sudoku_solution [90];
};

struct Sudoku_queues 
{
	queue<struct Sudoku_puzzle_desc*> *puzzle_queue;
	queue<struct Sudoku_solution_desc*> *solution_queue;
};

long int get_file_size (FILE* file_stream);

void* read_file_thrd (void* arg);

void read_file (string file_name);

void* file_recv_listener ();

pthread_t create_file_recv_listener_thrd ();

struct Sudoku_puzzle_desc* get_sudoku_puzzle_from_queue (queue <struct Sudoku_puzzle_desc*> puzzle_queue);

void* sudoku_solve_thrd (void *arg);

struct Sudoku_solution_desc* sudoku_solve (struct Sudoku_puzzle_desc* puzzle_desc);

pthread_t create_sudoku_solve_thrd ();

long int get_puzzle_counter (long int& counter);

struct Sudoku_puzzle_desc* get_next_puzzle_from_buf (FILE *file_stream, char *start, char *current, char *end, int puzzle_size);

void update_current_ptr (char *current, char *end);

char* get_end_ptr (struct File_stream_desc* stream_desc);

void assign_sudoku_puzzle (struct File_stream_desc *stream_desc);

void* assign_sudoku_puzzle_thrd (void *arg);

struct Sudoku_queues *init_sudoku_queue (struct Sudoku_queues *queues);

pthread_t create_assign_sudoku_puzzle_thrd ();

void insert_solution_to_map (struct Sudoku_solution_desc *ssd);

void traverse_solution_queue (queue<struct Sudoku_solution_desc*> sq);

bool solution_queues_empty (vector<queue<struct Sudoku_solution_desc*>*> *qv);
void waitting_file_read (FILE *file_stream, char *start, char *current, char *end);

void merge_sudoku_solution_thrd ();

pthread_t create_merge_sudoku_solution_thrd ();

void update_current_ptr (char **current);

void ignore_line_break (char **current);

void start_thrd (_FUNC_TYPE FT, _MODE_TYPE MT);

pthread_t get_thrd_id (vector<pthread_t>::iterator iter);

void sudoku_thrd_join ();

void thrd_exit_arbitrate ();
void set_puzzles_assign_done_flag (bool flag);
bool get_puzzles_assign_done_flag ();
bool is_basic_mode ();
void init_type (_FUNC_TYPE FT, _MODE_TYPE MT);
void add_sorted_solution_to_queue ();
void print_consumed_time ();
void waitting_for_cv_lk (int thrd_type);
void update_next_print_counter ();
bool get_merge_thrd_exit_flag ();
void set_merge_thrd_exit_flag (pthread_t thrd_type, bool flag);
void print_single_solution (int solution[]);
void set_allow_file_read_thrd_exit_flag (bool flag);
bool get_allow_file_read_thrd_exit_flag ();

#endif

