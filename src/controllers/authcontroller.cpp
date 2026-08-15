#include "controllers/authcontroller.h"

AuthController::AuthController(QObject *parent)
    : BaseController{parent}
    , authService(new AuthService(this))
{
    // Прокидываем сигналы от AuthService'а
    connect(authService, &AuthService::registrationFinished, this, &AuthController::on_RegistrationFinished);
    connect(authService, &AuthService::logInFinished, this, &AuthController::on_LogInFinished);
    connect(authService, &AuthService::logOutFinished, this, &AuthController::on_LogOutFinished);
    connect(authService, &AuthService::registrationInProgress, this, &AuthController::registrationInProgress);
    connect(authService, &AuthService::logInProgress, this, &AuthController::logInProgress);
    connect(authService, &AuthService::logOutInProgress, this, &AuthController::logOutInProgress);
    connect(authService, &AuthService::refreshAccessTokenInProgress, this, &AuthController::refreshAccessTokenInProgress);
    connect(authService, &AuthService::refreshAccessTokenFinished, this, &AuthController::on_RefreshAccessTokenFinished);


}

void AuthController::on_RegistrationFinished(const NetworkResult& res, const QString& accToken, const QString& refToken)
{
    emit registrationFinished(res, accToken, refToken);
}

void AuthController::on_LogInFinished(const NetworkResult& res, const QString& accToken, const QString& refToken)
{
    emit logInFinished(res, accToken, refToken);
}

void AuthController::on_LogOutFinished(const NetworkResult& res)
{
    emit logOutFinished(res);
}

void AuthController::on_RefreshAccessTokenFinished(const NetworkResult& res, RetryableRequest req,
    const QString& accToken, const QString& refToken)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        req.retryCount = errorCount;
        emit errorOccurred(res, req);
    }
    emit refreshAccessTokenFinished(res, req, accToken, refToken);
}

NetworkResult AuthController::validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm)
{
    if (login.isEmpty())
    {
        return NetworkResult{false, ERROR_TYPES::EMPTY_LOGIN, generateMessageForError(ERROR_TYPES::EMPTY_LOGIN)};

    }
    if (login.size() < 3)
    {
        return NetworkResult{false, ERROR_TYPES::SHORT_LOGIN, generateMessageForError(ERROR_TYPES::SHORT_LOGIN)};
    }
    if (password.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD, generateMessageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_PASSWORD, generateMessageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    if (passwordConfirm.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD_CONFIRM, generateMessageForError(ERROR_TYPES::EMPTY_PASSWORD_CONFIRM)});
    }
    if (password != passwordConfirm)
    {
        return (NetworkResult{false, ERROR_TYPES::PASSWORD_MISMATCH, generateMessageForError(ERROR_TYPES::PASSWORD_MISMATCH)});
    }
    return (NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
}

NetworkResult AuthController::validateLogIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_LOGIN, generateMessageForError(ERROR_TYPES::EMPTY_LOGIN)});
    }
    if (login.size() < 3)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_LOGIN, generateMessageForError(ERROR_TYPES::SHORT_LOGIN)});
    }
    if (password.isEmpty())
    {
        return (NetworkResult{false, ERROR_TYPES::EMPTY_PASSWORD, generateMessageForError(ERROR_TYPES::EMPTY_PASSWORD)});
    }
    if (password.size() < 6)
    {
        return (NetworkResult{false, ERROR_TYPES::SHORT_PASSWORD, generateMessageForError(ERROR_TYPES::SHORT_PASSWORD)});
    }
    return (NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
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

void AuthController::requestRefreshAccessToken(const QString &refToken, RetryableRequest req)
{
    authService->refreshAccessToken(refToken, req);
}
