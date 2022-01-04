#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <complex.h>
#include <fftw3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <util_misc.h>
#include <util_sdl.h>

// fft buffer
#define N           5000
#define TOTAL_SIZE  .050            // size of screen in meters
#define ELEM_SIZE   (TOTAL_SIZE/N)  // size of single element, in meters
complex buff[N][N];

// display.c
int display_init(bool swap_white_black);
void display_hndlr(void);

// aperture.c
#define MAX_APERTURE 100

typedef struct {
    char name[20];
    char full_name[100];
    union {
        struct {
            double diameter_mm;
        } rh;
        struct {
            double width_mm;
            double height_mm;
        } ss;
        struct {
            double width_mm;
            double height_mm;
            double separation_mm;
        } ds;
        struct {
            double id_mm;
            double od_mm;
        } ring;
    };
} aperture_t;

aperture_t aperture[MAX_APERTURE];
int max_aperture;

int aperture_init(void);
void aperture_select(int idx);

// angular_spectrum_method.c
int angular_spectrum_method_init(void);
void angular_spectrum_method_execute(double wavelen, double z);

// inline helpers
static inline double square(double x) { return x*x; }

