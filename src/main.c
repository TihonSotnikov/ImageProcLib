
#include <stdio.h>
#include "input_output.h"
#include "imageproc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define NAMELEN 128

typedef enum {
    UNSPECIFIED = 0,
    GAUSS,
    EDGE_DETECTION,
    MEDIAN,
    GRAY
} Tool;

int F_HELP = 0;
int F_OUTPUT = 0;
Tool TOOL = UNSPECIFIED;
ImageFormat FORMAT_IN = UNKNOWN;
ImageFormat FORMAT_OUT = UNKNOWN;

float PARAMETERS[4] = {5,0,0,0};
char FILENAME_IN[NAMELEN];
char FILENAME_OUT[NAMELEN];
int PCNT = 0;

int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        printf("Usage:\n./imgproc gauss|median|edge_detection|grayscale \"path/to/image.jpg|png\" [radius/sigma] [-o \"output/result.jpg|png\"]");
        getch();
        return 1;
    }
    

    for (int p = 1; p < argc; p++)
    {
        if (strstr(argv[p], ".jpg") != NULL || strstr(argv[p], ".jpeg") != NULL)
        {
            if (F_OUTPUT)
            {
                strcpy_s(FILENAME_OUT, NAMELEN, argv[p]);
                FORMAT_OUT = JPEG;
                F_OUTPUT = 0;
            }
            else
            {
                strcpy_s(FILENAME_IN, NAMELEN, argv[p]);
                FORMAT_IN = JPEG;
            }
        }
        else if (strstr(argv[p], ".png") != NULL)
        {
            if (F_OUTPUT)
            {
                strcpy_s(FILENAME_OUT, NAMELEN, argv[p]);
                FORMAT_OUT = PNG;
                F_OUTPUT = 0;
            }
            else
            {
                strcpy_s(FILENAME_IN, NAMELEN, argv[p]);
                FORMAT_IN = PNG;
            }
        }
        else if (argv[p][0] >= '0' && argv[p][0] <= '9')
        {
            if (PCNT < 4) sscanf(argv[p], "%f", &PARAMETERS[PCNT++]);
        }
        else if (TOOL == UNSPECIFIED && strcmp(argv[p], "gauss") == 0) TOOL = GAUSS;
        else if (TOOL == UNSPECIFIED && strcmp(argv[p], "median") == 0) TOOL = MEDIAN;
        else if (TOOL == UNSPECIFIED && strcmp(argv[p], "edge_detection") == 0) TOOL = EDGE_DETECTION;
        else if (TOOL == UNSPECIFIED && strcmp(argv[p], "grayscale") == 0) TOOL = GRAY;
        else if (strcmp(argv[p], "-o") == 0) F_OUTPUT = 1;
        else if (strcmp(argv[p], "-h") == 0) F_HELP = 1;
    }


    if (FORMAT_IN == UNKNOWN)
    {
        fprintf(stderr, "Compatible input file not found.");
        getch();
        return -1;
    }
    printf("Input path: %s\n", FILENAME_IN);

    if (FORMAT_OUT == UNKNOWN)
    {
        FORMAT_OUT = FORMAT_IN;
        strcpy_s(FILENAME_OUT, NAMELEN, FORMAT_IN == JPEG ? "output.jpg" : "output.png");
        printf("Output path: %s\n", FILENAME_IN);
    }
    if (TOOL == UNSPECIFIED)
    {
        fprintf(stderr, "No tool selected. Available tools:\ngauss, median, edge_detection, grayscale");
        getch();
        return -1;
    }
    

    // Подготовка структуры
    Image* image = (Image*)malloc(sizeof(Image));
    image->data = NULL;

    // Загрузка FILENAME_IN изображения
    ImageProcStatus status = ipl_load_image(FILENAME_IN, image, FORMAT_IN == JPEG ? JPEG : PNG);

    switch (status)
    {
    case SUCCESS:
        break;
    
    case FILE_NOT_FOUND:
        fprintf(stderr, "File not found.");
        getch();
        return -1;
    
    case OUT_OF_MEMORY:
        fprintf(stderr, "Couldn't allocate memory.");
        getch();
        return -1;
    
    default:
        fprintf(stderr, "Unexpected error. Code: %d", status);
        getch();
        return -1;
    }

    switch (TOOL)
    {
    case GAUSS:
        status = ipl_gaussian_filter(image, PARAMETERS[0]);
        break;

    case EDGE_DETECTION:
        status = ipl_sobel_edge_detection(image);
        break;

    case MEDIAN:
        status = ipl_median_filter(image, (int)PARAMETERS[0]);
        break;

    case GRAY:
        status = ipl_grayscale(image);
        break;
        
    default:
        status = ipl_median_filter(image, (int)PARAMETERS[0]);
        break;
    }

    printf("Filter status = %d\n", status);

    status = ipl_save_image(FILENAME_OUT, image, JPEG);

    printf("Save Image status = %d\n", status);

    return 0;
}
