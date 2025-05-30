#ifndef INPUT_OUTPUT
#define INPUT_OUTPUT

#include <stdio.h>
#include "imageproc.h"

ImageProcStatus load_image(const char* file_name, Image* image, const ImageFormat file_format);
ImageProcStatus save_image(const char* file_name, Image* image, const ImageFormat file_format);

#endif