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
//  --------------------------------------------------------------
//  --------------------------------------------------------------

#include "common.h"

static fftw_plan   fwd;
static fftw_plan   back;

// -------------------- CODE ------------------------------

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
    apply(wavelen, z);  // xxx better name
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
