#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include "input_output.h"
#include "imageproc.h"

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
//        Изображение хранится в row-major порядке.
//        Если в изображении больше одного канала, значения каждого канала для определенного пикселя хранится построчно, подряд.
//        Пример хранения данных (для RGB): R1G1B1, R2G2B2 ... R9G9B9, R10G10B10 ...
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
ImageProcStatus ipl_load_image(const char* file_name, Image* image, const ImageFormat file_format)
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
ImageProcStatus ipl_save_image(const char* file_name, Image* image, const ImageFormat file_format)
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