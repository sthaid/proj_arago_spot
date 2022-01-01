#include "common.h"

//
// prototypes
//

static void rh_init(double diameter_mm);
static void ring_init(double id, double od);
static void ss_init(double width, double height);

// -----------------  APERTURE API ROUTINES  ---------------------------------

int aperture_init(void)
{
    FILE *fp;
    int linenum=0;
    char s[200], name[100], attributes[100];

    fp = fopen("aperture_defs", "r");
    if (fp == NULL) {
        ERROR("failed to open aperture_defs, %s\n", strerror(errno));
        return -1;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        linenum++;

        s[strcspn(s, "\n")] = 0;
        if (s[0] == '#' || strspn(s, " ") == strlen(s)) {
            continue;
        }

        if (sscanf(s, "%s%s", name, attributes) != 2) {
            goto error;
        }

        aperture_t *x = &aperture[max_aperture];

        strcpy(x->name, name);
        if (strcmp(name, "rh") == 0) {
            if (sscanf(attributes, "%lf", &x->rh.diameter_mm) != 1) goto error;
            INFO("rh:   diameter_mm=%0.3lf\n", x->rh.diameter_mm);
        } else if (strcmp(name, "ss") == 0) {
            if (sscanf(attributes, "%lf,%lf", &x->ss.width_mm, &x->ss.height_mm) != 2) goto error;
            INFO("ss:   width_mm=%0.3lf  height_mm=%0.3lf\n", x->ss.width_mm, x->ss.height_mm);
        } else if (strcmp(name, "ring") == 0) {
            if (sscanf(attributes, "%lf,%lf", &x->ring.id_mm, &x->ring.od_mm) != 2) goto error;
            INFO("ring: id_mm=%0.3lf  od_mm=%0.3lf\n", x->ring.id_mm, x->ring.od_mm);
        } else {
            goto error;
        }

        max_aperture++;
    }

    fclose(fp);
    return 0;

error:
    fclose(fp);
    ERROR("line %d invalid\n", linenum);
    return -1;
}

void aperture_select(int idx)
{
    assert(idx >= 0 && idx < max_aperture);
    aperture_t *x = &aperture[idx];

    memset(buff, 0, sizeof(buff));

    if (strcmp(x->name, "rh") == 0) {
        rh_init(x->rh.diameter_mm/1000);
    } else if (strcmp(x->name, "ss") == 0) {
        ss_init(x->ss.width_mm/1000, x->ss.height_mm/1000);
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
                buff[x][y] = 1;
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
                buff[x][y] = 1;
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
            if (fabs(y-y_ctr)*ELEM_SIZE < width/2 &&
                fabs(x-x_ctr)*ELEM_SIZE < height/2)
            {
                buff[x][y] = 1;
            }
        }
    }
}

