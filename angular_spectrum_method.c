// References:
// - https://en.wikipedia.org/wiki/Angular_spectrum_method
// - https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.674.3630&rep=rep1&type=pdf
// - https://rafael-fuente.github.io/simulating-diffraction-patterns-with-the-angular-spectrum-method-and-python.html
// - https://github.com/rafael-fuente/diffractsim
// - http://www.fftw.org/fftw3.pdf

#include "common.h"

// the calculation of kx and ky in multiply_by_propagation_term assumes N is even
_Static_assert((N & 1) == 0);

static fftw_plan   fwd;
static fftw_plan   back;

// -------------------- CODE ------------------------------

static void multiply_by_propagation_term(double wavelen, double z);

int angular_spectrum_method_init(void)
{
    // print values of defines
    printf("N          = %d\n", N);
    printf("TOTAL_SIZE = %0.3f mm\n", TOTAL_SIZE * 1000);
    printf("ELEM_SIZE  = %0.3f mm\n", ELEM_SIZE * 1000);

    // init 2d fft plans for inplace forward and backward fft
    fwd  = fftw_plan_dft_2d(N, N, (complex*)buff, (complex*)buff, FFTW_FORWARD, FFTW_ESTIMATE);
    back = fftw_plan_dft_2d(N, N, (complex*)buff, (complex*)buff, FFTW_BACKWARD, FFTW_ESTIMATE);

    // return success
    return 0;
}

void angular_spectrum_method_execute(double wavelen, double z)
{
    fftw_execute(fwd);
    multiply_by_propagation_term(wavelen, z);
    fftw_execute(back);
}

static void multiply_by_propagation_term(double wavelen, double z)
{
    double kx, ky, kz;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            kx = (2*M_PI/TOTAL_SIZE) * (i <= N/2-1 ? i : i-N);
            ky = (2*M_PI/TOTAL_SIZE) * (j <= N/2-1 ? j : j-N);
            kz = csqrt( square(2*M_PI/wavelen) - square(kx) - square(ky) );
            buff[i][j] = buff[i][j] * cexp(I * kz * z);
        }
    }
}
