#include "authservice.h"

AuthService::AuthService(QObject *parent)
    : QObject{parent}
{

}


AuthResult AuthService::registerUser(const QString &login, const QString &password, const QString &passwordConfirm)
{
    if (login.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EmptyLogin, messageForError(AUTH_ERRORS::EmptyLogin)};
    }
    if (login.size() < 3)
    {
        return AuthResult{false, AUTH_ERRORS::ShortLogin, messageForError(AUTH_ERRORS::ShortLogin)};
    }
    if (password.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EmptyPassword, messageForError(AUTH_ERRORS::EmptyPassword)};
    }
    if (password.size() < 6)
    {
        return AuthResult{false, AUTH_ERRORS::ShortPassword, messageForError(AUTH_ERRORS::ShortPassword)};
    }
    if (passwordConfirm.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EmptyPasswordConfirm, messageForError(AUTH_ERRORS::EmptyPasswordConfirm)};
    }
    if (password != passwordConfirm)
    {
        return AuthResult{false, AUTH_ERRORS::PasswordMismatch, messageForError(AUTH_ERRORS::PasswordMismatch)};
    }
    // тут к апишке обращение надо
    return AuthResult{true, AUTH_ERRORS::None, messageForError(AUTH_ERRORS::None)};
}


AuthResult AuthService::logIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EmptyLogin, messageForError(AUTH_ERRORS::EmptyLogin)};
    }
    if (login.size() < 3)
    {
        return AuthResult{false, AUTH_ERRORS::ShortLogin, messageForError(AUTH_ERRORS::ShortLogin)};
    }
    if (password.isEmpty())
    {
        return AuthResult{false, AUTH_ERRORS::EmptyPassword, messageForError(AUTH_ERRORS::EmptyPassword)};
    }
    if (password.size() < 6)
    {
        return AuthResult{false, AUTH_ERRORS::ShortPassword, messageForError(AUTH_ERRORS::ShortPassword)};
    }
    // тут к апишке обращение надо
    return AuthResult{true, AUTH_ERRORS::None, messageForError(AUTH_ERRORS::None)};
}


AuthResult AuthService::logOut()
{
    return AuthResult{true, AUTH_ERRORS::None, messageForError(AUTH_ERRORS::None)};
}
