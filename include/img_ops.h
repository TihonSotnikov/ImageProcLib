#ifndef IMAGEPROC_IMAGE_OPERATIONS
#define IMAGEPROC_IMAGE_OPERATIONS

#include "imageproc.h"

ImageProcStatus ipl_gaussian_filter(Image* image, const float sigma);
ImageProcStatus ipl_sobel_edge_detection(Image* image);

#endif