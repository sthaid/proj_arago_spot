#include "common.h"

//
// typedefs
//
 
enum aperature_id {APERTURE_ID_RH, APERTURE_ID_SS, APERTURE_ID_RING};

typedef struct {
    enum aperature_id id;
    union {
        struct {
            double diameter_mm;
        } rh;
        struct {
            double width_mm;
            double height_mm;
        } ss;
        struct {
            double id_mm;
            double od_mm;
        } ring;
    };
} aperture_t;

//
// variables
//

static aperture_t aperture[100];
static int max_aperture;

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
        if (strcmp(name, "rh") == 0) x->id = APERTURE_ID_RH;
        else if (strcmp(name, "ss") == 0) x->id = APERTURE_ID_SS;
        else if (strcmp(name, "ring") == 0) x->id = APERTURE_ID_RING;
        else goto error;

        switch (x->id) {
        case APERTURE_ID_RH:
            if (sscanf(attributes, "%lf", &x->rh.diameter_mm) != 1) goto error;
            INFO("rh:   diameter_mm=%0.3lf\n", x->rh.diameter_mm);
            break;
        case APERTURE_ID_SS:
            if (sscanf(attributes, "%lf,%lf", &x->ss.width_mm, &x->ss.height_mm) != 2) goto error;
            INFO("ss:   width_mm=%0.3lf  height_mm=%0.3lf\n", x->ss.width_mm, x->ss.height_mm);
            break;
        case APERTURE_ID_RING:
            if (sscanf(attributes, "%lf,%lf", &x->ring.id_mm, &x->ring.od_mm) != 2) goto error;
            INFO("ring: id_mm=%0.3lf  od_mm=%0.3lf\n", x->ring.id_mm, x->ring.od_mm);
            break;
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

    switch (x->id) {
    case APERTURE_ID_RH:
        rh_init(x->rh.diameter_mm/1000);
        break;
    case APERTURE_ID_SS:
        ss_init(x->ss.width_mm/1000, x->ss.height_mm/1000);
        break;
    case APERTURE_ID_RING:
        ring_init(x->ring.id_mm/1000, x->ring.od_mm/1000);
        break;
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

