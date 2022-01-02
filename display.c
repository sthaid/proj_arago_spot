// xxx use define for compute_completed

#include "common.h"

//
// defines
//

#define LARGE_FONT 24
#define SMALL_FONT 16 

#define MAX_INTENSITY_ALGORITHM  2  // xxx?

#define MAX_SCREEN 500
#define SCREEN_ELEMENT_SIZE  (1000 * TOTAL_SIZE / MAX_SCREEN)   // xxx mm

#define MIN_Z_MM     0     // xxx comment or assert requirements
#define MAX_Z_MM     3000
#define DELTA_Z_MM   100

#define MIN_WAVLEN_NM   400     // xxx comment or assert requirements
#define MAX_WAVLEN_NM   750
#define DELTA_WAVLEN_NM 25

//
// variables
//

static int      win_width;
static int      win_height;

static int      intensity_algorithm = 0;

static double   sensor_width  = .1;   // mm  xxx name
static double   sensor_height = .1;   // mm
static bool     sensor_lines_enabled = false;

static double   screen[MAX_SCREEN][MAX_SCREEN];
static int      aperture_idx;
static int      wavelen_nm;
static int      z_mm;
static bool     compute_in_progress;
static int      auto_init_state;

//
// prototypes
//

static int interference_pattern_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static int control_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event);
static void common_key_controls(int key);
static void run_compute_thread(int requested_aperture_idx, int requested_wavelen_nm, int requested_z_mm);
static void auto_init(void *cx);

// -----------------  DISPLAY_INIT  ---------------------------------------------

int display_init(bool swap_white_black)
{
    #define REQUESTED_WIN_WIDTH  800
    #define REQUESTED_WIN_HEIGHT 625

    // init sdl, and get actual window width and height
    win_width  = REQUESTED_WIN_WIDTH;
    win_height = REQUESTED_WIN_HEIGHT;
    if (sdl_init(&win_width, &win_height, false, false, swap_white_black) < 0) {
        ERROR("sdl_init %dx%d failed\n", win_width, win_height);
        return -1;
    }
    INFO("REQUESTED win_width=%d win_height=%d\n", REQUESTED_WIN_WIDTH, REQUESTED_WIN_HEIGHT);
    INFO("ACTUAL    win_width=%d win_height=%d\n", win_width, win_height);

    //  xxx comment
    run_compute_thread(0, 525, 2000); // xxx defines

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
        auto_init,      // called after pane handlers
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

static void render_screen(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], int wavelen_nm,
                texture_t texture, unsigned int pixels[MAX_SCREEN][MAX_SCREEN], 
                rect_t *pane);
static void render_intensity_graph(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], rect_t *pane);
static double get_sensor_value(int screen_idx, double screen[MAX_SCREEN][MAX_SCREEN]);

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
        char   sensor_width_str[20], sensor_height_str[20];
        char   intensity_algorithm_str[20];
        char   title_str[100];

        static double screen_copy[MAX_SCREEN][MAX_SCREEN];
        static int z_mm_copy, wavelen_nm_copy;

        // this pane is vertically arranged as follows
        // y-range  y-span  description 
        // -------  ------  -----------
        // 0-499    500     screen
        // 500-500  1       horizontal line
        // 501-620  120     intensity graph  (100 for graph, and 20 for x axis)
        //          ----
        //          621

        // xxx if no wavelen yet

        // xxx comments

        // render the sections that make up this pane, as described above
        if (compute_in_progress == false) {
            memcpy(screen_copy, screen, sizeof(screen_copy));
            z_mm_copy = z_mm;
            wavelen_nm_copy = wavelen_nm;
        }

        render_screen(0, MAX_SCREEN, screen_copy, wavelen_nm_copy, vars->texture, vars->pixels, pane);
        sdl_render_line(pane, 0,MAX_SCREEN, MAX_SCREEN-1,MAX_SCREEN, SDL_WHITE);
        render_intensity_graph(MAX_SCREEN+1, 120, screen_copy, pane);

        // display the title_str at top of pnae
        sprintf(title_str, "W=%0.0lfmm  Z=%dmm  %dnm", 
                TOTAL_SIZE*1000, z_mm_copy, wavelen_nm_copy);
        sdl_render_text(pane, 
                        (pane->w-COL2X(strlen(title_str),LARGE_FONT))/2, 0, 
                        LARGE_FONT, title_str, SDL_WHITE, SDL_BLACK);

        // if enabled then draw horizontal lines accross middle to represent the top and bottom of the sensor
        if (sensor_lines_enabled) {
            int sensor_height_pixels = sensor_height / SCREEN_ELEMENT_SIZE;
            int sensor_min_y = MAX_SCREEN/2 - sensor_height_pixels/2;
            int sensor_max_y = sensor_min_y + sensor_height_pixels - 1;

            sensor_min_y = MAX_SCREEN/2 - sensor_height_pixels/2;
            sensor_max_y = sensor_min_y + sensor_height_pixels - 1;
            sdl_render_line(pane, 0, sensor_min_y, MAX_SCREEN-1, sensor_min_y, SDL_WHITE);
            sdl_render_line(pane, 0, sensor_max_y, MAX_SCREEN-1, sensor_max_y, SDL_WHITE);
        }

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
        default: 
            common_key_controls(event->event_id);
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

static void render_screen(
                int y_top, int y_span, double screen[MAX_SCREEN][MAX_SCREEN], int wavelen_nm,
                texture_t texture, unsigned int pixels[MAX_SCREEN][MAX_SCREEN], 
                rect_t *pane)
{
    int i,j;
    int texture_width, texture_height;
    uint8_t r,g,b;

    // initialize pixels from screen data
    sdl_wavelen_to_rgb(wavelen_nm, &r, &g, &b);
    for (i = 0; i < MAX_SCREEN; i++) {
        for (j = 0; j < MAX_SCREEN; j++) {
            double factor;

            // this translates the calculated screen intensity (which has
            // been normalized to range 0 .. 1 by the sim_get_scren routine)
            // to a pixel intensity; two algorithm choices are provided:
            // - 0: logarithmic  (default)
            // - 1: linear
            // xxx comments
            if (intensity_algorithm == 0) {
                if (screen[i][j] == 0) {
                    factor = 0;
                } else {
                    factor = (1 + 0.2 * log(screen[i][j]));
                    if (factor < 0) factor = 0;
                }
            } else if (intensity_algorithm == 1) {
                factor = screen[i][j];
            } else {
                FATAL("invalid intensity_algorithm %d\n", intensity_algorithm);
            }

            // set the display pixel values 
            pixels[i][j] = PIXEL( (int)(r*factor), (int)(g*factor), (int)(b*factor));
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
    int     y_bottom;

    // allow for 20 pixels for x-axis
    y_span -= 20;
    y_bottom = y_top + y_span - 1;

    // draw graph of sensor_value
    for (screen_idx = 0; screen_idx < MAX_SCREEN; screen_idx++) {
        graph[screen_idx].x = screen_idx;
        graph[screen_idx].y = y_bottom - get_sensor_value(screen_idx,screen) * y_span;
    }
    sdl_render_lines(pane, graph, MAX_SCREEN, SDL_WHITE);

    // xxx comments
    y_top = y_bottom + 1;

    double tick_intvl, tick_loc;
    char tick_value_str[20];
    int tick_value, tick_value_loc, slen;

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
}

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

// -----------------  CONTROL PANE HANDLER  --------------------------------------

static int control_pane_hndlr(pane_cx_t * pane_cx, int request, void * init_params, sdl_event_t * event)
{
    struct {
        int nothing_yet;
    } * vars = pane_cx->vars;
    rect_t * pane = &pane_cx->pane;

    #define SDL_EVENT_AUTO_INIT        (SDL_EVENT_USER_DEFINED + 0)
    #define SDL_EVENT_APERTURE_SELECT  (SDL_EVENT_USER_DEFINED + 10)

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
        // - aperture select
        for (row = 0, i = 0; i < max_aperture; i++, row++) {
            sdl_render_text_and_register_event(
                    pane, 0, ROW2Y(row,LARGE_FONT), LARGE_FONT,
                    aperture[i].name, 
                    aperture_idx == i ? SDL_GREEN : SDL_LIGHT_BLUE, SDL_BLACK,
                    SDL_EVENT_APERTURE_SELECT+i, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);
        }

        sdl_render_text_and_register_event(
            pane, 0, pane->h-ROW2Y(1,LARGE_FONT), LARGE_FONT,
            auto_init_state == 0 ? "AUTO_INIT_START" : "AUTO_INIT_STOP", 
            SDL_LIGHT_BLUE, SDL_BLACK, 
            SDL_EVENT_AUTO_INIT, SDL_EVENT_TYPE_MOUSE_CLICK, pane_cx);

        return PANE_HANDLER_RET_NO_ACTION;
    }

    // -----------------------
    // -------- EVENT --------
    // -----------------------

    if (request == PANE_HANDLER_REQ_EVENT) {
        switch (event->event_id) {
        case SDL_EVENT_APERTURE_SELECT ... SDL_EVENT_APERTURE_SELECT+MAX_APERTURE-1: {
            int requested_aperture_idx = event->event_id - SDL_EVENT_APERTURE_SELECT;
            run_compute_thread(requested_aperture_idx, -1, -1);
            break; }
        case SDL_EVENT_AUTO_INIT: {
            auto_init_state = (auto_init_state == 0 ? 1 : 0);
            break; }
        default: 
            common_key_controls(event->event_id);
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

// -----------------  COMMON KEY CONTROLS  --------------------------------------

static void common_key_controls(int key)
{
    switch (key) {
    case SDL_EVENT_KEY_LEFT_ARROW:
    case SDL_EVENT_KEY_RIGHT_ARROW:
    case SDL_EVENT_KEY_SHIFT_LEFT_ARROW:
    case SDL_EVENT_KEY_SHIFT_RIGHT_ARROW: {
        int requested_z_mm = 
                (key == SDL_EVENT_KEY_SHIFT_LEFT_ARROW  ? MIN_Z_MM :
                 key == SDL_EVENT_KEY_SHIFT_RIGHT_ARROW ? MAX_Z_MM :
                 key == SDL_EVENT_KEY_LEFT_ARROW        ? z_mm - DELTA_Z_MM :
                                                          z_mm + DELTA_Z_MM);
        if (requested_z_mm == z_mm || requested_z_mm < MIN_Z_MM || requested_z_mm > MAX_Z_MM) {
            break;
        }
        run_compute_thread(-1, -1, requested_z_mm);
        break; }
    case SDL_EVENT_KEY_DOWN_ARROW:
    case SDL_EVENT_KEY_UP_ARROW:
    case SDL_EVENT_KEY_SHIFT_DOWN_ARROW:
    case SDL_EVENT_KEY_SHIFT_UP_ARROW: {
        int requested_wavelen_nm = 
                (key == SDL_EVENT_KEY_SHIFT_DOWN_ARROW ? MIN_WAVLEN_NM :
                 key == SDL_EVENT_KEY_SHIFT_UP_ARROW   ? MAX_WAVLEN_NM :
                 key == SDL_EVENT_KEY_DOWN_ARROW       ? wavelen_nm - DELTA_WAVLEN_NM :
                                                         wavelen_nm + DELTA_WAVLEN_NM);
        if (requested_wavelen_nm == wavelen_nm || 
            requested_wavelen_nm < MIN_WAVLEN_NM || 
            requested_wavelen_nm > MAX_WAVLEN_NM) 
        {
            break;
        }
        run_compute_thread(-1, requested_wavelen_nm, -1);
        break; }
    }
}

// -----------------  COMPUTE THREAD  --------------------------------------------

static void *compute_thread(void *cx);
static void get_screen(void);

static void run_compute_thread(int requested_aperture_idx, int requested_wavelen_nm, int requested_z_mm)
{
    pthread_t tid;

    if (compute_in_progress) return;

    INFO("requested_aperture_idx=%d  requested_wavelen_nm=%d nm  requested_z_mm=%d mm\n",
         requested_aperture_idx, requested_wavelen_nm, requested_z_mm);

    if (requested_aperture_idx != -1) aperture_idx = requested_aperture_idx;
    if (requested_wavelen_nm != -1) wavelen_nm = requested_wavelen_nm;
    if (requested_z_mm != -1) z_mm = requested_z_mm;

    memset(screen, 0, sizeof(screen));

    compute_in_progress = true;
    pthread_create(&tid, NULL, compute_thread, NULL);
}

static void *compute_thread(void *cx)
{
    int fd;
    char filename[200];

    // xxx create detached

    sprintf(filename, "saved_results/%s__%d__%d__.dat", 
            aperture[aperture_idx].full_name, wavelen_nm, z_mm);
    //INFO("filename = '%s'\n", filename);

    fd = open(filename, O_RDONLY);
    if (fd != -1) {
        read(fd, screen, sizeof(screen));
        close(fd);
        compute_in_progress = false;
        return NULL;
    }

    aperture_select(aperture_idx);
    angular_spectrum_method_execute(wavelen_nm/1e9, z_mm/1000.);
    get_screen();

    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    write(fd, screen, sizeof(screen));
    close(fd);

    compute_in_progress = false;
    return NULL;
}

static void get_screen(void)
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

// -----------------  AUTO INIT  -------------------------------------------------

static void auto_init(void *cx)
{
    static int auto_init_aperture_idx;
    static int auto_init_wavelen_nm;
    static int auto_init_z_mm;

    // if auto_init is not active then return
    if (auto_init_state == 0) return;

    // if auto_init was just activitated then init state variables
    if (auto_init_state == 1) {
        INFO("auto_init starting\n");
        auto_init_state = 2;
        auto_init_aperture_idx = 0;
        auto_init_wavelen_nm = MIN_WAVLEN_NM;
        auto_init_z_mm = MIN_Z_MM;
    }

    // if xxx comment
    if (compute_in_progress) return;

    // run_compute_thread
    run_compute_thread(auto_init_aperture_idx, auto_init_wavelen_nm, auto_init_z_mm);

    // advance z, wavelen, and aperture_idx
    auto_init_z_mm += DELTA_Z_MM;
    if (auto_init_z_mm > MAX_Z_MM) {
        auto_init_z_mm = MIN_Z_MM;
        auto_init_wavelen_nm += DELTA_WAVLEN_NM;
        if (auto_init_wavelen_nm > MAX_WAVLEN_NM) {
            auto_init_wavelen_nm = MIN_WAVLEN_NM;
            auto_init_aperture_idx++;
        }
    }

    // if aperture_idx has reached max then disable auto_init
    if (auto_init_aperture_idx == max_aperture) {
        INFO("auto_init completed\n");
        auto_init_state = 0;
    }
}

