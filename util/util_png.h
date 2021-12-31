#ifndef __UTIL_PNG_H__
#define __UTIL_PNG_H__

int32_t read_png_file(char* file_name, int32_t max_image_dim,
                       uint8_t ** pixels, int32_t * width, int32_t * height);

int32_t write_png_file(char* file_name,
                       uint8_t * pixels, int32_t width, int32_t height);

#endif
