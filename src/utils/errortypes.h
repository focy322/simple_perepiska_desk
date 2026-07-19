#ifndef ERRORTYPES_H
#define ERRORTYPES_H
#include <QString>

/**
 * @brief Перечисление возможных ошибок в приложении (в основном сетевых и валидации)
 */
// TODO: номера ошибок от запросов
enum ERROR_TYPES
{
    NO_ERROR,               //!< Успех! (Нет ошибки)
    UNKNOWN_ERROR,          //!< Неизвестная ошибка
    EMPTY_LOGIN,            //!< Пустой логин
    EMPTY_PASSWORD,         //!< Пустой пароль
    EMPTY_PASSWORD_CONFIRM, //!< Пустое подтверждение пароля
    SHORT_LOGIN,            //!< Слишком короткий логин (менее 3 символов)
    SHORT_PASSWORD,         //!< Слишком короткий пароль (менее 6 символов)
    PASSWORD_MISMATCH,      //!< Пароли не совпадают
    LOGIN_ALREADY_EXISTS,   //!< Логин уже занят
    ERRORS_COUNTS,          //!< Кол-во ошибок
    // TODO: Дополнить

};

/**
 * @brief Генерирует человекочитаемое сообщение для указанной ошибки
 * @param error Тип ошибки
 * @return Строка с описанием ошибки
 */
QString generateMessageForError(ERROR_TYPES error);

/**
 * @brief Структура для хранения результата сетевого запроса
 */
struct NetworkResult
{
    bool        ok      = false;                 //!< Флаг успешного выполнения
    ERROR_TYPES error   = ERROR_TYPES::NO_ERROR; //!< Код ошибки (если есть)
    QString     message = "";                    //!< Дополнительное сообщение об ошибке
};

#endif // ERRORTYPES_H

