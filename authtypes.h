#ifndef AUTHTYPES_H
#define AUTHTYPES_H
#include <QString>

// TODO: номера ошибок от запросов
enum AUTH_ERRORS
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

QString messageForError(AUTH_ERRORS error);

struct AuthResult
{
    bool ok = false;
    AUTH_ERRORS error = AUTH_ERRORS::NO_ERROR;
    QString message;
};

const AuthResult validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm);

const AuthResult validateLogIn(const QString &login, const QString &password);






#endif // AUTHTYPES_H

