#include "authcontroller.h"

AuthController::AuthController(QObject *parent)
    : QObject{parent}
    , authService(new AuthService(this))
{
    connect(authService, &AuthService::registrationFinished, this, &AuthController::registrationFinished);
    connect(authService, &AuthService::logInFinished, this, &AuthController::logInFinished);
    connect(authService, &AuthService::logOutFinished, this, &AuthController::logOutFinished);
}

void AuthController::requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{

    //TODO: Можно какую нибудь функцию validateRegistration & validateLogIn чтобы сразу ошибку прокидвать без дергания service
    // и также в service ее вызывать на всякий
    authService->registerUser(login, password, passwordConfirm);

}

void AuthController::requestLogIn(const QString &login, const QString &password)
{

    //TODO: Можно какую нибудь функцию validateRegistration & validateLogIn чтобы сразу ошибку прокидвать без дергания service
    // и также в service ее вызывать на всякий
    authService->logIn(login, password);

}

void AuthController::requestLogOut()
{
    authService->logOut();
}
