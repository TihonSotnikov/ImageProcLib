#include "imageproc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// @brief Округляет значение float и приводит его к диапазону unsigned char [0, 255].
//
// @param value [in] Исходное значение типа float для преобразования.
// @return Округленное значение, приведенное к типу unsigned char.
//         Если входное значение меньше 0, возвращает 0.
//         Если входное значение больше 255, возвращает 255.
unsigned char to_uchar(float value)
{
    if (value < 0.0f) value = 0.0f;
    else if (value > 255.0f) value = 255.0f;
    return (unsigned char)roundf(value);
}


// ------------------------------
// ---- ГАУССОВАЯ ФИЛЬТРАЦИЯ ----
// ------------------------------


// @brief Структура для представления одномерного гауссова ядра свертки.
//        Ядро симметрично и нормализовано (сумма коэффициентов равна 1).
typedef struct
{
    float* values; // Указатель на массив коэффициентов ядра.
    int radius;    // Радиус ядра (количество элементов от центра до края).
} Kernel;

// @brief Создает одномерное симметричное гауссово ядро свертки.
//        Значения вычисляются по формуле G(i) = exp(-i^2 / (2 * sigma^2))
//        для i в диапазоне [-radius, radius] и затем нормализуются.
//        Память под ядро выделяется динамически.
//
// @param sigma [in] Стандартное отклонение (sigma) гауссова распределения,
//                   определяющее степень размытия. Положительно.
// @return Указатель на созданную структуру Kernel или NULL в случае ошибки выделения памяти.
Kernel* generate_gaussian_kernel(const float sigma)
{
    // Радиус ядра выбирается как 3*sigma
    int radius = (int)ceilf(3.0f * sigma);
    // Размер ядра (диаметр + центр)
    int size = 2 * radius + 1;

    Kernel* kernel = (Kernel*)malloc(sizeof(Kernel));
    if (!kernel) return NULL;

    kernel->radius = radius;
    kernel->values = (float*)malloc(size * sizeof(float));
    if (!kernel->values) {
        free(kernel); // Освобождаем ранее выделенную память под структуру Kernel
        return NULL;
    }

    // Генерация значений ядра
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i)
    {
        float val = expf(-(float)(i * i) / (2.0f * sigma * sigma));
        // Индекс в массиве values: i смещается на radius, чтобы быть >= 0
        kernel->values[i + radius] = val;
        sum += val;
    }

    // Нормализация ядра (сумма коэффициентов должна быть равна 1)
    for (int i = 0; i < size; ++i)
    {
        kernel->values[i] /= sum;
    }

    return kernel;
}

// @brief Освобождает память, выделенную для структуры Kernel и ее данных.
//
// @param kernel [in] Указатель на структуру Kernel, которую необходимо освободить.
void free_kernel(Kernel* kernel)
{
    if (kernel) {
        free(kernel->values); // free(NULL) безопасен
        free(kernel);
    }
}

// @brief Выполняет горизонтальную свертку изображения с использованием заданного ядра.
//
// @param input_data  [in]  Указатель на массив входных данных изображения.
// @param output_data [out] Указатель на массив для записи результатов свертки.
// @param channels    [in]  Количество цветовых каналов в изображении.
// @param width       [in]  Ширина изображения в пикселях.
// @param height      [in]  Высота изображения в пикселях.
// @param kernel      [in]  Указатель на структуру Kernel, содержащую ядро свертки.
void horizontal_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel)
{
    for (size_t i = 0; i < height; i++) // Итерация по каждой строке изображения
    {
        for (size_t j = 0; j < width; j++) // Итерация по каждому пикселю (столбцу) в строке
        {
            for (int channel = 0; channel < channels; channel++) // Итерация по каждому цветовому каналу
            {
                float weighted_sum = 0.0f; // Аккумулятор для взвешенной суммы значений пикселей
                for (int offset = -kernel->radius; offset <= kernel->radius; offset++) // Итерация по элементам ядра
                {
                    // Вычисляем координату соседнего пикселя по горизонтали
                    int neighbor_col = (int)j + offset;

                    // Обработка границ: отражение (clamp to edge)
                    if (neighbor_col < 0) neighbor_col = 0;
                    else if (neighbor_col >= (int)width) neighbor_col = (int)width - 1;
                    
                    // Индекс пикселя в одномерном массиве input_data
                    size_t pixel_idx_in_data = (i * width + (size_t)neighbor_col) * channels + channel;
                    // Коэффициент ядра (значение ядра для текущего смещения)
                    float kernel_value = kernel->values[offset + kernel->radius];

                    weighted_sum += (float)input_data[pixel_idx_in_data] * kernel_value;
                }
                // Индекс для записи результата в output_data
                size_t output_pixel_idx = (i * width + j) * channels + channel;
                output_data[output_pixel_idx] = to_uchar(weighted_sum);
            }
        }
    }
}

// @brief Выполняет вертикальную свертку изображения с использованием заданного ядра.
//
// @param input_data  [in]  Указатель на массив входных данных изображения.
// @param output_data [out] Указатель на массив для записи результатов свертки.
// @param channels    [in]  Количество цветовых каналов в изображении.
// @param width       [in]  Ширина изображения в пикселях.
// @param height      [in]  Высота изображения в пикселях.
// @param kernel      [in]  Указатель на структуру Kernel, содержащую ядро свертки.
void vertical_convolution(const unsigned char* input_data, unsigned char* output_data, const int channels, const size_t width, const size_t height, const Kernel* kernel)
{
    for (size_t i = 0; i < height; i++) // Итерация по каждой строке изображения
    {
        for (size_t j = 0; j < width; j++) // Итерация по каждому пикселю (столбцу) в строке
        {
            for (int channel = 0; channel < channels; channel++) // Итерация по каждому цветовому каналу
            {
                float weighted_sum = 0.0f; // Аккумулятор для взвешенной суммы
                for (int offset = -kernel->radius; offset <= kernel->radius; offset++) // Итерация по элементам ядра
                {
                    // Вычисляем координату соседнего пикселя по вертикали
                    int neighbor_row = (int)i + offset;

                    // Обработка границ: отражение (clamp to edge)
                    if (neighbor_row < 0) neighbor_row = 0;
                    else if (neighbor_row >= (int)height) neighbor_row = (int)height - 1;
                    
                    // Индекс пикселя в одномерном массиве input_data
                    size_t pixel_idx_in_data = ((size_t)neighbor_row * width + j) * channels + channel;
                    // Коэффициент ядра
                    float kernel_value = kernel->values[offset + kernel->radius];

                    weighted_sum += (float)input_data[pixel_idx_in_data] * kernel_value;
                }
                // Индекс для записи результата в output_data
                size_t output_pixel_idx = (i * width + j) * channels + channel;
                output_data[output_pixel_idx] = to_uchar(weighted_sum);
            }
        }
    }
}

// @brief Применяет гауссов фильтр к изображению.
//        Фильтрация выполняется путем двух последовательных одномерных сверток
//        (горизонтальной и вертикальной), что эквивалентно двумерной гауссовой свертке,
//        но вычислительно эффективнее.
//
// @param image [in, out] Указатель на структуру Image, которая будет модифицирована (эффект размытия по гауссу).
// @param sigma [in]      Стандартное отклонение (sigma) для гауссова ядра.
//                        Определяет степень размытия. Должно быть положительным.
//                        Если sigma очень мало (<= 1e-6f), фильтрация пропускается, так как изображение изменится незначительно.
//
// @return INVALID_ARGUMENT В функцию передан невалидный аргумент.
//                          Если `image` или `image->data` равен NULL, или `sigma` отрицательное.
// @return OUT_OF_MEMORY    Не удалось выделить память для ядра или временного буфера.
// @return SUCCESS          Фильтр успешно применен или `sigma` слишком мало для эффекта.
ImageProcStatus ipl_gaussian_filter(Image* image, const float sigma)
{
    if (!image || !image->data) return INVALID_ARGUMENT;
    if (sigma < 0.0f) return INVALID_ARGUMENT; // Sigma не может быть отрицательной

    // Если sigma очень мала, изображение практически не изменится
    if (sigma <= 1e-6f) return SUCCESS;

    Kernel* kernel = generate_gaussian_kernel(sigma);
    if (!kernel) { // generate_gaussian_kernel вернет NULL, если не удастся выделить память
        return OUT_OF_MEMORY;
    }

    // Временный буфер для хранения результата горизонтальной свертки.
    // Это необходимо, так как вертикальная свертка должна использовать
    // полностью обработанные горизонтальные данные, а не смешанные (старые и новые).
    size_t data_size_bytes = (size_t)image->height * image->width * image->channels * sizeof(unsigned char);
    unsigned char* tmp_data = (unsigned char*)malloc(data_size_bytes);
    if (!tmp_data) 
    {
        free_kernel(kernel);
        return OUT_OF_MEMORY;
    }

    // Горизонтальная свертка: результат из image->data в tmp_data
    horizontal_convolution(image->data, tmp_data, image->channels, image->width, image->height, kernel);
    // Вертикальная свертка: результат из tmp_data в image->data (перезапись исходных данных)
    vertical_convolution(tmp_data, image->data, image->channels, image->width, image->height, kernel);

    free_kernel(kernel);
    free(tmp_data);

    return SUCCESS;
}


// -------------------------
// ---- ОПЕРАТОР СОБЕЛЯ ----
// -------------------------


// @brief Преобразует многоканальное изображение в одноканальное (оттенки серого).
//        Если изображение уже одноканальное, оно просто копируется.
//        Для 3-х или 4-х канальных изображений используется стандартная формула яркости.
//
// @param input_data  [in]  Указатель на массив входных данных многоканального изображения.
// @param output_data [out] Указатель на выходной массив для одноканального изображения (размером width * height).
// @param width       [in]  Ширина изображения в пикселях.
// @param height      [in]  Высота изображения в пикселях.
// @param channels_in [in]  Количество каналов во входном изображении.
void convert_to_one_channel(const unsigned char* input_data, unsigned char* output_data, const size_t width, const size_t height, const int channels_in)
{
    if (channels_in == 1) // Если изображение уже одноканальное (Ч/Б)
    {
        memcpy(output_data, input_data, height * width * sizeof(unsigned char));
    } 
    else if (channels_in == 3 || channels_in == 4) // RGB или RGBA
    {
        size_t num_pixels = width * height;
        for (size_t i = 0; i < num_pixels; ++i)
        {
            // Индексы каналов в исходном массиве
            size_t r_idx = i * channels_in;
            size_t g_idx = i * channels_in + 1;
            size_t b_idx = i * channels_in + 2;
            // Alpha канал (channels_in == 4) игнорируется

            float r = (float)input_data[r_idx];
            float g = (float)input_data[g_idx];
            float b = (float)input_data[b_idx];
            
            // Стандартная формула для преобразования в оттенки серого (яркость)
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;

            output_data[i] = to_uchar(gray);
        }
    }
    // Случаи с другим количеством каналов (например, 2) не обрабатываются (они ограничены функциями I/O)
}

// @brief Вычисляет магнитуду градиента изображения с помощью разделимого оператора Собеля.
//        Функция сначала вычисляет производные по X и Y (dI/dx, dI/dy) с ядром [-1, 0, 1].
//        Затем dI/dx сглаживается по вертикали ядром [1, 2, 1],
//        а dI/dy сглаживается по горизонтали ядром [1, 2, 1].
//        Используются циклические буферы для строк для оптимизации доступа к данным.
//        Результатом является карта величин градиента: sqrt(Gx^2 + Gy^2) для каждого пикселя.
// 
// @param input_grayscale_data [in]  Указатель на массив данных одноканального (grayscale) входного изображения.
// @param output_gradient_map  [out] Указатель на массив для записи карты величин градиента.
// @param width                [in]  Ширина изображения в пикселях.
// @param height               [in]  Высота изображения в пикселях.
void compute_sobel_magnitude(const unsigned char* input_grayscale_data, unsigned char* output_gradient_map, const size_t width, const size_t height)
{
    // Ядра для оператора Собеля
    const float deriv_kernel[3] = {-1.0f, 0.0f, 1.0f}; // Для производной
    const float smooth_kernel[3] = {1.0f, 2.0f, 1.0f}; // Для сглаживания

    // Циклические буферы для хранения промежуточных результатов производных по X и Y для 3-х строк.
    float dx_buf[3][width]; // dI/dx для 3-х строк пикселей подряд
    float dy_buf[3][width]; // dI/dy для 3-х строк пикселей подряд

    for (size_t i = 0; i < height; i++) // Итерация по строкам входного изображения
    {
        // Индекс текущей строки в циклическом буфере (0, 1 или 2)
        size_t current_buf_row = i % 3;

        // Вычисление горизонтальной производной (dI/dx) для текущей строки i
        for (size_t j = 0; j < width; j++) // Итерация по пикселям в строке
        {
            float sum_dx = 0.0f;
            for (int offset = -1; offset <= 1; offset++) // Применение ядра deriv_kernel по горизонтали
            {
                int current_col = (int)j + offset;
                // Обработка границ (clamp to edge)
                if (current_col < 0) current_col = 0;
                else if (current_col >= (int)width) current_col = (int)width - 1;

                size_t pixel_idx = i * width + (size_t)current_col;
                sum_dx += (float)input_grayscale_data[pixel_idx] * deriv_kernel[offset + 1];
            }
            dx_buf[current_buf_row][j] = sum_dx;
        }

        // Вычисление вертикальной производной (dI/dy) для текущей строки i
        for (size_t j = 0; j < width; j++) // Итерация по пикселям в строке
        {
            float sum_dy = 0.0f;
            for (int offset = -1; offset <= 1; offset++) // Применение ядра deriv_kernel по вертикали
            {
                int current_row = (int)i + offset;
                // Обработка границ (clamp to edge)
                if (current_row < 0) current_row = 0;
                else if (current_row >= (int)height) current_row = (int)height - 1;
                
                size_t pixel_idx = (size_t)current_row * width + j;
                sum_dy += (float)input_grayscale_data[pixel_idx] * deriv_kernel[offset + 1];
            }
            dy_buf[current_buf_row][j] = sum_dy;
        }

        // Если накоплено достаточно строк (минимум 2 предыдущие + текущая = 3 строки),
        // Вычисляем градиент для центральной из этих трех строк (т.е. для строки i-1).
        if (i < 2) continue; // Требуется как минимум 3 строки (0, 1, 2) для обработки строки 1.

        // Индексы строк в циклических буферах для окна 3x3, центрированного на строке (i-1)
        size_t prev_prev_buf_row = (i - 2) % 3; // Строка i-2
        size_t prev_buf_row = (i - 1) % 3;      // Строка i-1 (центральная для вычисления градиента)
        size_t curr_buf_row = i % 3;            // Строка i

        // Строка в выходном изображении, для которой вычисляется градиент
        size_t output_row_idx = i - 1;

        for (size_t j = 0; j < width; j++) // Итерация по пикселям в строке output_row_idx
        {
            // Gx = Сглаживание dI/dx по вертикали: [1, 2, 1]^T * [dI/dx(i-2), dI/dx(i-1), dI/dx(i)]
            float gx = dx_buf[prev_prev_buf_row][j] * smooth_kernel[0] +
                       dx_buf[prev_buf_row][j]      * smooth_kernel[1] +
                       dx_buf[curr_buf_row][j]      * smooth_kernel[2];

            // Gy = Сглаживание dI/dy по горизонтали: [dI/dy(j-1), dI/dy(j), dI/dy(j+1)] * [1, 2, 1]
            // Используем значения dI/dy из центральной строки окна (prev_buf_row, т.е. строка i-1)
            float gy = 0.0f;
            for (int offset = -1; offset <= 1; offset++)
            {
                int current_col = (int)j + offset;
                // Обработка границ (clamp to edge)
                if (current_col < 0) current_col = 0;
                else if (current_col >= (int)width) current_col = (int)width - 1;
                
                gy += dy_buf[prev_buf_row][current_col] * smooth_kernel[offset + 1];
            }

            // Магнитуда градиента
            float magnitude = sqrtf(gx * gx + gy * gy);
            output_gradient_map[output_row_idx * width + j] = to_uchar(magnitude);
        }
    }
}

// @brief Выполняет обнаружение границ на изображении с использованием оператора Собеля.
//        Изображение сначала преобразуется в оттенки серого.
//        Затем вычисляется магнитуда градиента яркости по Собелю.
//        Результат (одноканальная карта градиентов) заменяет исходные данные изображения.
//
// @param image [in, out] Указатель на структуру Image. Данные изображения будут заменены
//                        картой градиентов, а количество каналов установлено в 1.
//
// @return INVALID_ARGUMENT Невалидный аргумент.
//                          Если `image` или `image->data` равен NULL.
// @return OUT_OF_MEMORY    Не удалось выделить память.
// @return SUCCESS          Детеуция границ Собеля завершена успешно.
ImageProcStatus ipl_sobel_edge_detection(Image* image)
{
    if (!image || !image->data) return INVALID_ARGUMENT;

    // Временный буфер для одноканального (grayscale) изображения
    size_t num_pixels = (size_t)image->width * image->height;
    unsigned char* grayscale_data = (unsigned char*)malloc(num_pixels * sizeof(unsigned char));
    if (!grayscale_data) return OUT_OF_MEMORY;

    convert_to_one_channel(image->data, grayscale_data, image->width, image->height, image->channels);

    // Буфер для результата (карты градиентов).
    // Использутся calloc для инициализации нулями, так как compute_sobel_magnitude
    // не заполняет крайние пиксели (первую/последнюю строку/столбец).
    unsigned char* gradient_map_data = (unsigned char*)calloc(num_pixels, sizeof(unsigned char));
    if (!gradient_map_data)
    {
        free(grayscale_data);
        return OUT_OF_MEMORY;
    }

    compute_sobel_magnitude(grayscale_data, gradient_map_data, image->width, image->height);

    free(grayscale_data); // Временные данные в оттенках серого больше не нужны
    free(image->data);    // Освобождаем старые данные изображения

    image->channels = 1;             // Теперь изображение одноканальное
    image->data = gradient_map_data; // Заменяем данные на карту градиентов

    return SUCCESS;
}