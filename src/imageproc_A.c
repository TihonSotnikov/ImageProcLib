#include "imageproc.h"
#include <stdlib.h>
#include <omp.h>

// FOR DEBUG //
#include "input_output.h"

static inline int clamp(int val, int min_val, int max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

// unsigned char *create_padded_copy(char *source, int radius)
// {

// }

// @brief Применяет к изображению медианный фильтр.
// @param image Указатель на структуру изображения. [in/out]
// @param radius Радиус фильтра. Чем больше число, тем сильнее размытие.
ImageProcStatus ipl_median_filter(Image *image, const int radius)
{
    unsigned char *source = image->data;
    int win_size = radius * 2 + 1;
    int win_area = win_size * win_size;

    int width = image->width;           // Ширина оригинального изображения в пикселях
    int height = image->height;         // Высота оригинального изображения в пикселях
    int chan = image->channels;         // Число каналов
    int wc = width * chan;              // Байтовая ширина строки исходного изображения

    int w_pad = image->width + radius * 2;          // Ширина холста, дополненного 2-мя радиусами
    int wc_pad = w_pad * chan;                      // Байтовая ширина строки дополненного холста (Leading Dimension)
    int h_pad = image->height + radius * 2;    // Высота дополненного холста
    int size_pad = w_pad * h_pad;              // Кол-во пикселей на холсте

    //////////////////////////////////////////////////////////
    // Создание дополненной копии оригинального изображения //
    //////////////////////////////////////////////////////////

    printf("Making a padded copy.\n");

    unsigned char* padded = malloc(w_pad * h_pad * chan);
    if (padded == NULL) return OUT_OF_MEMORY;

    int c;
    #pragma omp parallel for
    for (c = 0; c < chan; c++)
    {
        int i, j;
        for (i = 0; i < h_pad; i++)
        {
            int i_src = clamp(i - radius, 0, height-1);
            int offset_src = i_src * wc;
            int offset_pad = i * wc_pad;
            for (j = 0; j < w_pad; j++)
            {
                int j_src = clamp(j - radius, 0, width-1);
                padded[offset_pad + j*chan + c] = source[offset_src + j_src*chan + c];
            }
        }
    }

    // Image pdd;
    // pdd.channels = image->channels;
    // pdd.format = image->format;
    // pdd.height = h_pad;
    // pdd.width = w_pad;
    // pdd.data = padded;
    // ipl_save_image("padded.jpeg", &pdd, JPEG);
    
    ////////////////////////////////////////////////
    // Применение медианнго фильтра к изображению //
    ////////////////////////////////////////////////

    int histogram[4][256];
    int hpos_x[4];
    int hpos_y[4];
    
    printf("Applying the median filter.\n");
    
    // Количество каналов варьируется от 1 до 4, поэтому c можно считать за константу
    #pragma omp parallel for
    for (int c = 0; c < chan; c++)
    {
        for (int i = 0; i < height; i++)
        {
            // printf("Row %d/%d...\n", i, height-1);
            hist_set(histogram[c], source, padded, i, wc_pad, 0, chan, c, win_size, &hpos_y[c], &hpos_x[c]);

            int i_offset = i * wc;
            for (int j = 0; j < width-1; j++)
            {
                source[i_offset + j*chan + c] = get_median(histogram[c], win_area);
                // if (c == 2) printf("hpos_x = %d, hpos_y = %d\n", hpos_x[c], hpos_y[c]);
                hist_move(histogram[c], source, padded, hpos_y[c], wc_pad, &hpos_x[c], chan, c, win_size);
            }
            source[i_offset + (width-1)*chan + c] = get_median(histogram[c], win_area);
        }
    }

    printf("Median filter finished.\n");
    free(padded);
    
    return SUCCESS;
}

// @brief Сброс и заполнение гистограммы для пикселя ппо координатам
// @param hist Указалель на гистограмму (массив unsigned char[256])
// @param padded Указатель на дополненное изображение
// @param y Координата y в оригинальном изображении (по вертикали)
// @param ldp Байтовая ширина строки изображения, для правильной индексации
// @param x Координата x в оригинальном изображении (по горизонтали)
// @param ch Количество каналов изображения (1-4)
// @param c Выбранный канал
// @param window Радиус фильтра
void hist_set(int *hist, unsigned char *original, unsigned char *padded, int y, int ldp, int x, int channels, int c, int window, int *hy, int *hx)
{
    for (int h = 0; h < 256; h++)
    {
        hist[h] = 0;
    }
    
    for (int i = y; i < y+window; i++)
    {
        for (int j = x; j < x+window; j++)
        {
            hist[padded[i*ldp + j*channels + c]]++;
        }
    }
    *hy = y;
    *hx = x;
}

void hist_move(int *hist, unsigned char *original, unsigned char *padded, int y, int ldp, int *hx, int channels, int c, int win_s)
{
    int col_to_remove_idx = *hx;
    for (int i_win = 0; i_win < win_s; i_win++) {
        int current_row_padded = y + i_win;
        hist[padded[current_row_padded * ldp + col_to_remove_idx * channels + c]]--;
    }

    (*hx)++;

    int col_to_add_idx = (*hx) + win_s - 1;
    for (int i_win = 0; i_win < win_s; i_win++) {
        int current_row_padded = y + i_win;
        hist[padded[current_row_padded * ldp + col_to_add_idx * channels + c]]++;
    }
}

unsigned char get_median(int *hist, int win_sqr)
{
    int cnt = 0;
    int half = win_sqr / 2;
    for (int i = 0; i < 256; i++)
    {
        cnt += hist[i];
        if (cnt > half) return (unsigned char)i;
    }
    return 255;
}

ImageProcStatus ipl_grayscale(Image *image)
{
    unsigned char *output_data = malloc(image->width * image->height);
    if (output_data == NULL) return OUT_OF_MEMORY;

    convert_to_one_channel(image->data, output_data, image->width, image->height, image->channels);
    free(image->data);
    image->data = output_data;
    image->channels = GRAYSCALE;
    image->format = JPEG;

    return SUCCESS;
}