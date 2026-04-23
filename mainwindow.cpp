#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatlistitemdelegate.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QThread>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatsListModel(new ChatListModel(this))
    , messagesItemDelegate(new ChatMessagesItemDelegate(this))
    , messagesListModel(new ChatMessagesListModel(this))
    , chatMessages()
    , currentChatName()
    , currentChatId(ULONG_LONG_MAX)
    , logOutBtn(new QPushButton("Выход", nullptr))
    , authController(new AuthController(this))
    , isAuthorized(false)
    , currentUsername("")
    , userId(ULONG_LONG_MAX)
    , userInfoController(new UserInfoController(this))
    , accessToken("")
    , refreshToken("")
    , chatsController(new ChatsController(this))
    , isFirstOpen(true)
    , chatsList{}
    , websocketController(new WebsocketController(this))
{
    connect(authController, &AuthController::registrationFinished, this, &MainWindow::on_registrationFinished);
    connect(authController, &AuthController::logInFinished, this, &MainWindow::on_logInFinished);
    connect(authController, &AuthController::logOutFinished, this, &MainWindow::on_logOutFinished);
    connect(authController, &AuthController::registrationInProgress, this, &MainWindow::on_registrationInProgress);
    connect(authController, &AuthController::logInProgress, this, &MainWindow::on_logInProgress);
    connect(authController, &AuthController::logOutInProgress, this, &MainWindow::on_logOutInProgress);
    connect(authController, &AuthController::refreshAccessTokenInProgress, this, &MainWindow::on_refreshAccessTokenInProgress);
    connect(authController, &AuthController::refreshAccessTokenFinished, this, &MainWindow::on_refreshAccessTokenFinished);
    connect(userInfoController, &UserInfoController::getMyUserInfoInProgress, this, &MainWindow::on_getMyUserInfoInProgress);
    connect(userInfoController, &UserInfoController::getMyUserInfoFinished, this, &MainWindow::on_getMyUserInfoFinished);
    connect(chatsController, &ChatsController::getMyChatsInProgress, this, &MainWindow::on_getMyChatsInProgress);
    connect(chatsController, &ChatsController::getMyChatsFinished, this, &MainWindow::on_getMyChatsFinished);
    connect(chatsController, &ChatsController::getChatMessagesInProgress, this, &MainWindow::on_getChatMessagesInProgress);
    connect(chatsController, &ChatsController::getChatMessagesFinished, this, &MainWindow::on_getChatMessagesFinished);
    connect(chatsController, &ChatsController::createDirectChatInProgress, this, &MainWindow::on_createDirectChatInProgress);
    connect(chatsController, &ChatsController::createDirectChatFinished, this, &MainWindow::on_createDirectChatFinished);
    connect(websocketController, &WebsocketController::socketConnectionInProgress, this, &MainWindow::on_socketConnectionInProgress);
    connect(websocketController, &WebsocketController::socketConnectionFinished, this, &MainWindow::on_socketConnectionFinished);
    connect(websocketController, &WebsocketController::socketDisonnectionInProgress, this, &MainWindow::on_socketDisonnectionInProgress);
    connect(websocketController, &WebsocketController::socketDisonnectionFinished, this, &MainWindow::on_socketDisonnectionFinished);
    connect(websocketController, &WebsocketController::sendingMessageInProgress, this, &MainWindow::on_sendingMessageInProgress);
    connect(websocketController, &WebsocketController::sendingMessageFinished, this, &MainWindow::on_sendingMessageFinished);
    connect(websocketController, &WebsocketController::newMessageRecieved, this, &MainWindow::on_newMessageRecieved);
    connect(websocketController, &WebsocketController::messageAccepted, this, &MainWindow::on_messageAccepted);


    ui->setupUi(this);
    setUpLogOutBtn();

    //TODO: Эту хуйню вынести куда то в отдельный файл а может и все css стили в по файлам растаскать
    ui->chatsView->setStyleSheet(
        "QListView {"
        " background-color:#FFF8DC;"
        " border: none;"
        " outline: 0; "
        " border-right: 1px solid black;"
        " }"
        "QListView::item {"
        " min-height: 36px;"
        " padding: 8px 12px;"
        " border-radius: 6px;"
        " }"
        "QListView::item:hover {"
        " background-color:#FFE4B5;"
        " }"
        "QListView::item:selected {"
        " background-color:#F4A460;"
        " color:black;"
        " }");

    ui->chatsView->setModel(chatsListModel);
    ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
    ui->messagesView->setModel(messagesListModel);
    ui->messagesView->setItemDelegate(messagesItemDelegate);
    ui->loadingAndContentWidgets->setCurrentWidget(ui->loadingPage);

    tryAuthorize();


}

MainWindow::~MainWindow()
{
    //TODO: записывать refreshToken в файл при выходе
    delete ui;
}



void MainWindow::on_chatsView_clicked(const QModelIndex &chatItem)
{
    if (chatItem.isValid())
    {
        currentChatId = chatItem.data(ChatListModel::ChatIdRole).toULongLong();
        currentChatName = chatItem.data(ChatListModel::ChatNameRole).toString().trimmed();
        ui->chatName->setText(currentChatName);
        auto chatIt = chatMessages.constFind(currentChatId); // Итератор на список сообщений (vector<ParsedChatMessagesArrayObject>) для чата с выбранным chatId
        if (chatIt != chatMessages.constEnd())
        {
            messagesListModel->setMessages(chatIt.value());
        } else
        {
            messagesListModel->clear();
            getChatMessages(currentChatId);
        }


#ifdef QT_DEBUG
        qDebug() << chatItem.data(ChatListModel::UserIdRole);
        qDebug() << typeid(chatItem).name();
#endif
    }
    else
    {
        // По сути он невалидным быть не может поэтому хз что тут добавить
    }
}


void MainWindow::on_sendMessageBtn_clicked()
{
    QString msgToSend = ui->messageInput->text();
    bool condToSendMsg = !msgToSend.trimmed().isEmpty() && currentChatId != ULONG_LONG_MAX; // Условия для отправки сообщения
    if (condToSendMsg)
    {
        ParsedChatMessagesArrayObject localMessage;
        localMessage.chatId = currentChatId;
        localMessage.senderId = userId;
        localMessage.message = msgToSend;
        localMessage.timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QString Uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        localMessage.clientMessageId = Uuid;

        chatMessages[currentChatId].push_back(localMessage);
        messagesListModel->appendMessage(localMessage);
        websocketController->requestSendMessage(currentChatId, msgToSend, Uuid);
        ui->messageInput->clear();

    }
    else
    {
        ui->messageInput->clear();
        return;
    }

}


void MainWindow::on_registrationBtn_clicked()
{
    QString login  = ui->registrationLogin->text();
    QString password = ui->registrationPassword->text();
    QString passwordConfirm = ui->registrationPasswordConfirm->text();

    authController->requestRegistration(login, password, passwordConfirm);

}

void MainWindow::on_logInBtn_clicked()
{
    QString login  = ui->logInLogIn->text();
    QString password = ui->logInPassword->text();

    authController->requestLogIn(login, password);

}

void MainWindow::on_revealRegistrationPasswordBtn_pressed()
{
    ui->registrationPassword->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealRegistrationPasswordBtn_released()
{
    ui->registrationPassword->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_revealRegistrationPasswordConfirmBtn_pressed()
{
    ui->registrationPasswordConfirm->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealRegistrationPasswordConfirmBtn_released()
{
    ui->registrationPasswordConfirm->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_revealLogInPasswordBtn_pressed()
{
    ui->logInPassword->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealLogInPasswordBtn_released()
{
    ui->logInPassword->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_switchToLogInBtn_clicked()
{
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
}


void MainWindow::on_switchToRegistrationBtn_clicked()
{
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageRegistration);
}

void MainWindow::setUpLogOutBtn()
{

    logOutBtn->setParent(ui->pageApp);
    logOutBtn->setFixedSize(90, 30); // Размеры кнопки (может в namespace вынести? хз)
    logOutBtn->raise(); // Поднять по Z
    logOutBtn->show();
    positionLogoutButton();
    connect(logOutBtn, &QPushButton::clicked, this, &MainWindow::on_logOutBtn_clicked);
}

void MainWindow::positionLogoutButton()
{
    const int margin = 8;

    // Габариты MainWindow (Почему то через appPage не выходит (походу геометрия до конца не формируется к моменту вызова функции))
    const QRect r = this->rect();

    // В левый нижний угол
    const int x = margin;
    const int y = r.height() - logOutBtn->height() - margin;

    logOutBtn->move(x, y);
}

void MainWindow::tryAuthorize()
{
    QFile file("refreshToken.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // файл не открылся
        isAuthorized = false;
        isFirstOpen = false;
        checkAuthorization(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)}, "", "");
        return;
    }

    refreshToken = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    authController->requestRefreshAccessToken(refreshToken);
}

void MainWindow::getMyInfo()
{
    qDebug() << "Отправленный  accessToken в getMyInfo " << accessToken;
    userInfoController->requestMyUserInfo(accessToken);
}

void MainWindow::checkAuthorization(const NetworkResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        isAuthorized = res.ok;
        accessToken = accToken;
        refreshToken = refToken;
        ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
        getMyInfo();
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        getChatsList();
        websocketController->requestConnectSocket(accessToken);
    }
    else
    {
        isAuthorized = res.ok;
        accessToken = accToken;
        refreshToken = refToken;
        ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);// Показывать окно входа
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);// Показывать окно входа
#ifdef QT_DEBUG
        qDebug() << "authorization Failed!!!";
#endif
    }
}

void MainWindow::getChatsList()
{
    chatsController->requestMyChats(accessToken);
}

void MainWindow::getChatMessages(const unsigned long long &chatId)
{
    chatsController->requestChatMessages(chatId, accessToken);
}

void MainWindow::createDirectChat(const unsigned long long &userId)
{
    chatsController->requestCreateDirectChat(userId, accessToken);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionLogoutButton();
}

void MainWindow::on_logOutBtn_clicked()
{
    authController->requestLogOut(accessToken, refreshToken);
}

void MainWindow::on_refreshAccessTokenInProgress()
{

}

void MainWindow::on_refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken, const QString &refToken)
{
    if (isFirstOpen)
    {
        isFirstOpen = false;
        checkAuthorization(res, accToken, refToken);
    }

    if (res.ok)
    {
        accessToken = accToken;
        refreshToken = refToken;
        qDebug() << "on_RefreshAccessTokenFinished = true!!!";
    }
    else
    {
        isAuthorized = false;
#ifdef QT_DEBUG
        qDebug() << "on_RefreshAccessTokenFinished = false";
#endif
    }
}

void MainWindow::on_getMyUserInfoInProgress()
{

}

void MainWindow::on_getMyUserInfoFinished(const NetworkResult &res, const QString &username, unsigned long long userId)
{
    if (res.ok)
    {
        currentUsername = username;
        this->userId = userId;
        messagesItemDelegate->setCurrentUserId(this->userId);
#ifdef QT_DEBUG
        qDebug() << "currentUsername" << currentUsername;
        qDebug() << "currentUserId" << this->userId;
#endif
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "on_getMyUserInfoFinished = false";
#endif
    }
}

void MainWindow::on_getMyChatsInProgress()
{

}

void MainWindow::on_getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects)
{
    if (res.ok)
    {
        for (const auto &chat : paObjects)
        {
            chatsList.insert(chat.chatId, chat);
        }

        // chatMessages.clear();
        // for (const ParsedArrayObject &chat : chatsList)
        // {
        //     QStringList messages;
        //     if (!chat.lastMessage.trimmed().isEmpty())
        //         messages.append(chat.lastMessage);

        //     chatMessages.insert(chat.chatId, messages);
        // }

        chatsListModel->setChats(paObjects);
#ifdef QT_DEBUG
        qDebug() << "on_getMyChatsFinished = true!!!";
#endif
    }
    else
    {
        chatsList.clear();
        chatMessages.clear();
        chatsListModel->clear();
        currentChatName.clear();
        currentChatId = ULONG_LONG_MAX;
        ui->chatName->setText("Выберите чат");
        messagesListModel->clear();
#ifdef QT_DEBUG
        qDebug() << "on_getMyChatsFinished = false!!!";
#endif
    }
}

void MainWindow::on_getChatMessagesInProgress()
{

}

// TODO: chatId сделать ссылкой и везде где он прокидывается до mainWindow
// (хз может я уже так делал и вылетел segmentation fault поэтому я ссылку убрал, но в принципе 8 байт не так страшно или сколько там sizeof)
void MainWindow::on_getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId, const std::vector<ParsedChatMessagesArrayObject>& paObjects)
{
    if (res.ok)
    {
        // TODO: одинаковые сообещения накладываются друг на друга сверху
        //chatMessages[chatId].insert(chatMessages[chatId].cbegin(), paObjects.cbegin(), paObjects.cend());
        chatMessages[chatId] = paObjects; // TODO: если работает то что выше то это удалить
        if (currentChatId == chatId)
            messagesListModel->setMessages(chatMessages[chatId]);
            //messagesListModel->setMessages(paObjects); // TODO: если работает то что выше то это удалить
        qDebug() << "on_getChatMessagesFinished = true!!!";
    }
    else
    {
        //chatMessages.remove(currentChatId);
        //messagesListModel->clear();
        qDebug() << "on_getChatMessagesFinished = false!!!";
    }
}

void MainWindow::on_createDirectChatFinished(const NetworkResult &res)
{
    if (res.ok)
    {
        qDebug() << "on_createDirectChatFinished = true!!!";
        getChatsList();
    }
    else
    {
        qDebug() << "on_createDirectChatFinished = false!!!";
    }

}

void MainWindow::on_createDirectChatInProgress()
{

}

void MainWindow::on_socketConnectionInProgress()
{

}

void MainWindow::on_socketConnectionFinished(const NetworkResult &res)
{
    if (res.ok)
    {
#ifdef QT_DEBUG
        qDebug() << "on_socketConnectionFinished  = TRUE!!!";
        qDebug() << QDateTime::currentDateTimeUtc();
#endif
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "on_socketConnectionFinished  = FALSE!!!";
        qDebug() << res.message;
#endif
    }

}

void MainWindow::on_socketDisonnectionInProgress()
{

}

void MainWindow::on_socketDisonnectionFinished(const NetworkResult &res)
{

}

void MainWindow::on_sendingMessageInProgress()
{

}

void MainWindow::on_sendingMessageFinished(const NetworkResult &res)
{

}

void MainWindow::on_newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage)
{
    unsigned long long newMsgChatId = newMessage.chatId;
    auto it = chatMessages.find(newMsgChatId);
    if (it != chatMessages.end())
    {
        it.value().push_back(newMessage);
        if (currentChatId == newMsgChatId)
            messagesListModel->appendMessage(newMessage);
    }
    //TODO: сделать обновление на клиенте двигая локальный вектор, а если нет чата такого то только тогда вызвать этот метод
    getChatsList();
}

void MainWindow::on_messageAccepted(const ParsedMessageAcceptedObject &msgAccObj)
{
    if (msgAccObj.clientMessageId.isEmpty() || msgAccObj.messageId == 0 || msgAccObj.chatId == 0)
    {
        return;
    }

    auto chatIt = chatMessages.find(msgAccObj.chatId);
    if (chatIt == chatMessages.end())
    {
        return;
    }

    auto &messages = chatIt.value();
    for (auto rit = messages.rbegin(); rit != messages.rend(); ++rit)
    {
        if (rit->clientMessageId != msgAccObj.clientMessageId)
            continue;

        rit->messageId = msgAccObj.messageId;
        rit->isPending = false;
        if (!msgAccObj.timestamp.isEmpty())
            rit->timestamp = msgAccObj.timestamp;

        if (currentChatId == msgAccObj.chatId)
            messagesListModel->setMessages(messages);

        return;
    }
}

void MainWindow::on_registrationFinished(const NetworkResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        ui->succesRegistrationLabel->setStyleSheet("color: green;");
        ui->succesRegistrationLabel->setText(res.message);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        ui->succesRegistrationLabel->clear();
        ui->registrationLogin->clear();
        ui->registrationPassword->clear();
        ui->registrationPasswordConfirm->clear();
        accessToken = accToken;
        qDebug() << "Пришедший accessToken от on_registrationFinished " << accToken;
        refreshToken = refToken;
        isAuthorized = true;
        getMyInfo();
        getChatsList();
        websocketController->requestConnectSocket(accessToken);
    }
    else
    {
        isAuthorized = false;
        accessToken = accToken;
        refreshToken = refToken;
        ui->succesRegistrationLabel->setStyleSheet("color: red;");
        ui->succesRegistrationLabel->setText(res.message);
    }
    ui->registrationBtn->setEnabled(true);
    ui->switchToLogInBtn->setEnabled(true);
}

void MainWindow::on_logInFinished(const NetworkResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        ui->succesLogInLabel->setStyleSheet("color: green;");
        ui->succesLogInLabel->setText(res.message);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        ui->succesLogInLabel->clear();
        ui->logInLogIn->clear();
        ui->logInPassword->clear();
        accessToken = accToken;
        refreshToken = refToken;
        isAuthorized = true;
        getMyInfo();
        getChatsList();
        websocketController->requestConnectSocket(accessToken);
    }
    else
    {
        isAuthorized = false;
        accessToken = accToken;
        refreshToken = refToken;
        ui->succesLogInLabel->setStyleSheet("color: red;");
        ui->succesLogInLabel->setText(res.message);
        ui->chatName->setText("Выберите чат");
        currentChatName = "";
    }
    ui->logInBtn->setEnabled(true);
    ui->switchToRegistrationBtn->setEnabled(true);
}

void MainWindow::on_logOutFinished(const NetworkResult &res)
{
    if (res.ok)
    {
        // TODO: Очистить пользовательские данные и поля
        isAuthorized = false;
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
    }
    else
    {
        // TODO: Сообщение об ошибке
#ifdef QT_DEBUG
        qDebug() << "LogOutError";
#endif
    }
}

void MainWindow::on_registrationInProgress()
{
    // Заморозка кнопок на время регистрации
    ui->registrationBtn->setEnabled(false);
    ui->switchToLogInBtn->setEnabled(false);
}

void MainWindow::on_logInProgress()
{
    // Заморозка кнопок на время авторизации
    ui->logInBtn->setEnabled(false);
    ui->switchToRegistrationBtn->setEnabled(false);
}

void MainWindow::on_logOutInProgress()
{
    //TODO: Реализация
}



