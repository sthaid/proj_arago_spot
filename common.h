#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <util_misc.h>
#include <util_sdl.h>

#define MAX_SCREEN 500
#define SCREEN_ELEMENT_SIZE  0.1   // m

int display_init(bool swap_white_black);
void display_hndlr(void);


void sim_get_screen(double screen[MAX_SCREEN][MAX_SCREEN]);
