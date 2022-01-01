// ASSERT N IS EVEN

// angular_spectrum_method


// xxx wavelen needs to be 500
// xxx do fft in place
// xxx does negative z make a difference
// xxx close compare to this
//        https://rafael-fuente.github.io/simulating-diffraction-patterns-with-the-angular-spectrum-method-and-python.html

// simplify graphcis interface ?
// zoom in and out 
// why is rh not symetric


//  --------------------------------------------------------------


#if 0
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#endif

#include "common.h"

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#define N     5000
#define  ELEM_SIZE   (.050/N)
#define WAVELEN 532e-9  // green  ??
#define Z (2.)  // meters
//#define Z 0.5 // meters

complex  in[N][N];
complex   out[N][N];
fftw_plan   plan_fwd;
fftw_plan   plan_back;

void print_array(char *name, complex array[][N]);
void apply(void);
void init_rh(void);
void init_ss(void);
void init_ring(void);

void shift(void);

//void plot(void);

inline double square(double x) { return x*x; }

int main(int argc, char **argv)
{
    //int x,y;

    printf("elem size = %lf mm\n", ELEM_SIZE*1000);
    printf("screen width = %lf mm\n", N*ELEM_SIZE*1000);

    // init in array
    init_ring();
#if 0
#if 1
    int ctr=N/4;
    for (x = ctr-5; x <= ctr+5; x++) {
        for (y = ctr-5; y <= ctr+5; y++) {
            in[x][y] = 1;
        }
    }
#else
    in[CTR][CTR] = 1;
#endif
    //print_array("in", in);
#endif

    // init 2d fft
    plan_fwd  = fftw_plan_dft_2d(N, N, (complex*)in, (complex*)out, FFTW_FORWARD, FFTW_ESTIMATE);
    plan_back = fftw_plan_dft_2d(N, N, (complex*)in, (complex*)out, FFTW_BACKWARD, FFTW_ESTIMATE);

    // run fft forward
    fftw_execute(plan_fwd);

#if 0
    int i,j;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            printf("%10.6lf  ", creal(out[i][j]));
        }
        printf("\n");
    }
#endif

    // copy out to in, and 
    // apply propagation phase term
    //shift();
    memcpy(in, out, sizeof(out));
    apply();
    // xxx

    // run fft backward
    //shift();
    fftw_execute(plan_back);

    // print result
    //print_array("out", out);
    //for (x = 0; x < N; x++) {
        //printf("%d = %lf\n", x, cabs(out[x][CTR]));
    //}
    //plot();

    display_init(false);
    display_hndlr();
}

void print_array(char *name, complex array[][N])
{
    int x, y;

    printf("ARRAY %s\n", name);
    for (x = 0; x < N; x++) {
        for (y = 0; y < N; y++) {
            complex v = array[x][y];
            if (cabs(v) > 1e-6) {
                printf("%d,%d = %lf + %lf I\n", x, y, creal(v), cimag(v));
            }
        }
    }
    printf("\n");
}

#if 0
void apply(void)
{
    int x, y;

    for (x = 0; x < N; x++) {
        for (y = 0; y < N; y++) {
            #define XXX   (N/2)
#if 0
            double thetax, thetay, vx, vy;
            thetax = ((x-XXX)*ELEM_SIZE) / Z;
            thetay = ((y-XXX)*ELEM_SIZE) / Z;
            vx = sin(thetax) / WAVELEN;
            vy = sin(thetay) / WAVELEN;
#else
            double vx, vy;
            //vx = (x-XXX)*ELEM_SIZE / sqrt(square((x-XXX)*ELEM_SIZE) + square(Z))   / WAVELEN;
            //vy = (y-XXX)*ELEM_SIZE / sqrt(square((y-XXX)*ELEM_SIZE) + square(Z))   / WAVELEN;
            vx = (x-XXX)*ELEM_SIZE / Z / WAVELEN;
            vy = (y-XXX)*ELEM_SIZE / Z / WAVELEN;
#endif

            in[x][y] = in[x][y] *
                cexp(I * Z * (2 * M_PI) * sqrt(1/square(WAVELEN) - square(vx) - square(vy)));
        }
    }
}
#else
#define XXX   (N/2)
void apply(void)
{
#if 0
    int i, j;
    double vx, vy;

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            vx = (i-XXX) * (ELEM_SIZE / (WAVELEN * Z));
            vy = (j-XXX) * (ELEM_SIZE / (WAVELEN * Z));
            in[i][j] = in[i][j] * 
                cexp(I * Z * (2 * M_PI) * sqrt(1/square(WAVELEN) - square(vx) - square(vy)));
        }
    }
#endif
    int i,j;
    double Lx, Ly, kz;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {

            Lx = N * ELEM_SIZE / 2;
            Ly = N * ELEM_SIZE / 2;
#if 0
            kz = sqrt(
                   square(2*M_PI/WAVELEN) -
                   square(M_PI*i/Lx) -
                   square(M_PI*j/Ly));
#else
            double kx = (M_PI/Lx) * (i <= N/2-1 ? i : i-N);
            double ky = (M_PI/Ly) * (j <= N/2-1 ? j : j-N);
            //double kx = (i <= N/2-1 ?  (M_PI/Lx) * i :
                                       //(M_PI/Lx) * (i-N));
            //double ky = (j <= N/2-1 ?  (M_PI/Ly) * j :
                                       //(M_PI/Ly) * (j-N));
            kz = csqrt(
                   square(2*M_PI/WAVELEN) -
                   square(kx) -
                   square(ky));

            //kz = csqrt(
                   //square(2*M_PI/WAVELEN) -
                   //square(M_PI*(i-N/2)/Lx) -
                   //square(M_PI*(j-N/2)/Ly));
#endif

            in[i][j] = in[i][j] * cexp(I * kz * Z);
        }
    }
#endif
}

#if 0
void plot(void)
{
    double maxval = -1;
    double value[N];
    int x, num_stars;
    char stars[1000];

    for (x = 0; x < N; x++) {
        value[x] = cabs(out[x][CTR/2]);
        if (value[x] > maxval) maxval = value[x];
    }

    for (x = 0; x < N; x++) {
        num_stars = value[x] * 100 / maxval;
        memset(stars, '*', num_stars);
        stars[num_stars] = 0;

        printf("%4d %10lf - %s\n", x, value[x], stars);
    }


    double maxavgval;
    double avg[10000];
    int i,j;
    #define MAXAVG  (N/100)
    memset(avg,0,sizeof(avg));

    for (i = 0; i < 100; i++) {
        for (j = 0; j < MAXAVG; j++) {
            avg[i] += value[i*MAXAVG+j];
        }
    }
    maxavgval = 0;
    for (i = 0; i < 100; i++) {
        avg[i] /= MAXAVG;
        if (avg[i] > maxavgval) maxavgval = avg[i];
    }

    for (i = 0; i < 100; i++) {
        num_stars = avg[i] * 100 / maxavgval;
        memset(stars, '*', num_stars);
        stars[num_stars] = 0;

        printf("%3d : %s\n", i, stars);
    }
    
}
#endif

complex conjugate(complex x)
{
    return creal(x) - I*cimag(x);
}

#define MAX_SCREEN_AMP N
int display_algorithm = 0;
void sim_get_screen(double screen[MAX_SCREEN][MAX_SCREEN])
{
    const int scale_factor = MAX_SCREEN_AMP / MAX_SCREEN;

    // display_algorithm 0 ...
    // 
    // using the screen_amp1, screen_amp2, and scale_factor as input,
    // compute the return screen buffer intensity values;
    for (int i = 0; i < MAX_SCREEN; i++) {
        for (int j = 0; j < MAX_SCREEN; j++) {
            double sum = 0;
            for (int ii = i*scale_factor; ii < (i+1)*scale_factor; ii++) {
                for (int jj = j*scale_factor; jj < (j+1)*scale_factor; jj++) {
                    //sum += square(screen_amp1[ii][jj]) + square(screen_amp2[ii][jj]);
                    //sum += (cabs(out[ii][jj]));
                    sum += creal(   out[ii][jj] * conjugate(out[ii][jj])  );
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
        double max_screen_value_recipricol = 
                (display_algorithm == 0 ? 1 : 0.7) / max_screen_value;
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

inline void swap(complex *a, complex *b)
{
    complex tmp = *a;
    *a = *b;  
    *b = tmp;
}

void shift(void)
{
    int i,j;
    // swap first quadrant with third
    // swap second quadrant with foruth
    for (i = 0; i < N/2; i++) {
        for (j = 0; j < N/2; j++) {
            swap(&out[i][j],  &out[i+N/2][j+N/2]);
            swap(&out[i+N/2][j],  &out[i][j+N/2]);
        }
    }

}
