#include "common.h"


int main(int argc, char **argv)
{
    if (aperture_init() < 0) return 1;
    if (angular_spectrum_method_init() < 0) return 1;

    display_init(true);
    display_hndlr();

    return 0;
}

