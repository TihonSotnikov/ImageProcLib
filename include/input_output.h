#ifndef INPUT_OUTPUT
#define INPUT_OUTPUT

#include <stdio.h>
#include "imageproc.h"

void write_to_file_contextual(void *context, void *data, int size);
ImageProcStatus free_image_data(Image* image);
ImageProcStatus ipl_load_image(const char* file_name, Image* image, const ImageFormat file_format);
ImageProcStatus ipl_save_image(const char* file_name, Image* image, const ImageFormat file_format);

#endif