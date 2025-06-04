#ifndef PROC_H
#define PROC_H

#include <stdio.h>

typedef enum
{
    PNG,
    JPEG, 
    UNKNOWN // неподдерживаемый / неопределенный формат
} ImageFormat;

typedef enum
{
    GRAYSCALE = 1,
    RGB = 3,
    RGBA = 4
} ImageColorChannels;

typedef struct
{
    ImageFormat format;
    size_t width;
    size_t height;
    ImageColorChannels channels;
    unsigned char* data;
} Image;

typedef enum
{
    SUCCESS = 0,        
    INVALID_ARGUMENT,   // некорректный / недопустимый аргумент
    FILE_NOT_FOUND,     // не найден файл
    FILE_ACCESS_DENIED, // нет прав доступа к файлу (чтение / запись)
    FILE_READ,          // ошибка во время чтения файла 
    FILE_WRITE,         // ошибка во время записи файла 
    UNSUPPORTED_FORMAT, // формат файла не поддерживается 
    OUT_OF_MEMORY,      // не удалось выделить необходимую память
    INTERNAL,           // непредвиденная внутренняя ошибка в библиотеке
} ImageProcStatus;

// @brief Структура для представления одномерного гауссова ядра свертки.
//        Ядро симметрично и нормализовано (сумма коэффициентов равна 1).
typedef struct
{
    float* values; // Указатель на массив коэффициентов ядра.
    int radius;    // Радиус ядра (количество элементов от центра до края).
} Kernel;

// PART A

unsigned char to_uchar(float value);
Kernel* generate_gaussian_kernel(const float sigma);
void free_kernel(Kernel* kernel);
void horizontal_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel);
void vertical_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel);
ImageProcStatus ipl_gaussian_filter(Image* image, const float sigma);
void convert_to_one_channel(const unsigned char* input_data, unsigned char* output_data, const size_t width, const size_t height, const int channels_in);
void compute_sobel_magnitude(const unsigned char* input_grayscale_data, unsigned char* output_gradient_map, const size_t width, const size_t height);
ImageProcStatus ipl_sobel_edge_detection(Image* image);

// PART B

static inline int clamp(int val, int min_val, int max_val);
ImageProcStatus ipl_median_filter(Image *image, const int radius);
void hist_set(int *hist, unsigned char *original, unsigned char *padded, int y, int ldp, int x, int channels, int c, int r, int *hy, int *hx);
void hist_move(int *hist, unsigned char *original, unsigned char *padded, int y, int ldp, int *hx, int channels, int c, int r);
unsigned char get_median(int *hist, int r);
ImageProcStatus ipl_grayscale(Image *image);

#endif 
