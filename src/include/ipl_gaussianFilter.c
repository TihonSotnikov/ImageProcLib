#include "imageproc.h"
#include <math.h>
#include <stdlib.h>

// @brief Структура ядра.
//        Симметрическое гауссово ядро с нормализацией по сумме коэффициентов.
typedef struct
{
    float* values; // Коэффициенты 
    int radius;    // Радиус ядра (в пикселях)
} Kernel; 

// @brief Выделяет память для ядра (проверка на корректное выделение находится в gaussian_filter).
//        Далее создает одномерное симметрическое (G(-i) = G(i)) гауссово ядро с нормализацией.
//        Значения хранятся в массиве values, где элементу i + radius соответствует значение ядра для G(i).
//        Значения вычисляются по формуле exp(-i^2 / (2 * sigma^2)) для i ∈ [-r, r] с последующей нормализацией на сумму всех коэффициентов.
//
// @param sigma [in] Отклонение Гауссова распределения (определяет степень сглаживания), на основании которого рассчитывается ядро.
// @return Указатель на заполненую структуру ядра.
Kernel* generate_gaussian_kernel(const float sigma)
{
    int radius = ceil(3 * sigma);
    int size = 2 * radius + 1;

    Kernel* kernel = (Kernel*)malloc(sizeof(Kernel));
    kernel->radius = radius;
    kernel->values = (float*)malloc(size * sizeof(float));

    // Генерация ядра 
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i)
    {
        float val = expf(-(i * i) / (2 * sigma * sigma));
        kernel->values[i + radius] = val; // буфер на 2 * radius + 1
        sum += val;
    }

    // Нормализация 
    for (int i = 0; i < size; ++i)
    {
        kernel->values[i] /= sum;
    }

    return kernel;
}

// @brief Освобождает память выделенную под структуру ядра.
//
// @param kernel [in] Указатель на структуру ядра.
void free_kernel(Kernel* kernel)
{
    free(kernel->values);
    free(kernel);
}

// @brief Округляет значение float суммы значений для пикселя под размеры char для последующий записи в массив данных изображения.
//
// @param value [in, out] Значение суммы для округления.
// @return Округленное значение суммы.
//         Если входное значение меньше нуля, округляет до 0 (мин. значение для char).
//         Если входное значение больше, чем может вместить char, возвращает 255 (макс. значение для char).
unsigned char to_uchar(float value)
{
    if (value < 0.0f) value = 0.0f;
    else if (value > 255.0f) value = 255.0f;
    return (unsigned char)roundf(value);
}

// @brief Проход по горизонтале изображения.
//        Рассчитываем значение каждого пикселя умножая соседние на значения ядра по смещению.
//        После рассчета значения пикселя данные записываются в выходной массив output_data.
// 
// @param input_data  [in]      Указатель на изначальный массив данных изображения. 
//                              Необходим для получения исходных данных каждого пикселя.
// @param output_data [in, out] Указатель на массив в который записываются полученные значения пикселей. 
// @param channels    [in]      Количество каналов в изображении (фактически 3-е измерение массива). 
//                              Для каждого канала значение рассчитывается отдельно. 
// @param width       [in]      Количество пикселей в ширину.
// @param height      [in]      Количество пикселей в высоту.
// @param kernel      [in]      Указатель на структуру ядра.
void horisontal_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel)
{
    for (int i = 0; i < (int)height; i++) // Итерация по каждой строке изображения 
    {
        for (int j = 0; j < (int)width; j++) // Итерация по каждому пикселю (столбцу)
        {
            for (int channel = 0; channel < channels; channel++) // Итерация по каждому цветовому каналу (свертка применяется независимо к каждому каналу)
            {
                float sum_pixel_values = 0.0f; // Аккумулятор для взвешенной суммы
                for (int offset = -kernel->radius; offset <= kernel->radius; offset++) // Итерация по смещению в ядре 
                {
                    int current_pixel_offset = j + offset; // Индекс текущего обрабатываемого пикселя со смещением по строке 

                    // Обработка пикселей на границе изображения
                    if (current_pixel_offset < 0) current_pixel_offset = 0;
                    else if (current_pixel_offset >= width) current_pixel_offset = width - 1;
                    
                    int current_pixel_index = (i * width + current_pixel_offset) * channels + channel; // Индекс текущего обрабатываемого пикселя во всем массиве 
                    float kernel_weight = kernel->values[offset + kernel->radius]; // Отображение значения ядра на индекс массива 

                    sum_pixel_values += (float)input_data[current_pixel_index] * kernel_weight;
                }
                int byte_idx = (i * width + j) * channels + channel;
                output_data[byte_idx] = to_uchar(sum_pixel_values);
            }
        }
    }
}

// @brief Проход по вертикале изображения.
//        Рассчитываем значение каждого пикселя умножая соседние (по вертикале) на значения ядра по смещению.
//        После рассчета значения пикселя данные записываются в выходной массив output_data.
// 
// @param input_data  [in]      Указатель на изначальный массив данных изображения. 
//                              Необходим для получения исходных данных каждого пикселя.
// @param output_data [in, out] Указатель на массив в который записываются полученные значения пикселей. 
// @param channels    [in]      Количество каналов в изображении (фактически 3-е измерение массива). 
//                              Для каждого канала значение рассчитывается отдельно. 
// @param width       [in]      Количество пикселей в ширину.
// @param height      [in]      Количество пикселей в высоту.
// @param kernel      [in]      Указатель на структуру ядра.
void vertical_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel)
{
    for (int i = 0; i < (int)height; i++) // Итерация по каждой строке изображения 
    {
        for (int j = 0; j < (int)width; j++) // Итерация по каждому пикселю (столбцу)
        {
            for (int channel = 0; channel < channels; channel++) // Итерация по каждому цветовому каналу (свертка применяется независимо к каждому каналу)
            {
                float sum_pixel_values = 0.0f; // Аккумулятор для взвешенной суммы
                for (int offset = -kernel->radius; offset <= kernel->radius; offset++) // Итерация по смещению в ядре 
                {
                    int current_pixel_offset = i + offset; // Индекс текущего обрабатываемого пикселя со смещением по строке 

                    // Обработка пикселей на границе изображения
                    if (current_pixel_offset < 0) current_pixel_offset = 0;
                    else if (current_pixel_offset >= height) current_pixel_offset = height - 1;
                    
                    int current_pixel_index = (current_pixel_offset * width + j) * channels + channel; // Индекс текущего обрабатываемого пикселя во всем массиве 
                    float kernel_weight = kernel->values[offset + kernel->radius]; // Отображение значения ядра на индекс массива 

                    sum_pixel_values += (float)input_data[current_pixel_index] * kernel_weight;
                }
                int byte_idx = (i * width + j) * channels + channel;
                output_data[byte_idx] = to_uchar(sum_pixel_values);
            }
        }
    }
}

// @brief Применение Гауссова фильтра к изображению с заданным отклонение гауссова распределения sigma.
//        Фильтрация осуществляется за два этапа свертки по 1D.
//        Первый этап - горизонтальная свертка.
//        Второй этап - вертикальная свертка.
// 
// @param image [in, out] Указатель на структуру изображения для применения фильтра. 
//                        Данные перезаписываются на измененное изображение.
// @param sigma [in]      Отклонение гауссова распределения на основе которого рассчитывается ядро.
//                        При слишком маленьком отклонении рассчет не осуществляется, так как он незначительно повлияет на изображение.
//
// @return INVALID_ARGUMENT В функцию передан невалидный аргумент (NULL).
// @return OUT_OF_MEMORY    Не удалось выделить память.
// @return SUCCESS          Фильтр успешно применен.
ImageProcStatus gaussian_filter(Image* image, const float sigma)
{
    if (!image || !sigma || !image->data) return INVALID_ARGUMENT;

    if (sigma <= 1e-6) return SUCCESS; // Если sigma слишком мала (изображение не изменится или изменится незначительно)

    Kernel* kernel = generate_gaussian_kernel(sigma);
    if (!kernel || !kernel->values) return OUT_OF_MEMORY; // Проверяем выделилась ли память.

    // Временная запись нужна для корректной работы функций свертки. 
    // Если во время свертки брать данные и записывать в одногом и том же изображении,
    // алгоритм будет работать некорректно, так как необходимо иметь исходные данные для рассчетов новых значений пикселей.
    int tmp_data_size = image->height * image->width * image->channels;
    unsigned char* tmp_data = (unsigned char*)malloc(tmp_data_size * sizeof(unsigned char));
    if (!tmp_data) 
    {
        free_kernel(kernel);
        return OUT_OF_MEMORY;
    }

    // Функция Гаусса G(x, y), где x и y - координаты пикселя эквивалентна произведению одномерных G(x) и G(y).
    // Это позволяет оптимизировать алгоритм. Получаем сложность O(2N) вместо O(N*N).
    horisontal_convolution(image->data, tmp_data, image->channels, image->width, image->height, kernel);
    vertical_convolution(tmp_data, image->data, image->channels, image->width, image->height, kernel);

    free_kernel(kernel);

    free(tmp_data);

    return SUCCESS;
}