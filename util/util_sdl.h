#ifndef __UTIL_SDL_H__
#define __UTIL_SDL_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/queue.h>

//
// GENERAL
//

// colors
#define SDL_PURPLE     0 
#define SDL_BLUE       1
#define SDL_LIGHT_BLUE 2
#define SDL_GREEN      3
#define SDL_YELLOW     4
#define SDL_ORANGE     5
#define SDL_PINK       6
#define SDL_RED        7
#define SDL_GRAY       8
#define SDL_WHITE      9
#define SDL_BLACK      10

// pixels
#define BYTES_PER_PIXEL  4
#define PIXEL(r,g,b)     (((r) << 0) | ((g) << 8) | ((b) << 16) | (255 << 24))
#define PIXEL_PURPLE     PIXEL(127,0,255)
#define PIXEL_BLUE       PIXEL(0,0,255)
#define PIXEL_LIGHT_BLUE PIXEL(0,255,255)
#define PIXEL_GREEN      PIXEL(0,255,0)
#define PIXEL_YELLOW     PIXEL(255,255,0)
#define PIXEL_ORANGE     PIXEL(255,128,0)
#define PIXEL_PINK       PIXEL(255,105,180)
#define PIXEL_RED        PIXEL(255,0,0)
#define PIXEL_GRAY       PIXEL(224,224,224)
#define PIXEL_WHITE      PIXEL(255,255,255)
#define PIXEL_BLACK      PIXEL(0,0,0)

#define PIXEL_TO_RGB(p,r,g,b) \
    do { \
        r = ((p) >>  0) & 0xff; \
        g = ((p) >>  8) & 0xff; \
        b = ((p) >> 16) & 0xff; \
    } while (0)

#define FIRST_SDL_CUSTOM_COLOR 20

// convert font_ptsize to pixels
#define COL2X(c,font_ptsize)   ((c) * sdl_font_char_width(font_ptsize))
#define ROW2Y(r,font_ptsize)   ((r) * sdl_font_char_height(font_ptsize))

// rectangle
typedef struct {
    int16_t x, y;
    int16_t w, h;
} rect_t;

// point
typedef struct {
    int32_t x, y;
} point_t;

// texture
typedef void * texture_t;

//
// EVENTS            
//

// event types
#define SDL_EVENT_TYPE_MOUSE_CLICK          1
#define SDL_EVENT_TYPE_MOUSE_RIGHT_CLICK    2
#define SDL_EVENT_TYPE_MOUSE_MOTION         3
#define SDL_EVENT_TYPE_MOUSE_WHEEL          4

// event identifiers
// - no event
#define SDL_EVENT_NONE                   0
// - keyboard events
//   . event_id in range 1 to 127 are ASCII
//   . event_id in range 0x100 to 0x1ff are special keys
//   . if ctrl is active then SDL_EVENT_KEY_CTRL is added to the event_id value
//   . if alt is active then SDL_EVENT_KEY_ALT is added to the event_id value
#define SDL_EVENT_KEY_ESC                0x1b
#define SDL_EVENT_KEY_DELETE             0x7f
#define SDL_EVENT_KEY_INSERT             0x101
#define SDL_EVENT_KEY_HOME               0x102
#define SDL_EVENT_KEY_END                0x103
#define SDL_EVENT_KEY_PGUP               0x104
#define SDL_EVENT_KEY_PGDN               0x105
#define SDL_EVENT_KEY_UP_ARROW           0x106
#define SDL_EVENT_KEY_DOWN_ARROW         0x107
#define SDL_EVENT_KEY_LEFT_ARROW         0x108
#define SDL_EVENT_KEY_RIGHT_ARROW        0x109
#define SDL_EVENT_KEY_SHIFT_UP_ARROW     0x10a
#define SDL_EVENT_KEY_SHIFT_DOWN_ARROW   0x10b
#define SDL_EVENT_KEY_SHIFT_LEFT_ARROW   0x10c
#define SDL_EVENT_KEY_SHIFT_RIGHT_ARROW  0x10d
#define SDL_EVENT_KEY_F(n)               (0x10f+(n))  // n=1...12
#define SDL_EVENT_KEY_CTRL               0x1000
#define SDL_EVENT_KEY_ALT                0x2000
// - window events
#define SDL_EVENT_WIN_SIZE_CHANGE        0x8001
#define SDL_EVENT_WIN_MINIMIZED          0x8002
#define SDL_EVENT_WIN_RESTORED           0x8003
// - program quit event
#define SDL_EVENT_QUIT                   0x8fff
// - user defined events base
#define SDL_EVENT_USER_DEFINED           0x10000

// check if event_id is associated with the keyboard
#define SDL_EVENT_KEY_FIRST        0x1
#define SDL_EVENT_KEY_LAST         (SDL_EVENT_KEY_CTRL + SDL_EVENT_KEY_ALT + 0x1ff)
#define IS_KEYBOARD_EVENT_ID(evid) ((evid >= SDL_EVENT_KEY_FIRST) && ((evid) <= SDL_EVENT_KEY_LAST))

// event data structure 
typedef struct {
    int32_t event_id;
    void  * event_cx;
    union {
        struct {
            int32_t x;
            int32_t y;
        } mouse_click;
        struct {
            int32_t delta_x;
            int32_t delta_y;
            int32_t x;
            int32_t y;
            bool start;
            bool end;
            int32_t end_abs_total_delta_x;
            int32_t end_abs_total_delta_y;
        } mouse_motion;
        struct {
            int32_t delta_x;
            int32_t delta_y;
        } mouse_wheel;
    };
} sdl_event_t;

//
// PANES            
//

#define PANE_BORDER_STYLE_NONE      0
#define PANE_BORDER_STYLE_MINIMAL   1
#define PANE_BORDER_STYLE_STANDARD  2

#define PANE_HANDLER_REQ_INITIALIZE         0 
#define PANE_HANDLER_REQ_RENDER             1
#define PANE_HANDLER_REQ_EVENT              2
#define PANE_HANDLER_REQ_TERMINATE          3

#define PANE_HANDLER_RET_NO_ACTION          0
#define PANE_HANDLER_RET_PANE_TERMINATE     1
#define PANE_HANDLER_RET_DISPLAY_REDRAW     2

struct pane_cx_s;
TAILQ_HEAD(pane_list_head_s, pane_cx_s);
typedef int32_t (*pane_handler_t)(struct pane_cx_s * pane_cx, int32_t request, void * init, sdl_event_t * event);
typedef struct pane_cx_s {
    void * display_cx;
    int32_t x_disp;
    int32_t y_disp;
    int32_t w_total;
    int32_t h_total;
    int32_t border_style;
    rect_t  pane;
    void *  vars;
    pane_handler_t pane_handler;
    struct pane_list_head_s * pane_list_head;
    TAILQ_ENTRY(pane_cx_s) entries;
} pane_cx_t;

//
// PROTOTYPES
//

// sdl initialize, and misc routines
int32_t sdl_init(int32_t * w, int32_t * h, bool fullscreen, bool resizeable, bool swap_white_black);
void sdl_get_window_size(int32_t * w, int32_t * h);
void sdl_get_max_texture_dim(int32_t * max_texture_dim);
void sdl_full_screen(bool enable);
void sdl_minimize_window(void);

// pane support
void sdl_pane_manager(void *display_cx,                        // optional, context
                      void (*display_start)(void *display_cx), // optional, called prior to pane handlers
                      void (*display_end)(void *display_cx),   // optional, called after pane handlers
                      int64_t redraw_interval_us,              // 0=continuous, -1=never, else us
                      int32_t count,                           // number of pane handler varargs that follow
                      ...);                                    // pane_handler, init_params, x_disp, y_disp, w, h, border_style
void sdl_pane_create(struct pane_list_head_s * pane_list_head, pane_handler_t pane_handler, void * init,
                     int32_t x_disp, int32_t y_disp, int32_t w_total, int32_t h_total, 
                     int32_t border_style, void * display_cx);
void sdl_pane_update(pane_cx_t *pane_cx,
                     int32_t x_disp, int32_t y_disp, int32_t w_total, int32_t h_total);

// display init and present
void sdl_display_init(int32_t * win_width, int32_t * win_height, bool * win_minimized);
void sdl_display_present(void);

// font support
int32_t sdl_pane_cols(rect_t * pane, int32_t font_ptsize);
int32_t sdl_pane_rows(rect_t * pane, int32_t font_ptsize);
int32_t sdl_font_char_width(int32_t font_ptsize);
int32_t sdl_font_char_height(int32_t font_ptsize);

// event support
void sdl_register_event(rect_t * pane, rect_t * loc, int32_t event_id, int32_t event_type, void * event_cx);
void sdl_render_text_and_register_event(rect_t * pane, int32_t x, int32_t y, int32_t font_ptsize, char * str, 
        int32_t fg_color, int32_t bg_color, int32_t event_id, int32_t event_type, void * event_cx);
void sdl_render_texture_and_register_event(rect_t * pane, int32_t x, int32_t y,
        texture_t texture, int32_t event_id, int32_t event_type, void * event_cx);
sdl_event_t * sdl_poll_event(void);
void sdl_push_event(sdl_event_t *ev);
void sdl_play_event_sound(void);

// render text
rect_t sdl_render_text(rect_t * pane, int32_t x, int32_t y, int32_t font_ptsize, char * str, 
            int32_t fg_color, int32_t bg_color);
void sdl_render_printf(rect_t * pane, int32_t x, int32_t y, int32_t font_ptsize, 
            int32_t fg_color, int32_t bg_color, char * fmt, ...) __attribute__ ((format (printf, 7, 8)));

// render rectangle, lines, circles, points
void sdl_render_rect(rect_t * pane, rect_t * loc, int32_t line_width, int32_t color);
void sdl_render_fill_rect(rect_t * pane, rect_t * loc, int32_t color);
void sdl_render_line(rect_t * pane, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void sdl_render_lines(rect_t * pane, point_t * points, int32_t count, int32_t color);
void sdl_render_circle(rect_t * pane, int32_t x_center, int32_t y_center, int32_t radius, 
            int32_t line_width, int32_t color);
void sdl_render_point(rect_t * pane, int32_t x, int32_t y, int32_t color, int32_t point_size);
void sdl_render_points(rect_t * pane, point_t * points, int32_t count, int32_t color, int32_t point_size);

// render using textures
texture_t sdl_create_texture(int32_t w, int32_t h);
texture_t sdl_create_texture_from_pane_pixels(rect_t * pane);
texture_t sdl_create_filled_circle_texture(int32_t radius, int32_t color);
texture_t sdl_create_text_texture(int32_t fg_color, int32_t bg_color, int32_t font_ptsize, char * str);
void sdl_update_texture(texture_t texture, uint8_t * pixels, int32_t pitch);
void sdl_query_texture(texture_t texture, int32_t * width, int32_t * height);
rect_t sdl_render_texture(rect_t * pane, int32_t x, int32_t y, texture_t texture);
rect_t sdl_render_scaled_texture(rect_t * pane, rect_t * loc, texture_t texture);
void sdl_render_scaled_texture_ex(rect_t *pane, rect_t *src, rect_t *dst, texture_t texture);
void sdl_destroy_texture(texture_t texture);

// render using textures - webcam support
texture_t sdl_create_yuy2_texture(int32_t w, int32_t h);
void sdl_update_yuy2_texture(texture_t texture, uint8_t * pixels, int32_t pitch);
texture_t sdl_create_iyuv_texture(int32_t w, int32_t h);
void sdl_update_iyuv_texture(texture_t texture, uint8_t *y_plane, int y_pitch, 
            uint8_t *u_plane, int u_pitch, uint8_t *v_plane, int v_pitch);

// print screen, file_name must end in .jpg or .png
void sdl_print_screen(char * file_name, bool flash_display, rect_t * rect);

// colors
void sdl_define_custom_color(int32_t color, uint8_t r, uint8_t g, uint8_t b);
void sdl_wavelen_to_rgb(double wavelength, uint8_t *r, uint8_t *g, uint8_t *b);
int32_t sdl_color(char * color_str);

#endif
