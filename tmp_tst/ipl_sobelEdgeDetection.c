#include "imageproc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned char to_uchar(float value)
{
    if (value < 0.0f) value = 0.0f;
    else if (value > 255.0f) value = 255.0f;
    return (unsigned char)roundf(value);
}

void convert_to_one_channel(const unsigned char* input_data, unsigned char* output_data, const size_t width, const size_t height, const int channels)
{
    if (channels == 1) // Если изображение Ч/Б, его не нужно преобразовывать
    {
        memcpy(output_data, input_data, height * width * sizeof(unsigned char));
    } 
    else if (channels == 3 || channels == 4) // Преобразование по формуле Y = 0.299 * R + 0.587 * G + 0.114 * B (alpha канал не учитывается)
    {
        int total_bytes = width * height * channels;
        int output_data_idx = 0;
        for (int i = 0; i < total_bytes; i+=channels) // Массив одномерный, данные идут линейно, достаточно одного цикла по пикселям
        {
            float r = (float)input_data[i];
            float g = (float)input_data[i + 1];
            float b = (float)input_data[i + 2];
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;

            output_data[output_data_idx++] = to_uchar(gray);
        }
    }
}

void convolution(const unsigned char* input_data, unsigned char* output_data, const size_t width, const size_t height)
{
    float deriv_kernel[3] = {-1.0f, 0.0f, 1.0f};

    // Циклические буферы для частичных производных
    float dx_buf[3][width];
    float dy_buf[3][width];

    for (int i = 0; i < height; i++)
    {
        // Горизонтальная производная dx
        for (int j = 0; j < width; j++)
        {
            dx_buf[i % 3][j] = 0.0f;
            for (int offset = -1; offset <= 1; offset++)
            {
                int current_pixel_offset = j + offset;

                if (current_pixel_offset < 0) current_pixel_offset = 0;
                else if (current_pixel_offset >= width) current_pixel_offset = width - 1;

                int current_pixel_index = i * width + current_pixel_offset;

                dx_buf[i % 3][j] += input_data[current_pixel_index] * deriv_kernel[offset + 1];
            }
        }

        // Вертикальная производная dy
        for (int j = 0; j < width; j++)
        {
            dy_buf[i % 3][j] = 0.0f;
            for (int offset = -1; offset <= 1; offset++)
            {
                int current_pixel_offset = i + offset;

                if (current_pixel_offset < 0) current_pixel_offset = 0;
                else if (current_pixel_offset >= height) current_pixel_offset = height - 1;

                int current_pixel_index = current_pixel_offset * width + j;

                dy_buf[i % 3][j] += input_data[current_pixel_index] * deriv_kernel[offset + 1]; 
            }
        }

        if (i < 2) continue; // Нужно накопить 3 строки для вертикальной свертки

        // Индексы строк в циклических буферах для текущей выходной строки
        int i0 = (i - 2) % 3;
        int i1 = (i - 1) % 3;
        int i2 = i % 3;

        // Сглаживание по y + сглаживание по x + формирование результата
        for (int j = 0; j < width; j++)
        {
            // Вертикальное сглаживание dx
            float gx = dx_buf[i0][j] + dx_buf[i1][j] * 2.0f + dx_buf[i2][j];

            // Горизонтальное сглаживание dy
            float gy = 0.0f;
            float smooth_kernel[3] = {1.0f, 2.0f, 1.0f}; // Ядро сглаживания [1, 2, 1]

            for (int k_offset = -1; k_offset <= 1; k_offset++) // Индекс для горизонтального ядра
            {
                int current_col_for_gy = j + k_offset;

                if (current_col_for_gy < 0) current_col_for_gy = 0;
                else if (current_col_for_gy >= width) current_col_for_gy = width - 1;

                // dy_buf[i1][...] берет значения dI/dy из центральной строки окна
                gy += dy_buf[i1][current_col_for_gy] * smooth_kernel[k_offset + 1];
            }

            // Записываем модуль градиента в результирующий массив 
            output_data[(i - 1) * width + j] = to_uchar(sqrtf(gx * gx + gy * gy));
        }
    }
}

ImageProcStatus ipl_sobel_edge_detection(Image* image)
{
    if (!image || !image->data) return INVALID_ARGUMENT;

    // Для алгоритма детенции гранниц Собеля необходимо Ч/Б изображение
    // Выделяем память для копии изображения приведенного к одному каналу
    unsigned char* tmp_data = (unsigned char*)malloc(image->width * image->height * sizeof(unsigned char));
    if (!tmp_data) return OUT_OF_MEMORY;

    convert_to_one_channel(image->data, tmp_data, image->width, image->height, image->channels);

    // Выделяем память под новые данные изображения 
    unsigned char* new_data = (unsigned char*)malloc(image->width * image->height * sizeof(unsigned char));
    if (!new_data)
    {
        free(tmp_data);
        return OUT_OF_MEMORY;
    }

    convolution(tmp_data, new_data, image->width, image->height);

    free(tmp_data);
    free(image->data); // Данные будут заменены на new_data

    image->channels = 1; // теперь изображение Ч/Б
    image->data = new_data; // Записываем результат после свертки 

    return SUCCESS;
}