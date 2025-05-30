#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include "input_output.h"
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

// @brief Контекст операции записи файла для использования с функциями stb_image_write.
//        Эта структура передается как void* context в write_to_file_contextual.
//        Для отслеживания ошибок записи в файл.
typedef struct
{
    FILE* file;   // указатель на открытый файловый поток для операций ввода-вывода 
    int io_error; // флаг ошибки ввода-вывода: 0 - нет ошибок, 1 - наличие ошибки
} WriteOperationContext;

// @brief Пользовательская функция для обратного вызова в stb_image_write.
//        Записывает предоставленный блок данных в файл, используя контекст операции.
//        Устанавливает флаг io_error при ошибке fwrite
//
// @param context [in,out] Указатель на структуру WriteOperationContext.
// @param data    [in]     Указатель на данные для записи.
// @param size    [in]     Количество байт которые нужно записать.
void write_to_file_contextual(void *context, void *data, int size)
{
    WriteOperationContext* op_context = (WriteOperationContext*)context;

    // stbi_write всегда вызывает write_to_file_contextual для всех блоков.
    // Но если уже была зафиксирована ошибка в операциях записи, 
    // дальнейшие попытки будут бессмысленны.
    if (op_context->io_error) return; // Ошибка уже произошла, ничего не делаем.

    size_t written_size = fwrite(data, 1, size, op_context->file);

    // Если количество записаных байт отличается от изначального размера блока,
    // это ошибка записи (не все байты были записаны)
    if ((size_t)size != written_size)
    {
        op_context->io_error = 1; // поднимаем флаг ошибки 
    }
    // если запись успешна (size == written_size), io_error остается в 0 (если не был ранее 1)
}

// @brief Освобождает память, выделенную для пиксельных данных изображения.
//
// @param image [in,out] Указатель на структуру Image
// 
// @return INVALID_ARGUMENT image или image->data равен NULL
// @return SUCCESS          В случае успешного освобождения 
ImageProcStatus free_image_data(Image* image)
{
    // Проверка на валидность указателя на структуру и на данные изображения 
    if (!image || !image->data) return INVALID_ARGUMENT;

    stbi_image_free(image->data); // Освобождаем память с помощью имеющийся функции из stb_image
    image->data = NULL;                                  // Обнуляем указатель для предотвращения висячих ссылок 

    return SUCCESS;
}

// @brief Загружает изображение из файла в структуру Image.
//        Предварительно освобождает потенциальные мусорные данные из Image.
//        Поддерживает форматы файла PNG и JPEG. Если указан UNKNOWN, функция возвращает ошибку.
// 
// @param file_name   [in]  Строковое значение пути к файлу хранящему изображение.
// @param image       [out] Указатель на структуру Image, которая будет заполнена данными загруженного изображения.
//                          Память для image->data выделяется в stbi_load_from_file и далее освобождается с помощью free_image_data.
// @param file_format [in]  Ожидаемый формат изображения в файле (PNG или JPEG).
//                          Если формат UNKNOWN, функция вернет UNSUPPORTED_FORMAT.
//
// @return INVALID_ARGUMENT   Указатели на file_name или !image равны NULL.
//                            format является неопределенным в структурах форматом.
// @return UNSUPPORTED_FORMAT Формат изображения UNKNOWN.
//                            Количество цветовых каналов после загрузки изображения с помощью stbi_load_from_file
//                            Не поддерживается 
// @return FILE_NOT_FOUND     Файл после открытия равен NULL.
// @return FILE_READ          Указатель на данные файла равен NULL.
// @return SUCCESS            Изображение было успешно считано из файла и записано в структуру.
ImageProcStatus load_image(const char* file_name, Image* image, ImageFormat file_format)
{   
    free_image_data(image); // Очищаем от потенциальных мусорных данных

    if (!file_name || !image || (file_format != PNG && file_format != JPEG && file_format != UNKNOWN)) return INVALID_ARGUMENT;

    if (file_format == UNKNOWN)
    {
        return UNSUPPORTED_FORMAT;
    }

    // Открываем файл в бинарном режиме для чтения для дальнейшей работы с ним с помощью stbi_load_from_file
    FILE* file = fopen(file_name, "rb");
    if (!file) return FILE_NOT_FOUND;

    int width, height, channels;
    // Загружаем данные с помощью stb_image.
    // Последний параметр 0 означает, что количество каналов определяется из файла.
    unsigned char *data = stbi_load_from_file(file, &width, &height, &channels, 0);

    // если stbi_load_from_file вернула NULL, произошла ошибка загрузки / декодирования 
    if (!data)
    {   
        fclose(file);
        return FILE_READ;
    }

    // Проверка на поддерживаемое количество каналов 
    if (channels != 1 && channels != 3 && channels != 4)
    {
        stbi_image_free(data); // Очищаем память, выделенную stbi_load_from_file, так как формат не подходит 
        fclose(file);
        return UNSUPPORTED_FORMAT;
    }
    
    // Заполнение структуры Image данными загружаемого изображения 
    image->format = file_format;
    image->width = (size_t)width;
    image->height = (size_t)height;
    image->channels = channels; // Неявное преобразование Int в ImageColorChannels.
                                // Неподдерживаемые значения отсекаются проверкой выше.
    image->data = data;

    fclose(file);

    return SUCCESS;
}

// @brief Сохраняет изображение из структуры Image в файл.
//        Далее свобождает память структуры.
//        В случае ошибок записи также удаляет поврежденный файл.
//
// @param file_name   [in] Строковое значение пути к файлу в который нужно сохранить изображение.
// @param image       [in] Указатель на структуру изоражения сохраняемого в файл.
// @param file_format [in] Формат файла.
//                         При несовпадении формата изображения и формата файла возвращается ошибка.
//
// @return INVALID_ARGUMENT   Указатели на file_name, image равны NULL.
//                            Указатель на image->data равен NULL.
//                            Формат изображения не соответствует форматам определенным в структуре.
// @return UNSUPPORTED_FORMAT Формат изображения не поддерживается (UNKNOWN).
// @return FILE_NOT_FOUND     fopen вернул нулевой указатель.
// @return FILE_WRITE         Ошибка при записи данных в файл.
//                            Функция write_to_file_contextual передаваемая в stbi_write записала не все байты.
// @return INTERNAL           Внутренняя ошибка в stbi_write.
//                            Может возникать если формат изображения image не совпадает с форматом файла.
// @return SUCCESS            Изображение успешно сохранено в файл, память освобождена.
//
// @note В случае любой ошибки (кроме INVALID_ARGUMENT и UNSUPPORTED_FORMAT) потенциально поврежденный файл будет удален 
ImageProcStatus save_image(const char* file_name, Image* image, ImageFormat file_format)
{
    if (!file_name || !image || !image->data || (file_format != PNG && file_format != JPEG && file_format != UNKNOWN)) return INVALID_ARGUMENT;

    if (file_format == UNKNOWN)
    {   
        free_image_data(image); // в случае некорректного формата изображения освобождаем его память 
        return UNSUPPORTED_FORMAT;
    }

    // Открываем файл в режиме бинарного чтения для последующей записи изображения в него с помощью stbi_write
    FILE* file = fopen(file_name, "wb");

    if (!file)
    {
        free_image_data(image);
        return FILE_NOT_FOUND;
    }

    // Создаем контекст операции записи в файл
    WriteOperationContext op_ctx;
    op_ctx.file = file;
    op_ctx.io_error = 0; // Пока что считаем, что ошибок не было

    int stb_res = 1; // Флаг для отслеживания корректности записи stbi_write (пока что считаем, что ошибок не было)
                     // 1 - нет ошибок 
                     // 2 - наличие ошибки 

    if (file_format == PNG)
    {
        stb_res = stbi_write_png_to_func(
            write_to_file_contextual, 
            &op_ctx, 
            image->width, 
            image->height, 
            image->channels, 
            image->data, 
            image->width * image->channels // Шаг строки в байтах
        );                                               // Указывает на количество байт между началом одной строки пиксельных данных и началом следующей строки в памяти
    }
    ////////////////////////////////////////////////////////////////////////////////////
    // В stb JPEG имеет 3 канала, возможные некорректные записи в файл при GRAYSCALE. //
    // Если будут баги при сохранении, скорее всего ошибка в этом.                    //
    ////////////////////////////////////////////////////////////////////////////////////
    else if (file_format == JPEG)
    {
        stb_res = stbi_write_jpg_to_func(
            write_to_file_contextual, 
            &op_ctx, 
            image->width, 
            image->height, 
            image->channels, 
            image->data, 
            100 // Параметр JPEG качества 
        );
    }

    // Если произошла ошибка в write_to_file_contextual
    // Это ошибка записи в файл
    // освобождаем память image, закрываем и удаляем поврежденный файл перед возвратом 
    if (op_ctx.io_error == 1)
    {
        free_image_data(image);
        fclose(file);
        remove(file_name);
        return FILE_WRITE;
    }

    // Внутренняя ошибка в stbi_write
    // Также освобождаем память, закрываем и удаляем файл
    if (stb_res == 0)
    {
        free_image_data(image);
        fclose(file);
        remove(file_name);
        return INTERNAL;
    }

    fclose(file);

    free_image_data(image);

    return SUCCESS;
}

int main()
{
    Image* tst_image = (Image*)malloc(sizeof(Image));
    tst_image->data = NULL;

    ImageProcStatus status = load_image("99px_ru_wallpaper_369584_friren__frieren_i_fern__fern_devushki.jpg", tst_image, JPEG);

    printf("%d\n", status);

    status = gaussian_filter(tst_image, 100);

    printf("%d\n", status);

    status = save_image("imageeee.jpg", tst_image, JPEG);

    printf("%d\n", status);

    return 0;
}