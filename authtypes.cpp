#include "authtypes.h"

QString messageForError(AUTH_ERRORS error)
{
    switch (error)
    {
    case AUTH_ERRORS::NO_ERROR:               return "Успех!"; break;
    case AUTH_ERRORS::EMPTY_LOGIN:            return "Введите логин"; break;
    case AUTH_ERRORS::EMPTY_PASSWORD:         return "Введите пароль"; break;
    case AUTH_ERRORS::EMPTY_PASSWORD_CONFIRM: return "Подтвердите пароль"; break;
    case AUTH_ERRORS::SHORT_LOGIN:            return "Минимальный размер логина 3 символа!"; break;
    case AUTH_ERRORS::SHORT_PASSWORD:         return "Минимальный размер пароля 6 символов!"; break;
    case AUTH_ERRORS::PASSWORD_MISMATCH:      return "Пароли не совпадают!"; break;
    case AUTH_ERRORS::LOGIN_ALREADY_EXISTS:   return "Логин уже занят!"; break;
    case AUTH_ERRORS::UNKNOWN_ERROR:          return "Неизвестная ошибка";break;
    default:                                  return "Неизвестная ошибка";break;
    }
    return "Неизвестная ошибка";
}


const AuthResult validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{
    if (login.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EMPTY_LOGIN, messageForError(AUTH_ERRORS::EMPTY_LOGIN)};

    }
    if (login.size() < 3)
    {
        return AuthResult{false, AUTH_ERRORS::SHORT_LOGIN, messageForError(AUTH_ERRORS::SHORT_LOGIN)};
    }
    if (password.isEmpty())
    {
        return (AuthResult{false, AUTH_ERRORS::EMPTY_PASSWORD, messageForError(AUTH_ERRORS::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (AuthResult{false, AUTH_ERRORS::SHORT_PASSWORD, messageForError(AUTH_ERRORS::SHORT_PASSWORD)});
    }
    if (passwordConfirm.isEmpty())
    {
        return (AuthResult{false, AUTH_ERRORS::EMPTY_PASSWORD_CONFIRM, messageForError(AUTH_ERRORS::EMPTY_PASSWORD_CONFIRM)});
    }
    if (password != passwordConfirm)
    {
        return (AuthResult{false, AUTH_ERRORS::PASSWORD_MISMATCH, messageForError(AUTH_ERRORS::PASSWORD_MISMATCH)});
    }
    return (AuthResult{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)});
}

const AuthResult validateLogIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        return (AuthResult{false, AUTH_ERRORS::EMPTY_LOGIN, messageForError(AUTH_ERRORS::EMPTY_LOGIN)});
    }
    if (login.size() < 3)
    {
        return (AuthResult{false, AUTH_ERRORS::SHORT_LOGIN, messageForError(AUTH_ERRORS::SHORT_LOGIN)});
    }
    if (password.isEmpty())
    {
        return (AuthResult{false, AUTH_ERRORS::EMPTY_PASSWORD, messageForError(AUTH_ERRORS::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (AuthResult{false, AUTH_ERRORS::SHORT_PASSWORD, messageForError(AUTH_ERRORS::SHORT_PASSWORD)});
    }
    return (AuthResult{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)});
}
