#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <complex.h>
#include <fftw3.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

#include <util_misc.h>
#include <util_sdl.h>

int display_init(bool swap_white_black);
void display_hndlr(void);

static inline double square(double x) { return x*x; }

#define N           5000
#define TOTAL_SIZE  .050
#define ELEM_SIZE   (TOTAL_SIZE/N)

complex buff[N][N];

int asm_init(void);
void asm_execute(double wavelen, double z);



void init_aperture_rh(void);
void init_aperture_ring(void);
void init_aperture_ss(void);
