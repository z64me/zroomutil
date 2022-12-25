/*
 * main.c <z64.me>
 *
 * zroomutil's entry point
 *
 */

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "model.h"

#define PROGNAME "zroomutil"

static void showargs(void)
{
#define ARG "  "
	Log("arguments (order matters; each argument is a command):");
	Log(ARG "--import file.zroom - imports a room file");
	Log(ARG "                      (when used multiple times, rooms are concatenated)");
	Log(ARG "--flatten - merges all groups into one");
	Log(ARG "--wavefront out.obj - exports the result to Wavefront model file");
	Log(ARG "--zroom out.zroom - exports the result to zroom model file");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	struct room *room = 0;
	
	Log("welcome to " PROGNAME);
	
	if (argc < 2)
		showargs();
	
	for (int i = 1; i < argc; ++i)
	{
		const char *a = argv[i];
		const char *next = argv[i + 1];
		
		if (!strcmp(a, "--import"))
		{
			struct room *tmp = room_load(next);
			
			if (room)
				room_merge(room, tmp);
			else
				room = tmp;
			
			++i;
		}
		else if (!strcmp(a, "--wavefront"))
		{
			room_writeWavefront(room, next);
			++i;
		}
		else if (!strcmp(a, "--zroom"))
		{
			room_writeZroom(room, next, true);
			++i;
		}
		else if (!strcmp(a, "--flatten"))
		{
			room_flatten(room);
		}
	}
	
	if (room)
		room_free(room);
	
	return 0;
}
