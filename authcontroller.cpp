#include "authcontroller.h"

AuthController::AuthController(QObject *parent)
    : QObject{parent}
    , authService(new AuthService(this))
{
    // Прокидываем сигналы от AuthService'а
    connect(authService, &AuthService::registrationFinished, this, &AuthController::registrationFinished);
    connect(authService, &AuthService::logInFinished, this, &AuthController::logInFinished);
    connect(authService, &AuthService::logOutFinished, this, &AuthController::logOutFinished);
    connect(authService, &AuthService::registrationInProgress, this, &AuthController::registrationInProgress);
    connect(authService, &AuthService::logInProgress, this, &AuthController::logInProgress);
    connect(authService, &AuthService::logOutInProgress, this, &AuthController::logOutInProgress);
    connect(authService, &AuthService::refreshAccessTokenInProgress, this, &AuthController::refreshAccessTokenInProgress);
    connect(authService, &AuthService::refreshAccessTokenFinished, this, &AuthController::refreshAccessTokenFinished);


}
NetworkResult AuthController::validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{
    if (login.isEmpty())
    {
        return NetworkResult{false, ERROR_TYPES::EMPTY_LOGIN, messageForError(ERROR_TYPES::EMPTY_LOGIN)};

    }
    if (login.size() < 3)
    {
        return NetworkResult{false, ERROR_TYPES::SHORT_LOGIN, messageForError(ERROR_TYPES::SHORT_LOGIN)};
    }
    if (password.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD, messageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_PASSWORD, messageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    if (passwordConfirm.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD_CONFIRM, messageForError(ERROR_TYPES::EMPTY_PASSWORD_CONFIRM)});
    }
    if (password != passwordConfirm)
    {
        return (NetworkResult{false, ERROR_TYPES::PASSWORD_MISMATCH, messageForError(ERROR_TYPES::PASSWORD_MISMATCH)});
    }
    return (NetworkResult{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)});
}

NetworkResult AuthController::validateLogIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_LOGIN, messageForError(ERROR_TYPES::EMPTY_LOGIN)});
    }
    if (login.size() < 3)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_LOGIN, messageForError(ERROR_TYPES::SHORT_LOGIN)});
    }
    if (password.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD, messageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_PASSWORD, messageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    return (NetworkResult{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)});
}

void AuthController::requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{

    // Проверка входных на соответсвие бизнес-правилам
    auto res = validateRegistration(login, password, passwordConfirm);
    if (res.ok)
        authService->registerUser(login, password);
    else
        emit registrationFinished(res);
}

void AuthController::requestLogIn(const QString &login, const QString &password)
{

    // Проверка входных на соответсвие бизнес-правилам
    auto res = validateLogIn(login, password);
    if (res.ok)
        authService->logIn(login, password);
    else
        emit logInFinished(res);

}

void AuthController::requestLogOut(const QString &accToken, const QString &refToken)
{
    authService->logOut(accToken, refToken);
}

void AuthController::requestRefreshAccessToken(const QString &refToken)
{
    authService->refreshAccessToken(refToken);
}
