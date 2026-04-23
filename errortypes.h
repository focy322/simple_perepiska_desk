#ifndef ERRORTYPES_H
#define ERRORTYPES_H
#include <QString>

// TODO: номера ошибок от запросов
enum ERROR_TYPES
{
    NO_ERROR,
    UNKNOWN_ERROR,
    EMPTY_LOGIN,
    EMPTY_PASSWORD,
    EMPTY_PASSWORD_CONFIRM,
    SHORT_LOGIN,
    SHORT_PASSWORD,
    PASSWORD_MISMATCH,
    LOGIN_ALREADY_EXISTS,
    ERRORS_COUNTS,                          //!< Кол-во ошибок
    // TODO: чета еще

};

QString messageForError(ERROR_TYPES error);

struct NetworkResult
{
    bool ok = false;
    ERROR_TYPES error = ERROR_TYPES::NO_ERROR;
    QString message;
};








#endif // ERRORTYPES_H

