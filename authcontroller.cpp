#include "authcontroller.h"

AuthController::AuthController(QObject *parent)
    : QObject{parent}
    , authService(new AuthService(this))
{

}

void AuthController::requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{

    AuthResult res = authService->registerUser(login, password, passwordConfirm);
    emit registrationFinished(res);

}

void AuthController::requestLogIn(const QString &login, const QString &password)
{

    AuthResult res = authService->logIn(login, password);
    emit logInFinished(res);

}

void AuthController::requestLogOut()
{
    AuthResult res = authService->logOut();
    emit loggedOut(res);
}
