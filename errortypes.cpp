#include "errortypes.h"

QString messageForError(ERROR_TYPES error)
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


AuthResult validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{
    if (login.isEmpty())
    {
        return AuthResult{false, ERROR_TYPES::EMPTY_LOGIN, messageForError(ERROR_TYPES::EMPTY_LOGIN)};

    }
    if (login.size() < 3)
    {
        return AuthResult{false, ERROR_TYPES::SHORT_LOGIN, messageForError(ERROR_TYPES::SHORT_LOGIN)};
    }
    if (password.isEmpty())
    {
        return (AuthResult{false, ERROR_TYPES::EMPTY_PASSWORD, messageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (AuthResult{false, ERROR_TYPES::SHORT_PASSWORD, messageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    if (passwordConfirm.isEmpty())
    {
        return (AuthResult{false, ERROR_TYPES::EMPTY_PASSWORD_CONFIRM, messageForError(ERROR_TYPES::EMPTY_PASSWORD_CONFIRM)});
    }
    if (password != passwordConfirm)
    {
        return (AuthResult{false, ERROR_TYPES::PASSWORD_MISMATCH, messageForError(ERROR_TYPES::PASSWORD_MISMATCH)});
    }
    return (AuthResult{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)});
}

AuthResult validateLogIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        return (AuthResult{false, ERROR_TYPES::EMPTY_LOGIN, messageForError(ERROR_TYPES::EMPTY_LOGIN)});
    }
    if (login.size() < 3)
    {
        return (AuthResult{false, ERROR_TYPES::SHORT_LOGIN, messageForError(ERROR_TYPES::SHORT_LOGIN)});
    }
    if (password.isEmpty())
    {
        return (AuthResult{false, ERROR_TYPES::EMPTY_PASSWORD, messageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (AuthResult{false, ERROR_TYPES::SHORT_PASSWORD, messageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    return (AuthResult{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)});
}
