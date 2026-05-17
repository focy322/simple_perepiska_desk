#include "errortypes.h"

QString generateMessageForError(ERROR_TYPES error)
{
    switch (error)
    {
    case ERROR_TYPES::NO_ERROR:               return "Успех!"; break;
    case ERROR_TYPES::EMPTY_LOGIN:            return "Введите логин"; break;
    case ERROR_TYPES::EMPTY_PASSWORD:         return "Введите пароль"; break;
    case ERROR_TYPES::EMPTY_PASSWORD_CONFIRM: return "Подтвердите пароль"; break;
    case ERROR_TYPES::SHORT_LOGIN:            return "Минимальный размер логина 3 символа!"; break;
    case ERROR_TYPES::SHORT_PASSWORD:         return "Минимальный размер пароля 6 символов!"; break;
    case ERROR_TYPES::PASSWORD_MISMATCH:      return "Пароли не совпадают!"; break;
    case ERROR_TYPES::LOGIN_ALREADY_EXISTS:   return "Логин уже занят!"; break;
    case ERROR_TYPES::UNKNOWN_ERROR:          return "Неизвестная ошибка";break;
    default:                                  return "Неизвестная ошибка";break;
    }
    return "Неизвестная ошибка";
}



