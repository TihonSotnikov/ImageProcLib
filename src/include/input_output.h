#ifndef INPUT_OUTPUT
#define INPUT_OUTPUT

#include <stdio.h>
#include "imageproc.h"

ImageProcStatus load_image(const char* filename, Image* image, ImageFormat format);
ImageProcStatus save_image(const char* filename, Image* image, ImageFormat format);

#endif