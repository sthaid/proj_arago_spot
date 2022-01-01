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

#include <util_misc.h>
#include <util_sdl.h>

#define N           5000
#define TOTAL_SIZE  .050
#define ELEM_SIZE   (TOTAL_SIZE/N)

// fft buffer
complex buff[N][N];

// display.c
int display_init(bool swap_white_black);
void display_hndlr(void);

// aperture.c
int aperture_init(void);
void aperture_select(int idx);

// angular_spectrum_method.c
int angular_spectrum_method_init(void);
void angular_spectrum_method_execute(double wavelen, double z);

// inline helpers
static inline double square(double x) { return x*x; }

