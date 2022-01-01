#include "common.h"

void init_aperture_rh(void)
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

void init_aperture_ring(void)
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

void init_aperture_ss(void)
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

