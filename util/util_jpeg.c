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
#include <jpeglib.h>

#include "util_jpeg.h"
#include "util_misc.h"

//
// documentationn: 
//   http://www.opensource.apple.com/source/tcl/tcl-87/tcl_ext/tkimg/tkimg/libjpeg/libjpeg.doc
//   https://refspecs.linuxfoundation.org/LSB_4.1.0/LSB-Desktop-generic/LSB-Desktop-generic/libjpegman.html
//   https://github.com/LuaDist/libjpeg/blob/master/example.c
//

//
// defines
//

#define BYTES_PER_PIXEL 4

//
// typedefs
//

//
// variables
//

static jmp_buf err_jmpbuf;

//
// prototypes
//

static void jpeg_decode_error_exit_override(j_common_ptr cinfo);
static void jpeg_decode_output_message_override(j_common_ptr cinfo);

// -----------------  JPEG DECOMPRESSION  --------------------------------------------------

//
// Args: 
// - file_name: pathname of the jpeg file to be read
// - max_image_dim: When this arg is 0 it is not used; otherwise this arg is a 
//   request that the returned image width and height do not exceed max_image_dim.
//   The jpeg library image scaling feature is used to accomplish this; however, this
//   feature does not support reducing the image size by greater than a factor of 8.
//   For extremely large jpeg images or extremely small max_image_dim, the
//   scaling will reduce the image by a factor of 8, which may not succeed in 
//   reducing the returned image's dimensions to less or equal to the max_image_dim.
// - pixels: the pixels is malloced by read_jpeg_file; the caller should free
//   this memory when done; 4 bytes per pixel; in SDL_PIXELFORMAT_ABGR8888
// - width, height: return the image width and height
//

int32_t read_jpeg_file(char* file_name, int32_t max_image_dim,
                       uint8_t ** pixels, int32_t * width, int32_t * height)
{
    FILE                          * fp = NULL;
    struct jpeg_decompress_struct   cinfo; 
    struct jpeg_error_mgr           err_mgr;
    uint8_t                       * out = NULL;

    // preset returns to caller
    *pixels = NULL;
    *width  = 0;
    *height = 0;

    // open file_name
    fp = fopen(file_name, "rb");
    if (!fp) {
        ERROR("fopen %s\n", file_name);
        return -1;
    }

    // initailze setjmp, for use by the error exit override
    if (setjmp(err_jmpbuf)) {
        goto error_return;
    }

    // error management init:
    // - override the error_exit routine
    // - override the output_message routine
    cinfo.err = jpeg_std_error(&err_mgr);
    cinfo.err->error_exit = jpeg_decode_error_exit_override;
    cinfo.err->output_message = jpeg_decode_output_message_override;

    // initialize the jpeg decompress object,
    // supply fp to the jpeg decoder,
    // read the jpeg header, require_image==true
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, true);

    // set the desired output pixel format
    cinfo.out_color_space = JCS_RGB;

    // if max image dimension is supplied and the image's width or height 
    // exceeds the max_image_dim then use the jpeg library to request that 
    // the decompressed image be scaled down to a smaller size; 
    // note that the jpeg image library documentation states:
    // "Currently, the only supported scaling ratios are 1/1, 1/2, 1/4, and 1/8."
    if (max_image_dim > 0) {
        if (cinfo.image_width > (max_image_dim*4) || cinfo.image_height > (max_image_dim*4)) {
            cinfo.scale_denom = 8;
        } else if (cinfo.image_width > (max_image_dim*2) || cinfo.image_height > (max_image_dim*2)) {
            cinfo.scale_denom = 4;
        } else if (cinfo.image_width > (max_image_dim) || cinfo.image_height > (max_image_dim)) {
            cinfo.scale_denom = 2;
        } else {
            cinfo.scale_denom = 1;
        }
    }

    // initialize the decompression, this sets cinfo.output_width and cinfo.output_height
    jpeg_start_decompress(&cinfo);

    // allocate memory for the output, must be after call to jpeg_start_decompress
    out = malloc(cinfo.output_width * cinfo.output_height * BYTES_PER_PIXEL);
    if (out == NULL) {
        ERROR("failed allocate memory for width=%d height=%d bytes_per_pixel=%d\n",
               cinfo.output_width, cinfo.output_height, BYTES_PER_PIXEL);
        goto error_return;
    }

    // loop over scanlines
    uint8_t * outp = out;
    while (cinfo.output_scanline < cinfo.output_height) {
        int32_t i;
        JSAMPLE   row[1000000];
        JSAMPROW  scanline[1] = { row };
        uint8_t * r = row;

        // read one scanline
        jpeg_read_scanlines(&cinfo, scanline, 1);

        // save the row data in the output buffer
        for (i = 0; i < cinfo.output_width; i++) {
            outp[0] = r[0];
            outp[1] = r[1];
            outp[2] = r[2];
            outp[3] = 255;  
            outp+=4;
            r+=3;
        }
    }

    // finish decompress
    jpeg_finish_decompress(&cinfo);

    // success return
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    *pixels = out;
    *width  = cinfo.output_width;
    *height = cinfo.output_height;
    return 0;

    // error return
error_return:
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    free(out);
    return -1;
}

// -----------------  JPEG COMPRESSION  ----------------------------------------------------

int32_t write_jpeg_file(char* file_name, 
                       uint8_t * pixels, int32_t width, int32_t height)
{
    FILE                        * fp = NULL;
    struct jpeg_compress_struct   cinfo; 
    struct jpeg_error_mgr         err_mgr;

    // open file_name
    fp = fopen(file_name, "wb");
    if (!fp) {
        ERROR("fopen %s\n", file_name);
        return -1;
    }

    // initailze setjmp, for use by the error exit override
    if (setjmp(err_jmpbuf)) {
        goto error_return;
    }

    // error management init:
    // - override the error_exit routine
    // - override the output_message routine
    cinfo.err = jpeg_std_error(&err_mgr);
    cinfo.err->error_exit = jpeg_decode_error_exit_override;
    cinfo.err->output_message = jpeg_decode_output_message_override;

    // initialize the jpeg compress object,
    // supply fp to the jpeg encoder,
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    // describe the input image
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    // set default compression parameters
    jpeg_set_defaults(&cinfo);

    // initialize the compression
    jpeg_start_compress(&cinfo, TRUE);

    // loop over scanlines
    uint8_t * inp = pixels;
    while (cinfo.next_scanline < cinfo.image_height) {
        int32_t   i;
        JSAMPLE   row[1000000];
        JSAMPROW  scanline[1] = { row };
        uint8_t * r = row;

        // convert scanline pixelformat from 
        // SDL_PIXELFORMAT_ABGR8888 to JCS_RGB
        for (i = 0; i < cinfo.image_width; i++) {
            r[0] = inp[0];
            r[1] = inp[1];
            r[2] = inp[2];
            inp += 4;
            r   += 3;
        }

        // write the JCS_RGB scanline
        jpeg_write_scanlines(&cinfo, scanline, 1);
    }

    // finish compress
    jpeg_finish_compress(&cinfo);

    // success return
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return 0;

    // error return
error_return:
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return -1;
}

// -----------------  SUPPORT  -------------------------------------------------------------

static void jpeg_decode_error_exit_override(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);
    longjmp(err_jmpbuf, 1);
}

static void jpeg_decode_output_message_override(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    (*cinfo->err->format_message)(cinfo, buffer);
    if (strncmp(buffer, "Not a JPEG file", 15) != 0) {
        ERROR("%s\n", buffer);
    } else {
        DEBUG("%s\n", buffer);
    }
}
