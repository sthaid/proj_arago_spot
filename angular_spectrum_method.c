// xxx this file
// - ASSERT N IS EVEN
// - do fft in place
// - does negative z make a difference
// - use fftw_malloc ?
// - better name for apply0

// xxx other
// - simplify graphcis interface ?
// - zoom in and out 

// title: angular_spectrum_method

// references:
//  https://rafael-fuente.github.io/simulating-diffraction-patterns-with-the-angular-spectrum-method-and-python.html

//  --------------------------------------------------------------

#include "common.h"

static fftw_plan   fwd;
static fftw_plan   back;

// ---- UNIT TEST --------------------

void init_rh(void);
void init_ring(void);
void init_ss(void);

int main(int argc, char **argv)
{
    asm_init();
    init_ring();
    asm_execute(532e-9, 2.0);
    display_init(false);
    display_hndlr();

    return 0;
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
                buff[x][y] = 1;
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
                buff[x][y] = 1;
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
                buff[x][y] = 1;
            }
        }
    }
}

// -------------------- CODE --------------

static void apply(double wavelen, double z);

int asm_init(void)
{
    printf("N          = %d\n", N);
    printf("TOTAL_SIZE = %0.3f mm\n", TOTAL_SIZE * 1000);
    printf("ELEM_SIZE  = %0.3f mm\n", ELEM_SIZE * 1000);

    // init 2d fft plans for inplace forward and backward fft
    fwd  = fftw_plan_dft_2d(N, N, (complex*)buff, (complex*)buff, FFTW_FORWARD, FFTW_ESTIMATE);
    back = fftw_plan_dft_2d(N, N, (complex*)buff, (complex*)buff, FFTW_BACKWARD, FFTW_ESTIMATE);

    return 0;
}

void asm_execute(double wavelen, double z)
{
    fftw_execute(fwd);
    apply(wavelen, z);
    fftw_execute(back);
}

static void apply(double wavelen, double z)
{
    double kx, ky, kz;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            kx = (2*M_PI/TOTAL_SIZE) * (i <= N/2-1 ? i : i-N);
            ky = (2*M_PI/TOTAL_SIZE) * (j <= N/2-1 ? j : j-N);
            kz = csqrt( square(2*M_PI/wavelen) -
                        square(kx) -
                        square(ky) );
            buff[i][j] = buff[i][j] * cexp(I * kz * z);
        }
    }
}
