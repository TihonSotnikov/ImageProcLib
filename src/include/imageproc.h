#ifndef PROC_H
#define PROC_H

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
    // size_t stride; // мб понадобится, но не факт - фактическая длина строки изображения (изображения могут быть выровнены по опред. границе памяти). Желательно не использовать.
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

#endif 
