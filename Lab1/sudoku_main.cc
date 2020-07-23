#include <queue>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "sudoku_thread.h"

using namespace std;

/* switch solving algorithm, option:BASIC/DANCE/MINA/MINAC */
_FUNC_TYPE DEFAULT_FUNC_TYPE = DANCE;       


int main ()
{
	//Cut the cackle and come to the horses. Let's go!!!
	start_thrd (DEFAULT_FUNC_TYPE);
	sudoku_thrd_join ();

	/* BASIC_MODE only */
	print_consumed_time ();
	
	return 0;
}