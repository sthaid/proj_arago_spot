#include "common.h"


int main(int argc, char **argv)
{
    if (aperture_init() < 0) return 1;
    if (angular_spectrum_method_init() < 0) return 1;

    aperture_select(2);
    angular_spectrum_method_execute(532e-9, 2.0);

    display_init(false);
    display_hndlr();

    return 0;
}

