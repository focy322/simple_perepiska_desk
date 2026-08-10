#ifndef USERINFOCONTROLLER_H
#define USERINFOCONTROLLER_H

#include <QObject>
#include "services/userinfoservice.h"

/**
 * Контроллер для работы с информацией о пользователях.
 * Принимает запросы от UI, вызывает методы UserInfoService и возвращает результат через сигналы.
 */
class UserInfoController : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoController(QObject *parent = nullptr);

    /**
     * Запрашивает информацию о текущем авторизованном пользователе.
     * \param accToken токен доступа (Access Token)
     */
    void requestMyUserInfo(const QString &accToken);

    /**
     * Запрашивает информацию о другом пользователе по его ID.
     * \param accToken токен доступа (Access Token)
     * \param userId идентификатор пользователя
     */
    void requestUserInfo(const QString &accToken, unsigned long long userId) const;

    /**
     * Отправляет запрос на обновление аватара текущего пользователя.
     * \param accToken токен доступа (Access Token)
     * \param imageData бинарные данные изображения аватара
     */
    void requestUploadAvatar(const QString &accToken, const QByteArray &imageData);

    /**
     * Запрашивает поиск пользователей по введенной строке.
     * \param accessToken токен доступа (Access Token)
     * \param input строка для поиска (имя или логин)
     */
    void requestFindUser(const QString &accessToken, const QString &input) const;

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

private:
    // --- Внутренние сервисы ---
    UserInfoService *userInfoService;     //!< Сервис для работы с API информации о пользователях
};

#endif // USERINFOCONTROLLER_H
