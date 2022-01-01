#include "common.h"

//
// defines
//

#define LARGE_FONT 24
#define SMALL_FONT 16 

#define MAX_INTENSITY_ALGORITHM  2  // xxx?

#define MAX_SCREEN 500
#define SCREEN_ELEMENT_SIZE  (1000 * TOTAL_SIZE / MAX_SCREEN)   // xxx mm

// xxx dont' display until cpmputedone
//
// variables
//

static int               win_width;
static int               win_height;

static int               intensity_algorithm = 0;

static double            sensor_width  = .1;   // mm
static double            sensor_height = .1;   // mm
static bool              sensor_lines_enabled = false;

static int               current_aperture_idx; // xxx init
static bool              compute_done;

//
// prototypes
//

static int interference_pattern_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static int control_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static void *compute_thread(void *cx);
static double get_sensor_value(int screen_idx, double screen[MAX_SCREEN][MAX_SCREEN]);  // xxx move
static void get_screen(double screen[MAX_SCREEN][MAX_SCREEN]);  // xxx move

// -----------------  DISPLAY_INIT  ---------------------------------------------

int display_init(bool swap_white_black)
{
    pthread_t tid;

    #define REQUESTED_WIN_WIDTH  800
    #define REQUESTED_WIN_HEIGHT 625

    // init sdl, and get actual window width and height
    win_width  = REQUESTED_WIN_WIDTH;
    win_height = REQUESTED_WIN_HEIGHT;
    if (sdl_init(&win_width, &win_height, false, false, swap_white_black) < 0) {
        FATAL("sdl_init %dx%d failed\n", win_width, win_height);
    }
    INFO("REQUESTED win_width=%d win_height=%d\n", REQUESTED_WIN_WIDTH, REQUESTED_WIN_HEIGHT);
    INFO("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    //  xxx when init make sure there is at least one
    current_aperture_idx = 0;
    pthread_create(&tid, NULL, compute_thread, NULL);

    // return success
    return 0;
}

// -----------------  DISPLAY_HANDLER  ------------------------------------------

void display_hndlr(void)
{
    int interference_pattern_pane_width, interference_pattern_pane_height;
    int control_pane_width, control_pane_height;

    // init pane sizing variables
    interference_pattern_pane_width = MAX_SCREEN + 4;
    interference_pattern_pane_height = (MAX_SCREEN+121) + 4;
    control_pane_width = win_width - interference_pattern_pane_width;
    control_pane_height = interference_pattern_pane_height;

    // call the pane manger; 
    // - this will not return except when it is time to terminate the program
    // - the interferometer_diagram_pane_hndlr and interference_pattern_graph_pane_hndlr
    //   panes intentionally have the same display location; these can be switched 
    //   betweeen by right clicking
    sdl_pane_manager(
        NULL,           // context
        NULL,           // called prior to pane handlers
        NULL,           // called after pane handlers
        100000,         // 0=continuous, -1=never, else us
        2,              // number of pane handler varargs that follow
        interference_pattern_pane_hndlr, NULL, 
            0, 0, 
            interference_pattern_pane_width, interference_pattern_pane_height,
            PANE_BORDER_STYLE_MINIMAL,
        control_pane_hndlr, NULL, 
            interference_pattern_pane_width, 0,
            control_pane_width, control_pane_height,
            PANE_BORDER_STYLE_MINIMAL
        );
}

// -----------------  INTERFEROMETER PATTERN PANE HANDLER  ----------------------

static void render_interference_screen(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], 
                texture_t texture, unsigned int pixels[MAX_SCREEN][MAX_SCREEN], 
                rect_t *pane);
static void render_intensity_graph(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], rect_t *pane);
static void render_scale(
                int y_top, int y_span, rect_t *pane);

static int interference_pattern_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        texture_t texture;
        unsigned int pixels[MAX_SCREEN][MAX_SCREEN];
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

   #define SDL_EVENT_SENSOR_WIDTH         (SDL_EVENT_USER_DEFINED + 0)
   #define SDL_EVENT_SENSOR_HEIGHT        (SDL_EVENT_USER_DEFINED + 1)
   #define SDL_EVENT_SENSOR_LINES         (SDL_EVENT_USER_DEFINED + 2)
   #define SDL_EVENT_INTENSITY_ALGORITHM  (SDL_EVENT_USER_DEFINED + 3)

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        vars->texture = sdl_create_texture(MAX_SCREEN, MAX_SCREEN);
        INFO("PANE x,y,w,h  %d %d %d %d\n",
            pane->x, pane->y, pane->w, pane->h);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        double screen[MAX_SCREEN][MAX_SCREEN];
        char   sensor_width_str[20], sensor_height_str[20];
        char   intensity_algorithm_str[20];

        // xxx
        // this pane is vertically arranged as follows
        // y-range  y-span  description 
        // -------  ------  -----------
        // 0-499    500     interference screen
        // 500-500  1       horizontal line
        // 501-600  100     intensity graph
        // 601-620  20      scale in mm
        //          ----
        //          621

        // get the screen data
        memset(screen, 0, sizeof(screen));
        if (compute_done) {
            get_screen(screen);
        }

        // render the sections that make up this pane, as described above
        render_interference_screen(0, MAX_SCREEN, screen, vars->texture, vars->pixels, pane);
        sdl_render_line(pane, 0,MAX_SCREEN, MAX_SCREEN-1,MAX_SCREEN, SDL_WHITE);
        render_intensity_graph(MAX_SCREEN+1, 100, screen, pane);
        render_scale(MAX_SCREEN+101, 20, pane);

        if (!compute_done) {
            sdl_render_printf(pane, 
                MAX_SCREEN/2 - COL2X(7,LARGE_FONT)/2, MAX_SCREEN/2 - ROW2Y(1,LARGE_FONT)/2,
                LARGE_FONT, SDL_WHITE, SDL_BLACK, "WORKING");
        }
            

        // xxx move all controls to ctrl pane
        // register events
        sprintf(sensor_width_str, "W=%3.1f", sensor_width);
        sdl_render_text_and_register_event(
                pane, 0, MAX_SCREEN+1, LARGE_FONT,
                sensor_width_str, SDL_LIGHT_BLUE, SDL_BLACK,
                SDL_EVENT_SENSOR_WIDTH, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        sprintf(sensor_height_str, "H=%3.1f", sensor_height);
        sdl_render_text_and_register_event(
                pane, COL2X(6,LARGE_FONT), MAX_SCREEN+1, LARGE_FONT,
                sensor_height_str, SDL_LIGHT_BLUE, SDL_BLACK,
                SDL_EVENT_SENSOR_HEIGHT, SDL_EVENT_TYPE_MOUSE_WHEEL, pane_cx);

        sdl_render_text_and_register_event(
                pane, MAX_SCREEN-COL2X(4,LARGE_FONT), MAX_SCREEN+1, LARGE_FONT,
                "SENS", SDL_LIGHT_BLUE, SDL_BLACK,
                SDL_EVENT_SENSOR_LINES, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        // xxx 
        sprintf(intensity_algorithm_str, "ALG=%d", intensity_algorithm);
        sdl_render_text_and_register_event(
                pane, pane->w-COL2X(5,LARGE_FONT), 0, LARGE_FONT,
                intensity_algorithm_str, SDL_LIGHT_BLUE, SDL_BLACK,
                SDL_EVENT_INTENSITY_ALGORITHM, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

// xxx scaling

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_SENSOR_WIDTH:
            if (event->mouse_wheel.delta_y > 0) sensor_width += .1;
            if (event->mouse_wheel.delta_y < 0) sensor_width -= .1;
            if (sensor_width < 0.1) sensor_width = 0.1;
            if (sensor_width > 5.0) sensor_width = 5.0;
            break;
        case SDL_EVENT_SENSOR_HEIGHT:
            if (event->mouse_wheel.delta_y > 0) sensor_height += .1;
            if (event->mouse_wheel.delta_y < 0) sensor_height -= .1;
            if (sensor_height < 0.1) sensor_height = 0.1;
            if (sensor_height > 5.0) sensor_height = 5.0;
            break;
        case SDL_EVENT_SENSOR_LINES:
            sensor_lines_enabled = !sensor_lines_enabled;
            break;
        case SDL_EVENT_INTENSITY_ALGORITHM:
            intensity_algorithm = (intensity_algorithm + 1) % MAX_INTENSITY_ALGORITHM;
            break;
        }
        return PANE_HANDLER_RET_DISPLAY_REDRAW;
    }

    // ---------------------------
    // -------- TERMINATE --------
    // ---------------------------

    if (request == PANE_HANDLER_REQ_TERMINATE) {
        free(vars);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // not reached
    assert(0);
    return PANE_HANDLER_RET_NO_ACTION;
}

// xxx simpler names
static void render_interference_screen(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], 
                texture_t texture, unsigned int pixels[MAX_SCREEN][MAX_SCREEN], 
                rect_t *pane)
{
    int i,j;
    int texture_width, texture_height;

    // initialize pixels from screen data
    for (i = 0; i < MAX_SCREEN; i++) {
        for (j = 0; j < MAX_SCREEN; j++) {
            int green;

            // this translates the calculated screen intensity (which has
            // been normalized to range 0 .. 1 by the sim_get_scren routine)
            // to a pixel intensity; two algorithm choices are provided:
            // - 0: logarithmic  (default)
            // - 1: linear
            if (intensity_algorithm == 0) {
                green = ( screen[i][j] == 0 
                          ? 0 
                          : 255.99 * (1 + 0.2 * log(screen[i][j])) );
                if (green < 0) green = 0;
            } else if (intensity_algorithm == 1) {
                green = 255.99 * screen[i][j];
            } else {
                FATAL("invalid intensity_algorithm %d\n", intensity_algorithm);
            }
            if (green < 0 || green > 255) {
                ERROR("green %d\n", green);
                green = (green < 0 ? 0 : 255);
            }

            // set the display pixel values 
            pixels[i][j] = (0xff << 24) | (green << 8);
        }
    }

    // update the texture with the new pixel values
    sdl_update_texture(texture, (unsigned char*)pixels, MAX_SCREEN*sizeof(int));

    // render
    sdl_query_texture(texture, &texture_width, &texture_height);
    assert(y_span == texture_height);
    sdl_render_texture(pane, 0, y_top, texture);
}

static void render_intensity_graph(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], rect_t *pane)
{
    point_t graph[MAX_SCREEN];
    int     screen_idx;
    int     y_bottom = y_top + y_span - 1;

    // draw graph of sensor_value
    for (screen_idx = 0; screen_idx < MAX_SCREEN; screen_idx++) {
        graph[screen_idx].x = screen_idx;
        graph[screen_idx].y = y_bottom - get_sensor_value(screen_idx,screen) * y_span;
    }
    sdl_render_lines(pane, graph, MAX_SCREEN, SDL_WHITE);

    // draw horizontal lines accross middle to represent the top and bottom of the sensor
    if (sensor_lines_enabled) {
        int sensor_height_pixels = sensor_height / SCREEN_ELEMENT_SIZE;
        int sensor_min_y = MAX_SCREEN/2 - sensor_height_pixels/2;
        int sensor_max_y = sensor_min_y + sensor_height_pixels - 1;

        sensor_min_y = MAX_SCREEN/2 - sensor_height_pixels/2;
        sensor_max_y = sensor_min_y + sensor_height_pixels - 1;
        sdl_render_line(pane, 0, sensor_min_y, MAX_SCREEN-1, sensor_min_y, SDL_WHITE);
        sdl_render_line(pane, 0, sensor_max_y, MAX_SCREEN-1, sensor_max_y, SDL_WHITE);
    }
}

static void render_scale(int y_top, int y_span, rect_t *pane)
{
    double tick_intvl, tick_loc;
    char tick_value_str[20];
    int tick_value, tick_value_loc, slen;
    char screen_diam_str[50];

    // convert 5mm to tick_intvl in pixel units
    tick_intvl = 5 / SCREEN_ELEMENT_SIZE;

    // starting at the center, adjust tick_loc to the left to 
    // find the first tick location
    tick_loc = MAX_SCREEN / 2;
    while (tick_loc >= tick_intvl) {
        tick_loc -= tick_intvl;
    }

    // render the ticks and the tick value strings
    for (; tick_loc < MAX_SCREEN; tick_loc += tick_intvl) {
        // if tick location is too close to either side then skip it
        if (tick_loc < 5 || tick_loc >= MAX_SCREEN-5) continue;

        // render the tick
        sdl_render_line(pane, nearbyint(tick_loc), y_top, nearbyint(tick_loc), y_top+5, SDL_WHITE);

        // determine the tick value (in mm), and display it centered below the tick
        // - determine the tick_value based on the difference between tick_loc and the center
        tick_value = nearbyint((tick_loc - MAX_SCREEN/2) * SCREEN_ELEMENT_SIZE);
        sprintf(tick_value_str, "%d", tick_value);
        // - determine the tick_value_loc, accounting for the string length of the tick_value_str
        slen = strlen(tick_value_str);
        if (tick_value < 0) slen++;
        tick_value_loc = nearbyint(tick_loc - COL2X(slen,SMALL_FONT) / 2.);
        // - render the tick value
        sdl_render_text(pane, 
                        tick_value_loc, y_top+5,
                        SMALL_FONT, tick_value_str, SDL_WHITE, SDL_BLACK);
    }

    // render the axis line
    sdl_render_line(pane, 0,y_top, MAX_SCREEN-1,y_top, SDL_WHITE);

    // display the screen diameter at the top of the pane
    sprintf(screen_diam_str, "SCREEN DIAMETER %g mm", MAX_SCREEN*SCREEN_ELEMENT_SIZE);
    sdl_render_text(pane, 
                    (pane->w-COL2X(strlen(screen_diam_str),LARGE_FONT))/2, 0, 
                    LARGE_FONT, screen_diam_str, SDL_WHITE, SDL_BLACK);
}

// -----------------  CONTROL PANE HANDLER  --------------------------------------

static int control_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

   #define SDL_EVENT_SIM_CFGSEL  (SDL_EVENT_USER_DEFINED + 10)

    // ----------------------------
    // -------- INITIALIZE --------
    // ----------------------------

    if (request == PANE_HANDLER_REQ_INITIALIZE) {
        vars = pane_cx->vars = calloc(1,sizeof(*vars));
        INFO("PANE x,y,w,h  %d %d %d %d\n",
            pane->x, pane->y, pane->w, pane->h);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // ------------------------
    // -------- RENDER --------
    // ------------------------

    if (request == PANE_HANDLER_REQ_RENDER) {
        int row, i;

        // register for events ...

        // - config select
        for (row = 0, i = 0; i < max_aperture; i++, row++) {
            sdl_render_text_and_register_event(
                    pane, 0, ROW2Y(row,LARGE_FONT), LARGE_FONT,
                    aperture[i].name, 
                    current_aperture_idx == i ? SDL_GREEN : SDL_LIGHT_BLUE, SDL_BLACK,
                    SDL_EVENT_SIM_CFGSEL+i, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);
        }

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_SIM_CFGSEL ... SDL_EVENT_SIM_CFGSEL+MAX_APERTURE-1: {
            pthread_t tid;

            current_aperture_idx = event->event_id - SDL_EVENT_SIM_CFGSEL;
            compute_done = false;
            pthread_create(&tid, NULL, compute_thread,NULL);
//AAA xxx
            break; }
        }

        return PANE_HANDLER_RET_DISPLAY_REDRAW;
    }

    // ---------------------------
    // -------- TERMINATE --------
    // ---------------------------

    if (request == PANE_HANDLER_REQ_TERMINATE) {
        free(vars);
        return PANE_HANDLER_RET_NO_ACTION;
    }

    // not reached
    assert(0);
    return PANE_HANDLER_RET_NO_ACTION;
}

// xxx create detached
static void *compute_thread(void *cx)
{
    aperture_select(current_aperture_idx);
    angular_spectrum_method_execute(532e-9, 2.0);  // xxx need to also vary wavlen and z

    compute_done = true;

    return NULL;
}

// -----------------  SUPPORT ROUTINES  -----------------------------------------

static double get_sensor_value(int screen_idx, double screen[MAX_SCREEN][MAX_SCREEN])
{
    int sensor_min_x, sensor_max_x, sensor_min_y, sensor_max_y;
    int sensor_width_screen_elements, sensor_height_screen_elements;
    double sum, sensor_value;
    int i, j, cnt;

    // convert caller's sensor dimensions (in mm) to screen-elements
    sensor_width_screen_elements = sensor_width / SCREEN_ELEMENT_SIZE;
    sensor_height_screen_elements = sensor_height / SCREEN_ELEMENT_SIZE;

    // determine sensor location for indexing into the screen array;
    // these min/max values are used to compute an average sensor_value
    // for different size sensors
    sensor_min_x = screen_idx - sensor_width_screen_elements/2;
    sensor_max_x = sensor_min_x + sensor_width_screen_elements - 1;
    sensor_min_y = MAX_SCREEN/2 - sensor_height_screen_elements/2;
    sensor_max_y = sensor_min_y + sensor_height_screen_elements - 1;

    // if the sensor min_x or max_x is off the screen then 
    // limit the min_x or max_x value to the screen boundary
    if (sensor_min_x < 0) sensor_min_x = 0;
    if (sensor_max_x > MAX_SCREEN-1) sensor_max_x = MAX_SCREEN-1;

    // compute the average sensor value
    sum = 0;
    cnt = 0;
    for (j = sensor_min_y; j <= sensor_max_y; j++) {
        for (i = sensor_min_x; i <= sensor_max_x; i++) {
            sum += screen[j][i];
            cnt++;
        }
    }
    sensor_value = cnt > 0 ? sum/cnt : 0;

    // return the average sensor_value
    return sensor_value;
}

static void get_screen(double screen[MAX_SCREEN][MAX_SCREEN])
{
    const int scale_factor = N / MAX_SCREEN;

    // using the screen_amp1, screen_amp2, and scale_factor as input,
    // compute the return screen buffer intensity values;
    for (int i = 0; i < MAX_SCREEN; i++) {
        for (int j = 0; j < MAX_SCREEN; j++) {
            double sum = 0;
            for (int ii = i*scale_factor; ii < (i+1)*scale_factor; ii++) {
                for (int jj = j*scale_factor; jj < (j+1)*scale_factor; jj++) {
                    sum += square(creal(buff[ii][jj])) + square(cimag(buff[ii][jj]));
                }
            }
            screen[i][j] = sum;
        }
    }

    // determine max_screen_value
    double max_screen_value = -1;
    for (int i = 0; i < MAX_SCREEN; i++) {
        for (int j = 0; j < MAX_SCREEN; j++) {
            if (screen[i][j] > max_screen_value) {
                max_screen_value = screen[i][j];
            }
        }
    }
    DEBUG("max_screen_value %g\n", max_screen_value);

    // normalize screen values to range 0..1
    if (max_screen_value) {
        double max_screen_value_recipricol = 1 / max_screen_value;
        for (int i = 0; i < MAX_SCREEN; i++) {
            for (int j = 0; j < MAX_SCREEN; j++) {
                screen[i][j] *= max_screen_value_recipricol;
            }
        }
    }
}

