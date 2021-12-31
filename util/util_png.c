#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <assert.h>

#include <setjmp.h>
#include <png.h>

#include "util_png.h"
#include "util_misc.h"

//
// documentationn: 
//  http://www.libpng.org
//  http://www.libpng.org/pub/png/pngdocs.html
//  http://www.libpng.org/pub/png/libpng-manual.txt
//
// example
//   http://zarb.org/~gc/html/libpng.html
//

//
// Possible Future Enhancements
// - add user_error_fn, and user_warning_fn
// - determine what the following code does, and would it be helpful
//      int32_t number_of_passes = png_set_interlace_handling(png_ptr);
//      png_read_update_info(png_ptr, png_info);
// - implement max_image_dim
//

//
// defines
//

#define PNG_COLOR_TYPE_STR(x) \
    ((x) == PNG_COLOR_TYPE_GRAY       ? "PNG_COLOR_TYPE_GRAY" : \
     (x) == PNG_COLOR_TYPE_PALETTE    ? "PNG_COLOR_TYPE_PALETTE" : \
     (x) == PNG_COLOR_TYPE_RGB        ? "PNG_COLOR_TYPE_RGB" : \
     (x) == PNG_COLOR_TYPE_RGB_ALPHA  ? "PNG_COLOR_TYPE_RGB_ALPHA" : \
     (x) == PNG_COLOR_TYPE_GRAY_ALPHA ? "PNG_COLOR_TYPE_GRAY_ALPHA"  \
                                      : "????")

//
// variables
//

//
// prototypes
//

// -----------------  READ PNG FILE  ---------------------------------------------------

//
// Args:  
// - file_name: pathname of the png file to be read
// - max_image_dim: currently not implemented; the intent is if the 
//   image width or height exceeds max_image_dim then the image size would be
//   scaled down such that the width and height are less or equal to max_image_dim
// - pixels: the pixels_arg is malloced by read_png_file; the caller should free
//   this memory when done; 4 bytes per pixel; in SDL_PIXELFORMAT_ABGR8888
// - width_arg, height_arg: return the image width and height
//
// Notes:          
// - the only png file format currently supported is PNG_COLOR_TYPE_RGB_ALPHA
//

int32_t read_png_file(char* file_name, int32_t max_image_dim,
                   uint8_t ** pixels_arg, int32_t * width_arg, int32_t * height_arg)
{
    FILE        * fp           = NULL;
    png_structp   png_ptr      = NULL;
    png_infop     png_info     = NULL;
    uint8_t     * pixels       = NULL;
    uint8_t    ** row_pointers = NULL;
    uint8_t       hdr[8];
    int32_t       len, width, height, color_type, rowbytes, y, ret;
    int32_t       bit_depth __attribute__((unused));

    // preset returns to caller
    *pixels_arg = NULL;
    *width_arg  = 0;
    *height_arg = 0;

    // open file_name
    fp = fopen(file_name, "rb");
    if (!fp) {
        ERROR("%s: fopen failed, %s\n", file_name, strerror(errno));
        goto error;
    }

    // read and verify header
    len = fread(hdr, 1, sizeof(hdr), fp);
    if (len != sizeof(hdr)) {
        ERROR("%s: hdr read failed, len=%d, %s\n", file_name, len, strerror(errno));
        goto error;
    }
    if (png_sig_cmp(hdr, 0, sizeof(hdr))) {
        DEBUG("%s: is not a png file\n", file_name);
        goto error;
    }

    // init
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        ERROR("%s: png_create_read_struct failed\n", file_name);
        goto error;
    }

    png_info = png_create_info_struct(png_ptr);
    if (png_info == NULL) {
        ERROR("%s: png_create_info_struct failed\n", file_name);
        goto error;
    }

    // register error jmpbuf
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("%s: failed\n", file_name);
        goto error;
    }

    // provide the file-pointer to png
    png_init_io(png_ptr, fp);

    // inform png that we've already read the signature
    png_set_sig_bytes(png_ptr, 8);

    // read png info
    png_read_info(png_ptr, png_info);
    width      = png_get_image_width(png_ptr, png_info);
    height     = png_get_image_height(png_ptr, png_info);
    color_type = png_get_color_type(png_ptr, png_info);
    bit_depth  = png_get_bit_depth(png_ptr, png_info);
    DEBUG("width=%d height=%d color_type=%s bit_depth=%d\n",
          width, height, PNG_COLOR_TYPE_STR(color_type), bit_depth);

    // currently this routine supports only PNG_COLOR_TYPE_RGB_ALPHA
    if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
        ERROR("%s: unsupported color_type %s\n", file_name, PNG_COLOR_TYPE_STR(color_type));
        goto error;
    }

    // get the number of bytes in a row, and
    // allocate memory for the pixels
    rowbytes = png_get_rowbytes(png_ptr,png_info);
    DEBUG("rowbytes=%d\n", rowbytes);
    pixels = malloc(height*rowbytes);
    if (pixels == NULL) {
        ERROR("%s: malloc pixels failed, %dx%d\n", file_name, width, height);
        goto error;
    }

    // allocate and init row_pointers
    row_pointers = malloc(sizeof(void*) * height);
    for (y = 0; y < height; y++) {
        row_pointers[y] = pixels + y * rowbytes;
    }

    // read the image
    png_read_image(png_ptr, row_pointers);

    // success return
    *width_arg  = width;
    *height_arg = height;
    *pixels_arg = pixels;
    ret = 0;
    goto cleanup;

    // error return
error:
    ret = -1;
    goto cleanup;

    // cleanup and return
cleanup:
    if (fp) {
        fclose(fp);
    }
    free(row_pointers);
    if (ret == -1) {
        free(pixels);
    }
    png_destroy_read_struct(&png_ptr, &png_info, NULL);
    return ret;
}

// -----------------  WRITE PNG FILE  ---------------------------------------------------

//
// Args:  
// - file_name: pathname of the png file to be written
// - pixels: 4 bytes per pixel; in SDL_PIXELFORMAT_ABGR8888
// - width, height: the image width and height
//
// Notes:
// - created png file color_type is PNG_COLOR_TYPE_RGB_ALPHA
//

int32_t write_png_file(char* file_name,
                       uint8_t * pixels, int32_t width, int32_t height)
{
    #define BYTES_PER_PIXEL 4

    FILE      * fp        = NULL;
    png_structp png_ptr   = NULL;
    png_infop   png_info  = NULL;
    png_bytep   row_pointers[height];    
    int32_t     color_type, bit_depth, y, ret;

    // create file 
    fp = fopen(file_name, "wb");
    if (!fp) {
        ERROR("%s: fopen failed, %s\n", file_name, strerror(errno));
        goto error;  
    }

    // initialize
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    bit_depth = 8;
    for (y = 0; y < height; y++) {
        row_pointers[y] = &pixels[y*width*BYTES_PER_PIXEL];
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        ERROR("%s: png_create_write_struct failed\n", file_name);
        goto error;  
    }

    png_info = png_create_info_struct(png_ptr);
    if (!png_info) {
        ERROR("%s: png_create_info_struct failed\n", file_name);
        goto error;  
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("%s: failed\n", file_name);
        goto error;  
    }

    png_init_io(png_ptr, fp);

    // write file header 
    png_set_IHDR(png_ptr, png_info, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, png_info);

    // write bytes 
    png_write_image(png_ptr, row_pointers);

    // end write 
    png_write_end(png_ptr, NULL);

    // success return
    ret = 0;
    goto cleanup;

    // error return
error:
    ret = -1;
    goto cleanup;

    // cleanup and return
cleanup:
    if (fp) {
        fclose(fp);
    }
    png_destroy_write_struct(&png_ptr, &png_info);
    return ret;
}
