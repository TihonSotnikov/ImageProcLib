#include "imageproc.h"
#include <stdlib.h>

int median_filter(Image *image, int radius)
{
    unsigned char* output = malloc(image->width * image->height * image->channels);
    for (size_t i = 0; i < image->height; i++)
    {
        for (size_t j = 0; j < image->width; j++)
        {
            int sum = 0;
            for (size_t k = 0; k < image->channels; k++)
            {
                for (int r = -radius; r <= radius; r++)
                {
                    for (int c = -radius; c <= radius; c++)
                    {
                        int x = i + r;
                        int y = j + c;
                        if (x < 0 || x >= image->height || y < 0 || y >= image->width)
                        {
                            continue;
                        }
                        sum += image->data[x * image->width * image->channels + y * image->channels + k];
                    }
                }
                output[i * image->width * image->channels + j * image->channels + k] = sum / ((2 * radius + 1) * (2 * radius + 1));
            }
        }
    }
    return SUCCESS;
}

int gaussian_filter(Image *image)
{
    unsigned char* output = malloc(image->width * image->height * image->channels);
    
}