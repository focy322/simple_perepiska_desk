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
      * При событии resize у MainWindow вызывается resizeEvent базово класса
      * для изменения позиции конопки "Выход"
      * @param event объект QResizeEvent
      */
    void resizeEvent(QResizeEvent *event) override;


    /**
      * Отлавливание события фокуса на поле поиска для изменения с chatsView на searchView
      * @param obj объект к которому применяется event
      * @param event объект QEvent
      */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * Обработка нативных событий Windows для изменения размера окна
     */
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void closeCurrentChat();
    void hideEditStatusLabelSmoothly();
    void toggleLeftPanel();
    bool isLeftPanelExpanded = false;

private slots:
    void on_searchInput_textChanged(const QString &arg1);
    /**
      * При выборе чата из списка чатов в messagesView (chatName) выводит его название
      *
      * @param chatItem объект представляющий нажатый элемент из chatsList
      */
    void on_chatsView_clicked(const QModelIndex &chatItem);

    /**
      * При нажатии на кнопку отправки сообщения добавляет строку из messageInput в хранилище сообщений
      * и обновляет messagesListModel обновленным хранилищем сообщений
      */
    void on_sendMessageBtn_clicked();
    void on_attachFileBtn_clicked();

    // TODO: нормальное описание
    /**
      * При нажатии на кнопку "Зарегистрироваться" происходит регистрация
      *
      */
    void on_registrationBtn_clicked();

    // TODO: нормальное описание
    /**
      * При нажатии на кнопку "Войти" происходит вход
      *
      */
    void on_logInBtn_clicked();

    void on_revealRegistrationPasswordBtn_pressed();

    void on_revealRegistrationPasswordBtn_released();

    void on_revealRegistrationPasswordConfirmBtn_pressed();

    void on_revealRegistrationPasswordConfirmBtn_released();

    void on_revealLogInPasswordBtn_pressed();

    void on_revealLogInPasswordBtn_released();

    void on_switchToLogInBtn_clicked();

    void on_switchToRegistrationBtn_clicked();

    /**
      * Вызывается при получении сигнала о завершении регистрации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      *
      */
    void on_registrationFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении авторизации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      *
      */
    void on_logInFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении выхода из аккаунта
      * При успехе перебрасывает на окно регистрации попутно подчищая за собой все пользовательские данные(чаты, названия чатов и т.д)
      * При неудаче пока хз что
      *
      */
    void on_logOutFinished(const NetworkResult &res);

    /**
      * Вызывается при получении сигнала о том что начался процесс регистрации
      * Замораживает кнопки на веремя регистрации
      *
      */
    void on_registrationInProgress();

    /**
      * Вызывается при получении сигнала о том что начался процесс авторизации
      * Замораживает кнопки на время регистрации
      *
      */
    void on_logInProgress();

    /**
      * Вызывается при получении сигнала о том что начался процесс выходп из аккаунта
      * Что то происходит
      *
      */
    void on_logOutInProgress();



    void on_refreshAccessTokenInProgress();

    void on_refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken, const QString &refToken);

    void on_getMyUserInfoInProgress();

    void on_getMyUserInfoFinished(const NetworkResult &res, const QString &username, unsigned long long userId, const QString &avatarUrl);
    
    void on_getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user);
    
    void on_uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl);
    void showAvatarContextMenu(const QPoint &pos);
    void showUserNameContextMenu(const QPoint &pos);

    void on_getMyChatsInProgress();

    void on_getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects );

    void on_getChatMessagesInProgress();

    void on_getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId, const std::vector<ParsedChatMessagesArrayObject>& paObjects);

    void on_createDirectChatFinished(const NetworkResult &res);

    void on_createDirectChatInProgress();

    void on_socketConnectionInProgress();

    void on_socketConnectionFinished(const NetworkResult &res);

    void on_socketDisonnectionInProgress();

    void on_socketDisonnectionFinished(const NetworkResult &res);

    void onEditMessageRequested(quint64 messageId, const QString &currentText);
    void onDeleteMessageRequested(quint64 messageId);
    void on_editMessageFinished(const NetworkResult &res);
    void on_deleteMessageFinished(const NetworkResult &res);

    void on_sendingMessageInProgress();

    void on_sendingMessageFinished(const NetworkResult &res);

    void on_newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage);

    void on_messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);

    void on_textChanged();

    void on_findUserInProgress();

    void on_findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {});



    void on_gotDragNDropFiles();

    void on_uploadFileInProgress();

    void on_uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo);

    void on_messagesView_clicked(const QModelIndex &index);

    void on_needReadLastMessage(const std::pair<quint64, quint64> &message);

    void on_messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId);

    void on_downloadFileInfoInProgress();

    void on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

    void on_downloadFileInProgress();

    void on_downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

private:
    Ui::MainWindow *ui;
    QLabel *editStatusLabel = nullptr;
    ChatListModel *chatsListModel;                      //!< Модель чатов с доступом к полям через роли
    SearchListModel *searchListModel;                   //!< Модель списка пользователей при поиске с доступом к полям через роли
    ChatMessagesItemDelegate *messagesItemDelegate;     //!< Делегат сообщений (выравнивание своих/чужих)
    ChatMessagesListModel *messagesListModel;           //!< Модель сообщений для выбранного чата
    QHash<unsigned long long, std::vector<ParsedChatMessagesArrayObject>> chatMessages;  //!< Хранилище сообщений по chatId
    QHash<unsigned long long, ParsedChatMessagesArrayObject> draftsByChatId;  //!< Черновики сообщений по chatId
    QString currentChatName;                            //!< Название текущего открытого чата
    unsigned long long currentChatId;                   //!< Id текущего открытого чата

    quint64 editingMessageId = ULONG_LONG_MAX;          //!< Id редактируемого в данный момент сообщения


    AuthController *authController;                     //!< Принимает запросы от UI, дергает AuthService, возвращает результат через сигналы.
    bool isAuthorized;                                  //!< Флаг авторизации пользователя
    QString currentUsername;                            //!< Имя пользователя
    unsigned long long myUserId;                          //!< Id пользователя
    QString currentAvatarUrl;                           //!< Ссылка на текущий аватар пользователя
    UserInfoController *userInfoController;             //!< Принимает запросы от UI, дергает UserInfoService, возвращает результат через сигналы.
    QString accessToken;
    QString refreshToken;
    ChatsController *chatsController;                   //!< Принимает запросы от UI, дергает ChatService, возвращает результат через сигналы.
    bool isFirstOpen;                                   //!< Флаг первого открытия приложения для авторизирования пользователя в приложение
    QHash<unsigned long long, ParsedChatsListArrayObject> chatsList;  //!< Список чатов состоящий из ParsedArrayObject
    WebsocketController *websocketController;           //!< Принимает запросы от UI, дергает WebsocketService, возвращает результат через сигналы.
    QSoundEffect *notificationSound;
    FilesController *filesController;
    QPointer<QParallelAnimationGroup> pageSwitchAnimation;

    void tryAuthorize();

    void getMyInfo();

    void checkAuthorization(const NetworkResult &res, const QString &accToken, const QString &refToken);

    void getChatsList();

    void getChatMessages(const unsigned long long &chatId);

    void createDirectChat(const unsigned long long &userId);

    void switchPageWithSlideAnimation(QStackedWidget *stackedWidget, QWidget *newPage);
    void switchPageWithFadeAnimation(QStackedWidget *stackedWidget, QWidget *newPage);
    void animateStartupTransition();

    void saveDraftForChat(unsigned long long chatId);
    void loadDraftForChat(unsigned long long chatId);
    void appendAttachmentToDraft(unsigned long long chatId, const ParsedUploadedFileInfo &fileInfo, const QString &localPath = QString());
    void updateSendButtonState(unsigned long long chatId);
    QString stripAttachmentMarker(const QString &text) const;

    void autoDownloadImages(const std::vector<ParsedChatMessagesArrayObject>& messages);
    void autoDownloadImages(const ParsedChatMessagesArrayObject& message);

};
#endif // MAINWINDOW_H
