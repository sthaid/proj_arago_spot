#include "common.h"

//
// prototypes
//

static void rh_init(double diameter_mm);
static void ring_init(double id, double od);
static void ss_init(double width, double height);
static void ds_init(double width, double height, double separation);

// -----------------  APERTURE API ROUTINES  ---------------------------------

int aperture_init(void)
{
    FILE *fp;
    int linenum=0;
    char s[200], name[100], attributes[100];
    int intensity_algorithm;

    // open aperture definition file
    fp = fopen("aperture_defs", "r");
    if (fp == NULL) {
        ERROR("failed to open aperture_defs, %s\n", strerror(errno));
        return -1;
    }

    // read lines from file
    while (fgets(s, sizeof(s), fp) != NULL) {
        linenum++;

        // remove trailing newline char, and
        // if line is comment or blank then continue
        s[strcspn(s, "\n")] = 0;
        if (s[0] == '#' || strspn(s, " ") == strlen(s)) {
            continue;
        }

        // read the aperature definition name (rh,ring,ss,ds) and attributes
        if (sscanf(s, "%s %d %s", name, &intensity_algorithm, attributes) != 3) {
            goto error;
        }

        // store name and intensity_algorithm in aperture array
        aperture_t *x = &aperture[max_aperture];
        strcpy(x->name, name);
        x->intensity_algorithm = intensity_algorithm;

        // based on name, scan the attributes and store their values in the aperture array
        if (strcmp(name, "rh") == 0) {
            if (sscanf(attributes, "%lf", &x->rh.diameter_mm) != 1) goto error;
            sprintf(x->full_name, "rh__%0.3lf", x->rh.diameter_mm);
        } else if (strcmp(name, "ss") == 0) {
            if (sscanf(attributes, "%lf,%lf", &x->ss.width_mm, &x->ss.height_mm) != 2) goto error;
            sprintf(x->full_name, "ss__%0.3lf__%0.3lf", x->ss.width_mm, x->ss.height_mm);
        } else if (strcmp(name, "ds") == 0) {
            if (sscanf(attributes, "%lf,%lf,%lf", &x->ds.width_mm, &x->ds.height_mm, &x->ds.separation_mm) != 3) goto error;
            sprintf(x->full_name, "ds__%0.3lf__%0.3lf__%0.3lf", x->ss.width_mm, x->ss.height_mm, x->ds.separation_mm);
        } else if (strcmp(name, "ring") == 0) {
            if (sscanf(attributes, "%lf,%lf", &x->ring.id_mm, &x->ring.od_mm) != 2) goto error;
            sprintf(x->full_name, "ring__%0.3lf__%0.3lf", x->ring.id_mm, x->ring.od_mm);
        } else {
            goto error;
        }
        INFO("%-4s - %s\n", x->name, x->full_name);

        // keep track of the number of apertures defined
        max_aperture++;
    }

    // if no apertures defined then error
    if (max_aperture == 0) goto error;

    // close file and return success
    fclose(fp);
    return 0;

error:
    // close file and return error
    fclose(fp);
    ERROR("line %d invalid\n", linenum);
    return -1;
}

void aperture_select(int idx)
{
    // this routine calls one of the private routines, defined below,
    // which will init the fft buff for the selected aperture

    assert(idx >= 0 && idx < max_aperture);
    aperture_t *x = &aperture[idx];

    memset(buff, 0, sizeof(buff));

    if (strcmp(x->name, "rh") == 0) {
        rh_init(x->rh.diameter_mm/1000);
    } else if (strcmp(x->name, "ss") == 0) {
        ss_init(x->ss.width_mm/1000, x->ss.height_mm/1000);
    } else if (strcmp(x->name, "ds") == 0) {
        ds_init(x->ds.width_mm/1000, x->ds.height_mm/1000, x->ds.separation_mm/1000);
    } else if (strcmp(x->name, "ring") == 0) {
        ring_init(x->ring.id_mm/1000, x->ring.od_mm/1000);
    } else {
        FATAL("invalid idx %d\n", idx);
    }
}

// -----------------  PRIVATE ROUTINES  --------------------------------------

static void rh_init(double diameter)
{
    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            if (square((x-x_ctr)*ELEM_SIZE) + square((y-y_ctr)*ELEM_SIZE) <= square(diameter/2)) {
                buff[y][x] = 1;
            }
        }
    }
}

static void ring_init(double id, double od)
{
    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            double tmp = square((x-x_ctr)*ELEM_SIZE) + square((y-y_ctr)*ELEM_SIZE);
            if (tmp >= square(id/2) && tmp <= square(od/2)) {
                buff[y][x] = 1;
            }
        }
    }
}

static void ss_init(double width, double height)
{
    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            if (fabs(y-y_ctr)*ELEM_SIZE < height/2 &&
                fabs(x-x_ctr)*ELEM_SIZE < width/2)
            {
                buff[y][x] = 1;
            }
        }
    }
}

static void ds_init(double width, double height, double separation)
{
    double x_ctr = (N-1)/2.;
    double y_ctr = (N-1)/2.;

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            if (fabs(y-y_ctr)*ELEM_SIZE < height/2 &&
                fabs(x-x_ctr)*ELEM_SIZE > (separation-width)/2 &&
                fabs(x-x_ctr)*ELEM_SIZE < (separation+width)/2)
            {
                buff[y][x] = 1;
            }
        }
    }
}
