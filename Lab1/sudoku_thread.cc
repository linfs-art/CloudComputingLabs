#include <queue>
#include <map>
#include <vector>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <stdio.h>
#include <condition_variable>
#include <algorithm>
#include <pthread.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <chrono>

#include "sudoku_thread.h"
#include "sudoku_basic.h"
#include "sudoku_dancing_links.h"
#include "sudoku_min_arity.h"
#include "sudoku_min_arity_cache.h"

using namespace std;

vector<queue<struct Sudoku_puzzle_desc*>*> sudoku_puzzle_queue_vec;
vector<queue<struct Sudoku_solution_desc*>*> sudoku_solution_queue_vec;
queue<struct File_stream_desc*> file_stream_desc_queue;
map<long int, struct Sudoku_solution_desc*> solution_map;
queue<char*> solution_queue;

map<string, struct Cv_lk*> cv_map = 
{
	{"assign_thrd", 	  new Cv_lk ()},
	{"sudoku_solve_thrd", new Cv_lk ()},
	{"merge_thrd", 	 	  new Cv_lk ()},
	{"print_thrd", 		  new Cv_lk ()},
};

map<pthread_t, struct Cv_lk*> pt_cv_map;
map<pthread_t, struct Cv_lk*> sst_cv_map;

_FUNC_TYPE FUNC_TYPE; 
_MODE_TYPE MODE_TYPE;

long int puzzle_counter = 1;
long int next_print_counter = 1;
long int total_solved_puzzle = 0;
int solve_nprocs = 18;
int core_num = 0;

int exit_sudoku_thrd_num = 0;
bool puzzles_assign_done_flag = false;
bool solution_queue_add_done_flag = false;
bool showing_terminal_on = false;
bool allow_assign_thrd_exit_flag = false;

vector<pthread_t> print_thrd_id_vec;
pthread_mutex_t next_print_counter_mux;
pthread_mutex_t puzzle_counter_mux;
pthread_mutex_t terminal_mux;

double start_time;
double end_time;

/* microseconds */
double now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void set_start_time ()
{
	start_time = now ();
}

void set_end_time ()
{
	end_time = now ();
}

void print_consumed_time ()   
{
	/* seconds */
	double total_time = (end_time - start_time) / 1000000.0;

	cout << "Total Sudoku puzzles solved: " << puzzle_counter - 1 << "; total consumed time: " << total_time << " seconds; each consumed time: " << total_time / (puzzle_counter - 1)  << " seconds." << endl;  
}

pthread_mutex_t core_num_mux;
void update_core_num ()
{
	pthread_mutex_lock (&core_num_mux);
	core_num ++;
	pthread_mutex_unlock (&core_num_mux);
}

void thrd_bind_core (pthread_t thread, int ith_core)
{
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(ith_core, &cpuset);
	if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
	{
		cout << "thread bind core error!" << endl;
		exit (0);
	}
	
	update_core_num ();
	return;
}

void init_type (_FUNC_TYPE FT, _MODE_TYPE MT)
{
	FUNC_TYPE = FT;
	MODE_TYPE = MT;
}

void set_allow_assign_thrd_exit_flag (bool flag)
{
	allow_assign_thrd_exit_flag = flag;
}

bool get_allow_assign_thrd_exit_flag ()
{
	return allow_assign_thrd_exit_flag;
}

void notify_thrd_all (int thrd_type)
{
	map<pthread_t, struct Cv_lk*> *cv_map;

	if (thrd_type == SUDOKU_SOLVE_THRD)
		cv_map = &sst_cv_map;
	if (thrd_type == PRINT_THRD)
		cv_map = &pt_cv_map;

	for (map<pthread_t, struct Cv_lk*>::iterator iter = cv_map->begin (); iter != cv_map->end (); iter++)
	{
		iter->second->cv.notify_one ();
	}
}

pthread_mutex_t exit_mux;

void update_exit_sudoku_thrd_num ()
{
	pthread_mutex_lock (&exit_mux);
	exit_sudoku_thrd_num ++;
	pthread_mutex_unlock (&exit_mux);
}

pthread_mutex_t tsp_mux;

void update_total_solved_puzzle ()
{
	pthread_mutex_lock (&tsp_mux);
	total_solved_puzzle++;
	pthread_mutex_unlock (&tsp_mux);
}

bool merge_thrd_exit_content_b ()
{
	bool flag = false;

	if (exit_sudoku_thrd_num == solve_nprocs)
	{
		if ((solution_queues_empty (&sudoku_solution_queue_vec) && solution_map.empty ()) && (next_print_counter - 1 == total_solved_puzzle))
			flag = true;
	}

	return flag;
}

bool sudoku_solve_thrd_exit_content_b ()
{
	bool flag = false;

	if (get_puzzles_assign_done_flag ())
	{
		update_exit_sudoku_thrd_num ();
		flag = true;
	}

	return flag;
}

bool print_thrd_exit_content ()
{
	bool flag = false;

	if ((solution_map.empty () && get_merge_thrd_exit_flag ()) && (next_print_counter - 1 == total_solved_puzzle))
		flag = true;

	return flag;
}

bool sudoku_puzzle_assign_thrd_exit_content ()
{
	bool flag = false;

	if (file_stream_desc_queue.empty () && get_allow_assign_thrd_exit_flag ())
	{
		flag = true;
	}

	return flag;
}

bool thrd_exit_content (pthread_t thrd_type)
{
	bool flag = false;

	switch (thrd_type)
	{
		case FILE_READ_THRD:
			flag = get_allow_file_read_thrd_exit_flag ();
			break;
		case PUZZLE_ASSIGN_THRD:
			flag = sudoku_puzzle_assign_thrd_exit_content ();
			break;
		case SUDOKU_SOLVE_THRD:
			flag = sudoku_solve_thrd_exit_content_b ();
			break;
		case MERGE_THRD:
			flag = merge_thrd_exit_content_b ();
			break;
		case PRINT_THRD:
			flag = print_thrd_exit_content ();
			break;
	}

	return flag;
}

void notify_merge_thrd ()
{
	cv_map["merge_thrd"]->cv.notify_all ();
}

void log_thrd_exit (pthread_t thrd_type)
{
	switch (thrd_type)
	{
		case SUDOKU_SOLVE_THRD:
			cout << "sudoku solve thread exit !" << endl;
			break;
		case MERGE_THRD:
			cout << "merge thread exit !" << endl;
			break;
		case PRINT_THRD:
			cout << "print thread exit !" << endl;
			break;
		case PUZZLE_ASSIGN_THRD:
			cout << "puzzle assign thread exit !" << endl;
	}
}

bool merge_thrd_exit_flag = false;

bool set_merge_thrd_exit_flag (bool flag)
{
	merge_thrd_exit_flag = flag;
}

bool get_merge_thrd_exit_flag ()
{
	return merge_thrd_exit_flag;
}

void set_merge_thrd_exit_flag (pthread_t thrd_type, bool flag)
{
	if (thrd_type == MERGE_THRD)
	{
		set_merge_thrd_exit_flag (true);
	}
}

void thrd_exit_arbitrate (pthread_t thrd_type)
{
	if (!is_basic_mode ())
		return;

	if (thrd_exit_content (thrd_type))
	{
		// log_thrd_exit (thrd_type);
		pthread_exit (NULL);
	}
}

void set_solution_queue_add_done_flag (bool flag)
{
	solution_queue_add_done_flag = flag;
}

bool get_solution_queue_add_done_flag ()
{
	return solution_queue_add_done_flag;
}

void set_puzzles_assign_done_flag (bool flag)
{
	puzzles_assign_done_flag = flag;
	
}

bool allow_file_read_thrd_exit_flag;
void set_allow_file_read_thrd_exit_flag (bool flag)
{
	allow_file_read_thrd_exit_flag = flag;
}

bool get_allow_file_read_thrd_exit_flag ()
{
	return allow_file_read_thrd_exit_flag;
}

bool get_puzzles_assign_done_flag ()
{
	return puzzles_assign_done_flag;
}

long int get_file_size (FILE* file_stream)
{
	long int file_size = 0;

	fseek (file_stream, 0L, SEEK_END);
	file_size = ftell (file_stream);
	fseek (file_stream, 0L, SEEK_SET);

	return file_size;
}

bool file_exist (string file_name)
{
	bool flag = false;

	if (!access (&file_name[0], 0))
		flag = true;

	return flag;
}

void* read_file_thrd (void* arg)   //string*
{
	string* file_name = (string*) arg;
	read_file (*file_name);
}

void read_file (string file_name)
{
	if (!file_exist (file_name))
	{
		cout << "file :" << file_name << " does not exist!" << endl;
		return;
	}
	// cout << "file name: " << file_name << endl;
	FILE* file_stream = NULL;
	struct File_stream_desc* file_stream_desc = new File_stream_desc (); 

	file_stream_desc_queue.push (file_stream_desc);     //store

	file_stream = fopen (&file_name [0], "rb");
	file_stream_desc->file_stream = file_stream;
	file_stream_desc->file_size = get_file_size (file_stream);

	file_stream_desc->p_buf = (char*) malloc (file_stream_desc->file_size);       //wait to delete

	cv_map ["assign_thrd"]->cv.notify_one ();                                 //I am going to read file, please be ready!
	// cout << "read file start..." << endl;
	fread (file_stream_desc->p_buf, file_stream_desc->file_size, 1, file_stream);
	set_allow_file_read_thrd_exit_flag (true);
	set_allow_assign_thrd_exit_flag (true);
	// cout << "read file over..." << endl;
}

bool is_basic_mode ()
{
	bool flag = false;

	if (MODE_TYPE == BASIC_MODE)
	{
		flag = true;
	}

	return flag;
}

void* file_recv_listener (void* arg)
{
	pthread_t thread;
	string *file_name = new string ();
	
	// while (true)
	// {
		std::cin >> (*file_name);
		set_start_time ();
		int ret = pthread_create (&thread, NULL, read_file_thrd, (void*)file_name);
		assert (ret == 0);

		// thrd_bind_core (thread, core_num % get_nprocs_conf ());
		thrd_exit_arbitrate (FILE_READ_THRD);
	// }
}

pthread_t create_file_recv_listener_thrd ()
{
		pthread_t thread;
		int ret = pthread_create (&thread, NULL, file_recv_listener, NULL);
		if (ret != 0)
		{
			cout << "create file_recv_listener error!" << endl;
		}
		assert (ret == 0);

		return thread;
}

struct Sudoku_queues *init_sudoku_queues (struct Sudoku_queues *queues)
{
	queues = new Sudoku_queues ();
	queues->puzzle_queue = new queue<struct Sudoku_puzzle_desc*> ();
	queues->solution_queue = new queue<struct Sudoku_solution_desc*> ();
	return queues;
}

struct Sudoku_puzzle_desc* get_sudoku_puzzle_from_queue (queue <struct Sudoku_puzzle_desc*> *puzzle_queue)
{
	struct Sudoku_puzzle_desc* puzzle_desc = new struct Sudoku_puzzle_desc ();

	if (!puzzle_queue->empty ())
	{
		memcpy (puzzle_desc, puzzle_queue->front (), sizeof (struct Sudoku_puzzle_desc));
		puzzle_queue->pop ();

		return puzzle_desc;
	}
	else 
	{

		return NULL;
	}
}

void tune_up_file_pcur (FILE *fs, int ith_puzzle)
{
	int offset = 0;

	offset = (ith_puzzle - 1) * (N + 1);
	fseek (fs, offset, SEEK_SET);
}

void int_to_char_arr (int int_arr[], char char_arr[], int size)
{
	for (int i = 0; i < size; i++)
	{
		char_arr[i] = int_arr[i] + '0';
	}
}

void write_solution_to_file (FILE *fs, struct Sudoku_solution_desc *ssd)
{
	char arr[N+1];

	arr[N] = '\n';
	tune_up_file_pcur (fs, ssd->ith_puzzle);
	int_to_char_arr (ssd->sudoku_solution, arr, N);
	fwrite (arr, sizeof (char) * (N + 1), 1, fs);
}

struct Cv_lk *new_cv_lk ()
{
	struct Cv_lk * cv_lk = new Cv_lk ();

	return cv_lk;
}

void add_cv_lk_to_map (int thrd_type, pthread_t thread)
{
	if (thrd_type == SUDOKU_SOLVE_THRD)
		sst_cv_map.insert (std::make_pair (thread, new_cv_lk ()));
	if (thrd_type == PRINT_THRD)
		pt_cv_map.insert (std::make_pair (thread, new_cv_lk ()));
}

void* sudoku_solve_thrd (void *arg)
{
	struct Sudoku_queues *sudoku_queues = (struct Sudoku_queues*) arg;
	struct Sudoku_solution_desc* solution_desc = NULL;
	struct Sudoku_puzzle_desc* puzzle_desc = NULL;
	FILE *fs = fopen ("./sudoku_solution.out", "rb+");

	add_cv_lk_to_map (SUDOKU_SOLVE_THRD, pthread_self ());
	while (true)
	{
		unique_lock<std::mutex> lk (sst_cv_map [pthread_self ()]->cv_mtx);
		while(sst_cv_map [pthread_self ()]->cv.wait_for (lk, std::chrono::milliseconds(100)) == std::cv_status::timeout)
		{	
			while (puzzle_desc = get_sudoku_puzzle_from_queue (sudoku_queues->puzzle_queue))
			{
				solution_desc = sudoku_solve (puzzle_desc);
				// write_solution_to_file (fs, solution_desc);
				update_total_solved_puzzle ();
		 	}
		thrd_exit_arbitrate (SUDOKU_SOLVE_THRD);
		}	
	}
}

void get_basic_solution (char *puzzle, int *solution)
{
	Basic b (puzzle);
	b.basic_solve (solution);
}

void get_dl_solution (char *puzzle, int *solution)
{
	Dance dl (puzzle);
	dl.dance_solve (solution);
}

void get_mina_solution (char *puzzle, int *solution)
{
	Mina mina (puzzle);
	mina.mina_solve (solution);
}

void get_minac_solution (char *puzzle, int *solution)
{
	Minac minac (puzzle);
	minac.minac_solve (solution);
}

void get_sudoku_solution (char *puzzle, int *solution)
{
	switch (FUNC_TYPE)
	{
		case BASIC:
			get_basic_solution (puzzle, solution);
			break;
		case DANCE:
			get_dl_solution (puzzle, solution);
			break;
		case MINA:
			get_mina_solution (puzzle, solution);
			break;
		case MINAC:
			get_minac_solution (puzzle, solution);
			break;
	}
}

struct Sudoku_solution_desc* sudoku_solve (struct Sudoku_puzzle_desc* puzzle_desc)
{
	struct Sudoku_solution_desc* sudoku_solution_desc = new Sudoku_solution_desc ();

	sudoku_solution_desc->ith_puzzle = puzzle_desc->ith_puzzle;
	get_sudoku_solution (puzzle_desc->sudoku_puzzle, sudoku_solution_desc->sudoku_solution);

	return sudoku_solution_desc;
}

void waitting_for_cv_lk (int thrd_type)
{
	while (true)
	{
		if (thrd_type == SUDOKU_SOLVE_THRD)
			if (sst_cv_map.find (pthread_self ()) != sst_cv_map.end ())
				return;
	}

}

pthread_t create_sudoku_solve_thrd ()
{
	pthread_t thread;
	struct Sudoku_queues *sudoku_queues = NULL;

	sudoku_queues = init_sudoku_queues (sudoku_queues);
	sudoku_puzzle_queue_vec.push_back (sudoku_queues->puzzle_queue);
	sudoku_solution_queue_vec.push_back (sudoku_queues->solution_queue);

	int ret = pthread_create (&thread, NULL, sudoku_solve_thrd, (void*) sudoku_queues);

	thrd_bind_core (thread, core_num % get_nprocs_conf ());

	assert (ret == 0);

	return thread;
}

long int get_puzzle_counter ()
{
	return puzzle_counter;
}

void update_puzzle_counter ()
{
	pthread_mutex_unlock (&puzzle_counter_mux);
	puzzle_counter++;
	pthread_mutex_unlock (&puzzle_counter_mux);
}

void waitting_file_read (FILE *file_stream, char *start, char *current, char *end)
{
	while ((current - start + 81) >= ftell (file_stream))
	{
		cout << "waitting read file..." << endl;
		continue;
	}
}

void update_current_ptr (char **current)
{
	*current = *current + N - 1;
}

void ignore_line_break (char **current)
{
	*current = *current + 2;
}

struct Sudoku_puzzle_desc* get_next_puzzle_from_buf (FILE *file_stream, char *start, char **current, char *end, int puzzle_size)
{
	if (*current == end || (*current + 1) == end)
		return NULL;
	struct Sudoku_puzzle_desc* puzzle = new Sudoku_puzzle_desc ();
	waitting_file_read (file_stream, start, *current, end);

	if (*current != start)
	{
		ignore_line_break (current);
	}
	memcpy ((puzzle->sudoku_puzzle), *current, N);
	puzzle->ith_puzzle = get_puzzle_counter ();
	update_puzzle_counter ();

	return puzzle;
}

char* get_end_ptr (struct File_stream_desc* stream_desc)
{
	char* end = NULL;

	end = stream_desc->p_buf + (stream_desc->file_size - 1);

	return end;
}
// int i = 0;
queue<struct Sudoku_puzzle_desc*>* get_next_assign_queue (vector<queue<struct Sudoku_puzzle_desc*>*> *q_vec)
{
	vector<queue<struct Sudoku_puzzle_desc*>*>::iterator iter = q_vec->begin ();
	iter = iter + (puzzle_counter % solve_nprocs);

	return (*iter);
}

void waitting_solve_thrd_created ()
{
	while (sudoku_puzzle_queue_vec.size () != solve_nprocs)
		;
}

void assign_sudoku_puzzle (struct File_stream_desc *stream_desc)
{
	struct Sudoku_puzzle_desc* puzzle = NULL;
	queue<struct Sudoku_puzzle_desc*> *puzzle_queue = NULL;
	FILE *file_stream = stream_desc->file_stream;
	char *start = stream_desc->p_buf;
	char *current = start;
	char *end = start + (stream_desc->file_size - 1);
	
	waitting_solve_thrd_created ();
	while (puzzle = get_next_puzzle_from_buf (file_stream, start, &current, end, 81))
	{
		
		// random_shuffle (sudoku_puzzle_queue_vec.begin (), sudoku_puzzle_queue_vec.end ());
		// puzzle_queue = sudoku_puzzle_queue_vec.front ();
		puzzle_queue = get_next_assign_queue (&sudoku_puzzle_queue_vec);
		puzzle_queue->push (puzzle);

		update_current_ptr (&current);
		// notify_thrd_all (SUDOKU_SOLVE_THRD);        //performance worse after adding, why?
	}
	set_puzzles_assign_done_flag (true);
}

void* assign_sudoku_puzzle_thrd (void *arg)
{
	struct File_stream_desc *stream_desc = NULL;

	while (true)
	{
		unique_lock<std::mutex> lk (cv_map ["assign_thrd"]->cv_mtx);
		while (cv_map ["assign_thrd"]->cv.wait_for (lk, std::chrono::milliseconds (1)) == std::cv_status::timeout)
		{
			while (!file_stream_desc_queue.empty ())
			{
				stream_desc = file_stream_desc_queue.front ();
				assign_sudoku_puzzle (stream_desc);
				delete stream_desc;             //delete
				file_stream_desc_queue.pop ();
			}
			
			thrd_exit_arbitrate (PUZZLE_ASSIGN_THRD);
		}
	}
}

pthread_t create_assign_sudoku_puzzle_thrd ()
{
	pthread_t thread;

	int ret = pthread_create (&thread, NULL, assign_sudoku_puzzle_thrd, NULL);
	assert (ret == 0);

	// thrd_bind_core (thread, core_num % get_nprocs_conf ());
	return thread;
}

void insert_solution_to_map (struct Sudoku_solution_desc *ssd)
{
	solution_map.insert (std::make_pair(ssd->ith_puzzle, ssd));
}

bool solution_queues_empty (vector<queue<struct Sudoku_solution_desc*>*> *qv)
{
	vector<queue<struct Sudoku_solution_desc*>*>::iterator q_iter;
	bool empty_flag = true;

	for (q_iter = qv->begin (); q_iter != qv->end (); q_iter ++)
	{
		if (!(*q_iter)->empty ())
		{
			return empty_flag = false;
		}
	}
	return empty_flag;
}

void open_showing_terminal ()
{
	pthread_mutex_lock (&terminal_mux);
	if (!showing_terminal_on)
	{
		system ("gnome-terminal --title='Hey, this is a terminal for showing the sudoku solution!' -- sh -c 'tail -f ./sudoku_solution.out; exex bash'");
		cout << "Ready for displaying results. Please enter file name at another terminal and wait......" << endl;
		showing_terminal_on = true;
	}
	pthread_mutex_unlock(&terminal_mux);
}

void redirect_stdout ()
{
	freopen ("./sudoku_solution.out", "w", stdout);
}

vector<pthread_t> join_thrd;

void start_thrd (_FUNC_TYPE FT, _MODE_TYPE MT)
{	
	int nprocs = solve_nprocs;

	if (nprocs == 0)
		nprocs = get_nprocs_conf ();

	init_type (FT, MT);
	// redirect_stdout ();
	// open_showing_terminal ();

	create_file_recv_listener_thrd ();
	while (nprocs--)
	{
		join_thrd.push_back(create_sudoku_solve_thrd ());
	}

	create_assign_sudoku_puzzle_thrd ();

}

void sudoku_thrd_join ()
{
	for (vector<pthread_t>::iterator iter = join_thrd.begin (); iter != join_thrd.end (); iter++)
	{
		pthread_join (*iter, NULL);
	}

	set_end_time ();
}