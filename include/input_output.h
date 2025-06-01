#ifndef IMAGEPROC_INPUT_OUTPUT
#define IMAGEPROC_INPUT_OUTPUT

#include "imageproc.h"

ImageProcStatus ipl_load_image(const char* file_name, Image* image, const ImageFormat file_format);
ImageProcStatus ipl_save_image(const char* file_name, Image* image, const ImageFormat file_format);

#endif