#ifndef USERINFOCONTROLLER_H
#define USERINFOCONTROLLER_H


#include "base_controller.h"
#include "services/userinfoservice.h"

/**
 * Контроллер для работы с информацией о пользователях.
 * Принимает запросы от UI, вызывает методы UserInfoService и возвращает результат через сигналы.
 */
class UserInfoController : public BaseController
{
    Q_OBJECT
public:
    explicit UserInfoController(QObject *parent = nullptr);

    /**
     * Запрашивает информацию о текущем авторизованном пользователе.
     * \param accToken токен доступа (Access Token)
     * \param reReq
     * \param reReq
     */
    void requestMyUserInfo(const QString &accToken, RetryableRequest reReq);

    /**
     * Запрашивает информацию о другом пользователе по его ID.
     * \param accToken токен доступа (Access Token)
     * \param userId идентификатор пользователя
     * \param reReq
     * \param reReq
     */
    void requestUserInfo(const QString &accToken, unsigned long long userId, RetryableRequest reReq) const;

    /**
     * Отправляет запрос на обновление аватара текущего пользователя.
     * \param accToken токен доступа (Access Token)
     * \param imageData бинарные данные изображения аватара
     * \param reReq
     * \param reReq
     */
    void requestUploadAvatar(const QString &accToken, const QByteArray &imageData, RetryableRequest reReq);

    /**
     * Запрашивает поиск пользователей по введенной строке.
     * \param accessToken токен доступа (Access Token)
     * \param arg
     */
    void requestFindUser(const QString &accessToken, const QString &arg) const;

signals:
    // --- Сигналы процессов ---
    
    void getMyUserInfoInProgress();       //!< Сигнал о начале загрузки информации о текущем пользователе
    void findUserInProgress();            //!< Сигнал о начале поиска пользователей

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки информации о текущем пользователе.
     * \param res результат выполнения запроса
     * \param username имя пользователя
     * \param userId идентификатор пользователя
     * \param avatarUrl ссылка на аватар пользователя
     */
    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX, const QString &avatarUrl = "");

    /**
     * Сигнал об окончании загрузки информации о запрашиваемом пользователе.
     * \param res результат выполнения запроса
     * \param user объект с данными найденного пользователя
     */
    void getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user = {});

    /**
     * Сигнал об окончании загрузки нового аватара.
     * \param res результат выполнения запроса
     * \param avatarUrl новая ссылка на загруженный аватар
     */
    void uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl = "");

    /**
     * Сигнал об окончании поиска пользователей.
     * \param res результат выполнения запроса
     * \param paObjects список найденных пользователей
     * \param input строка поиска
     */
    void findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {}, const QString &input = "");

private slots:
    // --- Слоты для обработки сигналов от UserInfoService ---

    void on_GetMyUserInfoFinished(const NetworkResult &res, RetryableRequest reReq, const QString &username, unsigned long long userId, const QString &avatarUrl);
    void on_GetUserInfoFinished(const NetworkResult &res, RetryableRequest reReq, const ParsedFoundUsersObject &user);
    void on_UploadAvatarFinished(const NetworkResult &res, RetryableRequest reReq, const QString &avatarUrl);
    void on_FindUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects, const QString &input);

private:
    // --- Внутренние сервисы ---
    UserInfoService *userInfoService;     //!< Сервис для работы с API информации о пользователях
};

#endif // USERINFOCONTROLLER_H
