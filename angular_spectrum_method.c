// xxx this file
// - ASSERT N IS EVEN
// - do fft in place
// - does negative z make a difference

// xxx other
// - simplify graphcis interface ?
// - zoom in and out 

// title: angular_spectrum_method

// references:
//  https://rafael-fuente.github.io/simulating-diffraction-patterns-with-the-angular-spectrum-method-and-python.html

//  --------------------------------------------------------------

#include "common.h"

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#define N     5000
#define  ELEM_SIZE   (.050/N)
#define WAVELEN 532e-9  // green  ??
#define Z (2.)  // meters

complex  in[N][N];
//complex   out[N][N];
fftw_plan   plan_fwd;
fftw_plan   plan_back;

void apply(void);
void init_rh(void);
void init_ss(void);
void init_ring(void);

inline double square(double x) { return x*x; }

int main(int argc, char **argv)
{
    //int x,y;

    printf("elem size = %lf mm\n", ELEM_SIZE*1000);
    printf("screen width = %lf mm\n", N*ELEM_SIZE*1000);

    // init in array
    init_ring();

    // init 2d fft
    plan_fwd  = fftw_plan_dft_2d(N, N, (complex*)in, (complex*)in, FFTW_FORWARD, FFTW_ESTIMATE);
    plan_back = fftw_plan_dft_2d(N, N, (complex*)in, (complex*)in, FFTW_BACKWARD, FFTW_ESTIMATE);

    // run fft forward
    fftw_execute(plan_fwd);

    apply();

    // run fft backward
    fftw_execute(plan_back);

    display_init(false);
    display_hndlr();
}

void apply(void)
{
    int i,j;
    double Lx, Ly, kz;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {

            Lx = N * ELEM_SIZE / 2;
            Ly = N * ELEM_SIZE / 2;
            double kx = (M_PI/Lx) * (i <= N/2-1 ? i : i-N);
            double ky = (M_PI/Ly) * (j <= N/2-1 ? j : j-N);
            kz = csqrt(
                   square(2*M_PI/WAVELEN) -
                   square(kx) -
                   square(ky));
            in[i][j] = in[i][j] * cexp(I * kz * Z);
        }
    }
}

#define MAX_SCREEN_AMP N
void sim_get_screen(double screen[MAX_SCREEN][MAX_SCREEN])
{
    const int scale_factor = MAX_SCREEN_AMP / MAX_SCREEN;

    // using the screen_amp1, screen_amp2, and scale_factor as input,
    // compute the return screen buffer intensity values;
    for (int i = 0; i < MAX_SCREEN; i++) {
        for (int j = 0; j < MAX_SCREEN; j++) {
            double sum = 0;
            for (int ii = i*scale_factor; ii < (i+1)*scale_factor; ii++) {
                for (int jj = j*scale_factor; jj < (j+1)*scale_factor; jj++) {
                    sum += square(creal(in[ii][jj])) + square(cimag(in[ii][jj]));
                }
            }
            screen[i][j] = sum;
        }
    }

    // determine max_screen_value
    double max_screen_value = -1;
    for (int i = 0; i < MAX_SCREEN; i++) {
        for (int j = 0; j < MAX_SCREEN; j++) {
            if (screen[i][j] > max_screen_value) {
                max_screen_value = screen[i][j];
            }
        }
    }
    DEBUG("max_screen_value %g\n", max_screen_value);

    // normalize screen values to range 0..1
    if (max_screen_value) {
        double max_screen_value_recipricol = 1 / max_screen_value;
        for (int i = 0; i < MAX_SCREEN; i++) {
            for (int j = 0; j < MAX_SCREEN; j++) {
                screen[i][j] *= max_screen_value_recipricol;
            }
        }
    }
}

void init_rh(void)
{
    int x,y;

    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;
    const double diameter = 0.30e-3;  // .15 mm
    //const double diameter = 50e-3;  // 50 mm

    for (x = 0; x < N; x++) {
        for (y = 0; y < N; y++) {
            if (square((x-x_ctr)*ELEM_SIZE) + square((y-y_ctr)*ELEM_SIZE) <= square(diameter/2)) {
                in[x][y] = 1;
            }
        }
    }
}

void init_ring(void)
{
    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;
    double id = 6.35e-3;
    double od = 30e-3;
    //double id = 25e-3;
    //double od = 50e-3;

    double tmp;
    int x,y;

    for (x = 0; x < N; x++) {
        for (y = 0; y < N; y++) {
            tmp = square((x-x_ctr)*ELEM_SIZE) + square((y-y_ctr)*ELEM_SIZE);
            if (tmp >= square(id/2) && tmp <= square(od/2)) {
                in[x][y] = 1;
            }
        }
    }
}

void init_ss(void)
{
    int x,y;

    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;
    //const double height = 45e-3;  // 45mm
    //const double width = 10e-3;  // 10mm
    const double height = 50e-3;  // 50mm
    const double width = 0.15e-3;  // 0.15mm

    for (x = 0; x < N; x++) {
        for (y = 0; y < N; y++) {
            if (fabs(y-y_ctr)*ELEM_SIZE < width/2 &&
                fabs(x-x_ctr)*ELEM_SIZE < height/2)
            {
                in[x][y] = 1;
            }
        }
    }
}

