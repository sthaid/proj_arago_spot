#include "common.h"


int main(int argc, char **argv)
{
    asm_init();
    init_aperture_ring();
    asm_execute(532e-9, 2.0);
    display_init(false);
    display_hndlr();

    return 0;
}

