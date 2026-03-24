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
    connect(authService, &AuthService::refreshAccessTokenInProgress, this, &AuthController::RefreshAccessTokenInProgress);
    connect(authService, &AuthService::refreshAccessTokenFinished, this, &AuthController::RefreshAccessTokenFinished);


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

void AuthController::requestLogOut(const QString &refToken)
{
    authService->logOut(refToken);
}

void AuthController::requestRefreshAccessToken(const QString &refToken)
{
    authService->refreshAccessToken(refToken);
}
