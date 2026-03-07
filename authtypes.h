#ifndef AUTHTYPES_H
#define AUTHTYPES_H
#include <QString>

enum AUTH_ERRORS
{
    None,
    EmptyLogin,
    EmptyPassword,
    EmptyPasswordConfirm,
    ShortLogin,
    ShortPassword,
    PasswordMismatch,
    LoginAlreadyExists,
    ErrorsCount,
    //чета еще

};

inline QString messageForError(AUTH_ERRORS error)
{
    switch (error)
    {
    case AUTH_ERRORS::None:                 return "Успех!"; break;
    case AUTH_ERRORS::EmptyLogin:           return "Введите логин"; break;
    case AUTH_ERRORS::EmptyPassword:        return "Введите пароль"; break;
    case AUTH_ERRORS::EmptyPasswordConfirm: return "Подтвердите пароль"; break;
    case AUTH_ERRORS::ShortLogin:           return "Минимальный размер логина 3 символа!"; break;
    case AUTH_ERRORS::ShortPassword:        return "Минимальный размер пароля 6 символов!"; break;
    case AUTH_ERRORS::PasswordMismatch:     return "Пароли не совпадают!"; break;
    case AUTH_ERRORS::LoginAlreadyExists:   return "Логин уже занят!"; break;
    case AUTH_ERRORS::ErrorsCount:          break;
    }
    return "Неизвестная ошибка";
}

struct AuthResult
{
    bool ok = false;
    AUTH_ERRORS error = AUTH_ERRORS::None;
    QString message;
};






#endif // AUTHTYPES_H

