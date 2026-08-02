#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QMainWindow>
#include <QStackedWidget>
#include <QUuid>
#include <QSoundEffect>
#include <QVariantAnimation>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QGraphicsOpacityEffect>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include "models/chatlistmodel.h"
#include "models/chatmessageslistmodel.h"
#include "models/searchlistmodel.h"
#include "delegates/chatmessagesitemdelegate.h"
#include "controllers/authcontroller.h"
#include "controllers/userinfocontroller.h"
#include "controllers/chatscontroller.h"
#include "controllers/websocketcontroller.h"
#include "controllers/filescontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    /**
     * Обработка события отображения окна.
     * Сохраняет первоначальную ширину поля ввода.
     * \param event объект QShowEvent
     */
    void showEvent(QShowEvent *event) override;

    /**
     * Обработка события изменения размера окна.
     * Вызывает реализацию базового класса QMainWindow::resizeEvent.
     * \param event объект QResizeEvent
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * Фильтрация событий для различных элементов UI (например, обработка фокуса поля поиска,
     * горячих клавиш в поле ввода сообщения, кликов по аватарам и иконке приложения).
     * \param obj объект к которому применяется event
     * \param event объект QEvent
     * \return true, если событие было обработано, иначе false
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * Обработка нативных событий Windows для изменения размера окна
     * \param eventType тип события
     * \param message сообщение события
     * \param result результат обработки события
     */
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    /**
     * Обработка события закрытия окна.
     * Скрывает приложение в системный трей, если он доступен.
     * \param event объект QCloseEvent
     */
    void closeEvent(QCloseEvent *event) override;

private:
    /**
     * Закрывает текущий открытый чат, очищает messagesListModel и chatMessages
     * и сбрасывает currentChatId и currentChatName
     */
    void closeCurrentChat();

    /**
     * Показывает editStatusLabel с анимацией и скрывает его через 3 секунды
     */
    void hideEditStatusLabelSmoothly();

    /**
     * Настраивает панель аватара собеседника,
     * создавая необходимые виджеты и анимации
     */
    void setupInterlocutorAvatarPanel();
    
    /**
     * Показывает панель аватара собеседника с анимацией
     */
    void showInterlocutorAvatarPanel();
    
    /**
     * Скрывает панель аватара собеседника с анимацией
     */
    void hideInterlocutorAvatarPanel();

    /**
     * Переключает состояние левой панели между свернутым и развернутым
     */
    void toggleLeftPanel();

    /// Флаг, указывающий, развернута ли левая панель или свернута
    bool isLeftPanelExpanded = false;

private slots:
    /**
     * Вызывается при изменении текста в поле поиска пользователей
     * \param arg1 новый текст в поле поиска
     */
    void on_searchInput_textChanged(const QString &arg1);
    /**
     * Обрабатывает клик по элементу списка чатов или поиска.
     * Открывает выбранный чат или создает новый диалог, если выбран пользователь из результатов поиска.
     * \param chatItem объект QModelIndex представляющий нажатый элемент
     */
    void on_chatsView_clicked(const QModelIndex &chatItem);

    /**
     * Обрабатывает нажатие на кнопку отправки сообщения.
     * Отправляет сообщение через WebSocket, сохраняет его в локальном хранилище (либо редактирует существующее)
     * и очищает поле ввода.
     */
    void on_sendMessageBtn_clicked();

    /**
     * Обрабатывает нажатие на кнопку возврата в списки чатов.
     * Закрывает текущий открытый чат и возвращает пользователя к списку чатов.
     */
    void on_backBtn_clicked();

    /**
     * Обрабатывает нажатие на кнопку прикрепления файла.
     * Открывает диалог выбора файлов и добавляет их в список ожидания для отправки.
     */
    void on_attachFileBtn_clicked();

    /**
     * Действие при нажатии на кнопку "Регистрация"
     * вызывает метод регистрации в AuthController
     * 
     */
    void on_registrationBtn_clicked();

    /**
     * Действие при нажатии на кнопку "Войти"
     * вызывает метод авторизации в AuthController
     */
    void on_logInBtn_clicked();

    /**
     * Действие при нажатии на кнопку "Показать пароль" при регистрации
     * переводит поле ввода пароля в режим отображения текста
     */
    void on_revealRegistrationPasswordBtn_pressed();

    /**
     * Действие при отпускании кнопки "Показать пароль" при регистрации
     * возвращает поле ввода пароля в режим скрытия текста
     */
    void on_revealRegistrationPasswordBtn_released();

    /**
     * Действие при нажатии на кнопку "Показать пароль" в поле подтверждения пароля
     * переводит поле ввода пароля в режим отображения текста
     */
    void on_revealRegistrationPasswordConfirmBtn_pressed();

    /**
     * Действие при отпускании кнопки "Показать пароль" в поле подтверждения пароля
     * возвращает поле ввода пароля в режим скрытия
     */
    void on_revealRegistrationPasswordConfirmBtn_released();

    /**
     * Действие при нажатии на кнопку "Показать пароль" в поле ввода пароля для входа
     * переводит поле ввода пароля в режим отображения текста
     */
    void on_revealLogInPasswordBtn_pressed();

    /**
     * Действие при отпускании кнопки "Показать пароль" в поле ввода пароля для входа
     * возвращает поле ввода пароля в режим скрытия
     */
    void on_revealLogInPasswordBtn_released();

    /**
     * Действие при нажатии на кнопку "Перейти к входу"
     * переключает между окнами регистрации и входа
     */
    void on_switchToLogInBtn_clicked();

    /**
     * Действие при нажатии на кнопку "Перейти к регистрации"
     * переключает между окнами регистрации и входа
     */
    void on_switchToRegistrationBtn_clicked();

    /**
     * Действие при нажатии на кнопку "Войти" в поле ввода пароля нажатием клавиши Enter
     * вызывает метод авторизации в AuthController
     */
    void on_logInPassword_returnPressed();

    /**
      * Вызывается при получении сигнала о завершении регистрации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      * \param res объект NetworkResult содержащий результат регистрации
      * \param accToken строка с access токеном
      * \param refToken строка с refresh токеном
      */
    void on_registrationFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении авторизации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      * \param res объект NetworkResult содержащий результат авторизации
      * \param accToken строка с access токеном
      * \param refToken строка с refresh токеном
      */
    void on_logInFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении выхода из аккаунта
      * При успехе перебрасывает на окно регистрации попутно подчищая за собой все пользовательские данные(чаты, названия чатов и т.д)
      * При неудаче выводит сообщение об ошибке
      * \param res объект NetworkResult содержащий результат выхода из аккаунта
      */
    void on_logOutFinished(const NetworkResult &res);

    /**
      * Вызывается при получении сигнала о том что начался процесс регистрации
      * Замораживает кнопки на веремя регистрации
      */
    void on_registrationInProgress();

    /**
     * Вызывается при получении сигнала о том что начался процесс авторизации
     * Замораживает кнопки на время авторизации
     */
    void on_logInProgress();

    /**
     * Вызывается при получении сигнала о том что начался процесс выхода из аккаунта
     * Замораживает кнопки на время выхода
     */
    void on_logOutInProgress();

    /**
     * Вызывается при получении сигнала о том что начался процесс обновления access токена
     * Замораживает кнопки на время обновления токена
     */
    void on_refreshAccessTokenInProgress();

    /**
     * Вызывается при получении сигнала о том что завершился процесс обновления access токена
     * При успехе обновляет access токен и refresh токен в MainWindow
     * \param res объект NetworkResult содержащий результат обновления токена
     * \param accToken строка с access токеном
     * \param refToken строка с refresh токеном
     */
    void on_refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
     * Вызывается при получении сигнала о том что начался процесс получения информации о пользователе
     * Замораживает кнопки на время получения информации
     */
    void on_getMyUserInfoInProgress();

    /**
     * Вызывается при получении сигнала о том что завершился процесс получения информации о пользователе
     * При успехе обновляет информацию о пользователе в MainWindow
     * \param res объект NetworkResult содержащий результат получения информации о пользователе
     * \param username имя пользователя
     * \param userId id пользователя
     * \param avatarUrl ссылка на аватар пользователя
     */
    void on_getMyUserInfoFinished(const NetworkResult &res, const QString &username, unsigned long long userId, const QString &avatarUrl);
    
    /**
     * Вызывается при получении сигнала о том что начался процесс получения информации о другом пользователе
     * Замораживает кнопки на время получения информации
     * \param user объект ParsedFoundUsersObject содержащий информацию о пользователе
     * \param res объект NetworkResult содержащий результат получения информации
     */
    void on_getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user);
    
    /**
     * Вызывается при получении сигнала о том что завершился процесс загрузки аватара
     * При успехе обновляет аватар в MainWindow
     * \param res объект NetworkResult содержащий результат загрузки аватара
     * \param avatarUrl ссылка на загруженный аватар
     */
    void on_uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl);
    
    /**
     * Вызывается при получении сигнала о том что начался процесс загрузки аватара
     * Замораживает кнопки на время загрузки аватара
     * \param pos позиция курсора мыши
     */
    void showAvatarContextMenu(const QPoint &pos);

    /**
     * Показывает контекстное меню при клике на имя пользователя (например, для выхода из аккаунта)
     * \param pos позиция курсора мыши
     */
    void showUserNameContextMenu(const QPoint &pos);

    /**
     * Вызывается при начале загрузки списка чатов
     */
    void on_getMyChatsInProgress();

    /**
     * Вызывается при завершении загрузки списка чатов
     * \param res результат выполнения запроса
     * \param paObjects список полученных чатов
     */
    void on_getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects );

    /**
     * Вызывается при начале загрузки истории сообщений чата
     */
    void on_getChatMessagesInProgress();

    /**
     * Вызывается при завершении загрузки истории сообщений
     * \param res результат выполнения запроса
     * \param chatId идентификатор чата
     * \param paObjects список полученных сообщений
     */
    void on_getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId, const std::vector<ParsedChatMessagesArrayObject>& paObjects);

    /**
     * Вызывается при завершении создания нового личного чата
     * \param res результат выполнения запроса
     */
    void on_createDirectChatFinished(const NetworkResult &res);

    /**
     * Вызывается при начале создания нового личного чата
     */
    void on_createDirectChatInProgress();

    /**
     * Вызывается при начале установки соединения WebSocket
     */
    void on_socketConnectionInProgress();

    /**
     * Вызывается при успешной/неудачной попытке установить WebSocket соединение
     * \param res результат выполнения запроса
     */
    void on_socketConnectionFinished(const NetworkResult &res);

    /**
     * Вызывается при начале отключения WebSocket
     */
    void on_socketDisonnectionInProgress();

    /**
     * Вызывается при завершении отключения WebSocket
     * \param res результат выполнения запроса
     */
    void on_socketDisonnectionFinished(const NetworkResult &res);

    /**
     * Инициирует режим редактирования сообщения
     * \param messageId идентификатор сообщения
     * \param currentText текущий текст сообщения
     */
    void onEditMessageRequested(quint64 messageId, const QString &currentText);

    /**
     * Запрашивает подтверждение и инициирует удаление сообщения
     * \param messageId идентификатор сообщения
     */
    void onDeleteMessageRequested(quint64 messageId);

    /**
     * Вызывается после ответа сервера на запрос редактирования сообщения
     * \param res результат выполнения запроса
     */
    void on_editMessageFinished(const NetworkResult &res);

    /**
     * Вызывается после ответа сервера на запрос удаления сообщения
     * \param res результат выполнения запроса
     */
    void on_deleteMessageFinished(const NetworkResult &res);

    /**
     * Вызывается при начале отправки нового сообщения на сервер
     */
    void on_sendingMessageInProgress();

    /**
     * Вызывается после ответа сервера на отправку нового сообщения
     * \param res результат выполнения запроса
     */
    void on_sendingMessageFinished(const NetworkResult &res);

    /**
     * Обрабатывает получение нового сообщения по WebSocket
     * \param newMessage объект нового сообщения
     */
    void on_newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage);

    /**
     * Обрабатывает подтверждение доставки (принятия) сервером нашего сообщения
     * \param msgAccObj информация о принятом сообщении
     */
    void on_messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);

    /**
     * Вызывается при изменении текста в поле ввода сообщения (для автоподгонки высоты)
     */
    void on_textChanged();

    /**
     * Вызывается при начале поиска пользователя
     */
    void on_findUserInProgress();

    /**
     * Вызывается при завершении поиска пользователя, обновляет модель поиска
     * \param res результат выполнения запроса
     * \param paObjects список найденных пользователей
     */
    void on_findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {});

    /**
     * Вызывается, когда в окно приложения перетаскивают файлы (Drag'n'Drop), инициирует их загрузку
     */
    void on_gotDragNDropFiles();

    /**
     * Вызывается в процессе загрузки файла (вложения) на сервер
     */
    void on_uploadFileInProgress();

    /**
     * Вызывается при завершении загрузки файла на сервер
     * \param res результат выполнения запроса
     * \param filePath локальный путь загруженного файла
     * \param chatId идентификатор чата
     * \param fileInfo информация о загруженном файле
     */
    void on_uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo);

    /**
     * Обрабатывает клик по сообщению в списке (например, для скачивания вложений)
     * \param index индекс нажатого элемента
     */
    void on_messagesView_clicked(const QModelIndex &index);

    /**
     * Инициирует отправку запроса о прочтении сообщения собеседника
     * \param message пара <id чата, id сообщения>
     */
    void on_needReadLastMessage(const std::pair<quint64, quint64> &message);

    /**
     * Обновляет локальный статус сообщений на "прочитано"
     * \param userId ID пользователя, чьи сообщения прочитаны
     * \param chatId ID чата
     * \param lastReadMessageId ID последнего прочитанного сообщения
     */
    void on_messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId);

    /**
     * Вызывается в процессе загрузки метаданных файла для скачивания
     */
    void on_downloadFileInfoInProgress();

    /**
     * Вызывается после загрузки метаданных файла
     * \param res результат выполнения запроса
     * \param fileInfo полученные метаданные файла
     */
    void on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

    /**
     * Вызывается в процессе скачивания файла
     */
    void on_downloadFileInProgress();

    /**
     * Вызывается при завершении скачивания файла
     * \param res результат выполнения запроса
     * \param fileInfo информация о скачанном файле
     */
    void on_downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

#ifndef QT_DEBUG
    /**
     * Обрабатывает действия с иконкой приложения в трее (восстановление окна)
     * \param reason причина активации
     */
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
#endif

private:
    // UI элементы
    Ui::MainWindow *ui;                                                  //!< Указатель на сгенерированный UI класс
    QLabel *editStatusLabel = nullptr;                                   //!< Метка, отображающая статус "Редактирование" над полем ввода
#ifndef QT_DEBUG
    QSystemTrayIcon *trayIcon;                                           //!< Иконка приложения в системном трее
    QMenu *trayIconMenu;                                                 //!< Контекстное меню иконки в системном трее
    QAction *quitAction;                                                 //!< Действие "Выход" для меню системного трея
#endif
    QSoundEffect *notificationSound;                                     //!< Звуковой эффект для входящих сообщений
    QPointer<QParallelAnimationGroup> pageSwitchAnimation;               //!< Анимация переключения страниц
    QPointer<QVariantAnimation> logInBtnAnimation;                       //!< Анимация кнопки логина
    bool isLogInEnterPressed = false;                                    //!< Флаг, указывающий на нажатие Enter в поле логина
    bool m_initialInputWidthSet = false;                                 //!< Флаг установки первоначальной ширины поля ввода
    int m_baseWindowWidth = -1;                                          //!< Базовая ширина окна при запуске
    int m_currentSpacerWidth = 170;                                      //!< Текущая целевая ширина спейсеров (отступов)

    QFrame *interlocutorAvatarPanel = nullptr;
    QLabel *interlocutorAvatarLarge = nullptr;
    QPropertyAnimation *interlocutorAvatarOpacityAnim = nullptr;
    QVariantAnimation *interlocutorAvatarOutlineAnim = nullptr;
    int interlocutorAvatarOutlineAlpha = 0;
    void updateInterlocutorAvatarOutline();
    QGraphicsOpacityEffect *interlocutorAvatarOpacityEffect = nullptr;
    QPixmap currentInterlocutorAvatarFull;
    QFrame *windowBorderFrame = nullptr;
    
    QWidget *stagingWidget = nullptr;
    QWidget *stagingContentWidget = nullptr;
    QVBoxLayout *stagingFilesLayout = nullptr;
    QGridLayout *stagingMediaLayout = nullptr;
    QVariantAnimation *messageInputHorizontalAnim = nullptr;
    QVariantAnimation *messageInputHeightAnim = nullptr;

    // Переменные для отображения облака при наведении на чат в списке
    QWidget *chatTooltipWidget = nullptr;
    QLabel *tooltipTitleLabel = nullptr;
    QLabel *tooltipSubtitleLabel = nullptr;
    QLabel *tooltipTimeLabel = nullptr;
    QLabel *tooltipBadgeLabel = nullptr;
    QPropertyAnimation *tooltipOpacityAnim = nullptr;
    QGraphicsOpacityEffect *tooltipOpacityEffect = nullptr;
    QModelIndex lastHoveredChatIndex;
    QTimer *tooltipHideTimer = nullptr;

    // Моедли и делегаты
    ChatListModel *chatsListModel;                                       //!< Модель списка чатов с доступом к полям через роли
    SearchListModel *searchListModel;                                    //!< Модель списка пользователей при поиске с доступом к полям через роли
    ChatMessagesListModel *messagesListModel;                            //!< Модель списка сообщений для выбранного чата
    ChatMessagesItemDelegate *messagesItemDelegate;                      //!< Делегат сообщений (выравнивание своих/чужих, отображение вложений)

    // Хранилища данных
    QHash<unsigned long long, std::vector<ParsedChatMessagesArrayObject>> chatMessages; //!< Хранилище сообщений по chatId
    QHash<unsigned long long, ParsedChatMessagesArrayObject> draftsByChatId;            //!< Черновики сообщений по chatId
    QHash<unsigned long long, ParsedChatsListArrayObject> chatsList;     //!< Хранилище информации о чатах по chatId
    QString currentChatName;                                             //!< Название текущего открытого чата
    unsigned long long currentChatId;                                    //!< Идентификатор текущего открытого чата
    quint64 editingMessageId = ULONG_LONG_MAX;                           //!< Идентификатор редактируемого в данный момент сообщения

    // Поля авторизации и информации о пользователе
    bool isAuthorized;                                                   //!< Флаг авторизации пользователя
    bool isFirstOpen;                                                    //!< Флаг первого открытия приложения для попытки авто-авторизации
    QString currentUsername;                                             //!< Имя текущего пользователя
    unsigned long long myUserId;                                         //!< Идентификатор текущего пользователя
    QString currentAvatarUrl;                                            //!< Ссылка на текущий аватар пользователя
    QString accessToken;                                                 //!< Токен доступа (Access Token) для API
    QString refreshToken;                                                //!< Токен обновления (Refresh Token) для API

    // Контроллеры для взаимодействия с сервисами
    AuthController *authController;                                      //!< Контроллер авторизации (запросы от UI -> AuthService -> результаты через сигналы)
    UserInfoController *userInfoController;                              //!< Контроллер профиля пользователя (запросы от UI -> UserInfoService -> результаты через сигналы)
    ChatsController *chatsController;                                    //!< Контроллер чатов (запросы от UI -> ChatService -> результаты через сигналы)
    WebsocketController *websocketController;                            //!< Контроллер WebSocket (запросы от UI -> WebsocketService -> результаты через сигналы)
    FilesController *filesController;                                    //!< Контроллер файлов (загрузка и скачивание вложений)

    /**
     * Попытка автоматической авторизации при старте с использованием сохраненного токена
     */
    void tryAuthorize();

    /**
     * Запрос получения информации о текущем пользователе
     */
    void getMyInfo();

    /**
     * Обработка результатов авторизации и маршрутизация на нужный экран
     * \param res результат сетевого запроса
     * \param accToken полученный access token
     * \param refToken полученный refresh token
     */
    void checkAuthorization(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
     * Запрос на получение списка чатов текущего пользователя
     */
    void getChatsList();

    /**
     * Запрос на получение истории сообщений определенного чата
     * \param chatId идентификатор чата
     */
    void getChatMessages(const unsigned long long &chatId);

    /**
     * Запрос на создание личного чата с пользователем
     * \param userId идентификатор собеседника
     */
    void createDirectChat(const unsigned long long &userId);

    /**
     * Переключение страниц в QStackedWidget с использованием анимации свайпа
     */
    void switchPageWithSlideAnimation(QStackedWidget *stackedWidget, QWidget *newPage);

    /**
     * Переключение страниц в QStackedWidget с использованием анимации затухания (fade)
     */
    void switchPageWithFadeAnimation(QStackedWidget *stackedWidget, QWidget *newPage);

    /**
     * Анимация перехода при старте приложения
     */
    void animateStartupTransition();

    /**
     * Сохраняет набранный текст и вложения в черновик для выбранного чата
     * \param chatId идентификатор чата
     */
    void saveDraftForChat(unsigned long long chatId);

    /**
     * Восстанавливает текст и вложения из черновика для выбранного чата
     * \param chatId идентификатор чата
     */
    void loadDraftForChat(unsigned long long chatId);

    /**
     * Добавляет загруженное вложение к текущему черновику
     * \param chatId идентификатор чата
     * \param fileInfo загруженная информация о файле
     * \param localPath локальный путь для отображения
     */
    void appendAttachmentToDraft(unsigned long long chatId, const ParsedUploadedFileInfo &fileInfo, const QString &localPath = QString());

    /**
     * Обновляет состояние кнопки отправки сообщения (вкл/выкл) в зависимости от того, есть ли незагруженные файлы
     * \param chatId идентификатор чата
     */
    void updateSendButtonState(unsigned long long chatId);

    /**
     * Убирает маркеры вложений (например, [attachments: 123]) из текста перед отправкой
     * \param text исходный текст сообщения
     * \return очищенный текст
     */
    QString stripAttachmentMarker(const QString &text) const;
    
    /**
     * Обновляет интерфейс Staging Clouds для выбранного чата
     */
    void updateStagingCloudsUI(unsigned long long chatId);

    /**
     * Автоматически загружает изображения для переданного списка сообщений, если это необходимо
     * \param messages список сообщений
     */
    void autoDownloadImages(const std::vector<ParsedChatMessagesArrayObject>& messages);

    /**
     * Автоматически загружает изображения для одиночного сообщения, если это необходимо
     * \param message объект сообщения
     */
    void autoDownloadImages(const ParsedChatMessagesArrayObject& message);

    void refreshChatState(QHash<unsigned long long, ParsedChatsListArrayObject>::iterator &chatIt,
      const ParsedChatMessagesArrayObject &newMessage, bool isNeedRotation, bool isNeedIncrementUnread);

    void setUnreadCount(quint64 chatId, int count);
    
    void decreaseUnreadCount(quint64 chatId, int count);
};
#endif // MAINWINDOW_H
