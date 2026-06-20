#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatlistitemdelegate.h"
#include "searchitemdelegate.h"
#include <paths.h>
#include <keychain.h>

#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QSignalBlocker>
#include <QJsonObject>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatsListModel(new ChatListModel(this))
    , searchListModel(new SearchListModel(this))
    , messagesItemDelegate(new ChatMessagesItemDelegate(this))
    , messagesListModel(new ChatMessagesListModel(this))
    , chatMessages()
    , draftsByChatId()
    , currentChatName()
    , currentChatId(ULONG_LONG_MAX)
    , logOutBtn(new QPushButton("Выход", nullptr))
    , authController(new AuthController(this))
    , isAuthorized(false)
    , currentUsername("")
    , myUserId(ULONG_LONG_MAX)
    , userInfoController(new UserInfoController(this))
    , accessToken("")
    , refreshToken("")
    , chatsController(new ChatsController(this))
    , isFirstOpen(true)
    , chatsList{}
    , websocketController(new WebsocketController(this))
    , notificationSound(new QSoundEffect(this))
    , filesController(new FilesController(this))
{
    ui->setupUi(this);
    notificationSound->setSource(QUrl("qrc:/sounds/newMessageSound"));
    notificationSound->setVolume(1.f);
    QDir().mkpath(appDownloadsDir);

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
    connect(userInfoController, &UserInfoController::findUserInProgress, this, &MainWindow::on_findUserInProgress);
    connect(userInfoController, &UserInfoController::findUserFinished, this, &MainWindow::on_findUserFinished);
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
    connect(websocketController, &WebsocketController::messageMarkedRead, this, &MainWindow::on_messageMarkedRead);
    connect(ui->messagesView, &ListViewDragNDrop::gotDragNDropFiles, this, &MainWindow::on_gotDragNDropFiles);
    connect(filesController, &FilesController::uploadFileInProgress, this, &MainWindow::on_uploadFileInProgress);
    connect(filesController, &FilesController::uploadFileFinished, this, &MainWindow::on_uploadFileFinished);
    connect(ui->messagesView, &ListViewDragNDrop::needReadLastMessage, this, &MainWindow::on_needReadLastMessage);
    connect(filesController, &FilesController::downloadFileInfoInProgress, this, &MainWindow::on_downloadFileInfoInProgress);
    connect(filesController, &FilesController::downloadFileInfoFinished, this, &MainWindow::on_downloadFileInfoFinished);
    connect(filesController, &FilesController::downloadFileInProgress, this, &MainWindow::on_downloadFileInProgress);
    connect(filesController, &FilesController::downloadFileFinished, this, &MainWindow::on_downloadFileFinished);
    // Изменение высоты строки ввода собщения при переносе строки
    connect(ui->messageInput, &QTextEdit::textChanged, this, &MainWindow::on_textChanged);



    ui->chatsView->setModel(chatsListModel);
    ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
    ui->searchView->setModel(searchListModel);
    ui->searchView->setItemDelegate(new SearchItemDelegate(ui->searchView));
    ui->messagesView->setModel(messagesListModel);
    ui->messagesView->setItemDelegate(messagesItemDelegate);
    ui->loadingAndContentWidgets->setCurrentWidget(ui->loadingPage);
    ui->chatsAndSearchListsWidgets->setCurrentWidget(ui->chatsListPage);
    ui->searchInput->installEventFilter(this);
    setUpLogOutBtn();

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
        saveDraftForChat(currentChatId);
        currentChatId = chatItem.data(ChatListModel::ChatIdRole).toULongLong();
        ui->messagesView->setCurrentChatId(currentChatId);
        currentChatName = chatItem.data(ChatListModel::ChatNameRole).toString().trimmed();
        ui->chatName->setText(currentChatName);
        auto chatIt = chatMessages.constFind(currentChatId); // Итератор на список сообщений (vector<ParsedChatMessagesArrayObject>) для чата с выбранным chatId
        if (chatIt != chatMessages.constEnd())
        {
            messagesListModel->setMessages(chatIt.value());
        } else
        {
            messagesListModel->clear();
        }
        loadDraftForChat(currentChatId);
        updateSendButtonState(currentChatId);
        // Честно говоря не уверен когда нужно вызвать ее ведь у меня есть вебсокет по сути это лишняя нагрузка на сервер всегда ее вызывать поэтому пока ее else ветку засунул
        // но может быть нужно ее вызывать и при каждом клике на чат для актуализации сообщений, хз
        getChatMessages(currentChatId);


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
    //TODO: что то тут может быть не чисто с toPlainText
    QString msgToSend = stripAttachmentMarker(ui->messageInput->toPlainText());
    const auto draftIt = draftsByChatId.constFind(currentChatId);
    bool hasDraftAttachments = draftIt != draftsByChatId.constEnd() && !draftIt.value().attachments.isEmpty();
    bool condToSendMsg = currentChatId != ULONG_LONG_MAX
            && (!msgToSend.trimmed().isEmpty() || hasDraftAttachments); // Условия для отправки сообщения
    if (condToSendMsg)
    {
        ParsedChatMessagesArrayObject localMessage;
        localMessage.chatId = currentChatId;
        localMessage.senderId = myUserId;
        localMessage.message = msgToSend;
        localMessage.timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QString Uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        localMessage.clientMessageId = Uuid;
        localMessage.isPending = true;
        if (hasDraftAttachments)
        {
            localMessage.attachments = draftIt.value().attachments;
            localMessage.attachmentsCount = static_cast<unsigned int>(localMessage.attachments.size());
            localMessage.hasAttachments = !localMessage.attachments.isEmpty();
        }

        chatMessages[currentChatId].push_back(localMessage);
        messagesListModel->appendMessage(localMessage);
        websocketController->requestSendMessage(localMessage);
        ui->messageInput->clear();
        draftsByChatId.remove(currentChatId);

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
    auto *job = new QKeychain::ReadPasswordJob("Vent", this);
    job->setKey("vent_refresh_token");
    connect(job, &QKeychain::Job::finished, this, [this, job]() {
        const QString token = job->textData().trimmed();
        if (job->error() || token.isEmpty()) {
            isAuthorized = false;
            isFirstOpen = false;
            checkAuthorization(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)}, "", "");
            job->deleteLater();
            return;
        }

        refreshToken = token;
        authController->requestRefreshAccessToken(refreshToken);
        job->deleteLater();
    });
    job->start();
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

void MainWindow::switchPageWithSlideAnimation(QStackedWidget *stackedWidget, QWidget *newPage)
{
    if (!stackedWidget || !newPage) return;

    if (pageSwitchAnimation)
    {
        pageSwitchAnimation->stop();
        pageSwitchAnimation->deleteLater();
        pageSwitchAnimation = nullptr;

        QWidget *current = stackedWidget->currentWidget();
        if (current)
        {
            current->setGraphicsEffect(nullptr);
            current->setGeometry(stackedWidget->rect());
        }
        newPage->setGraphicsEffect(nullptr);
        newPage->setGeometry(stackedWidget->rect());

        if (current == newPage)
            return;
    }

    const int duration = 300; // total duration (ms)
    const int half = duration / 2;

    QWidget *oldPage = stackedWidget->currentWidget();
    if (oldPage == newPage) return;

    // Ensure pages have correct geometry and are visible for animation
    QRect area = stackedWidget->rect();
    oldPage->setGeometry(area);
    newPage->setGeometry(QRect(area.width(), 0, area.width(), area.height()));
    newPage->show();

    // Opacity effects
    auto *oldEffect = new QGraphicsOpacityEffect(oldPage);
    auto *newEffect = new QGraphicsOpacityEffect(newPage);
    oldEffect->setOpacity(1.0);
    newEffect->setOpacity(0.0);
    oldPage->setGraphicsEffect(oldEffect);
    newPage->setGraphicsEffect(newEffect);

    // Slide animations
    auto *slideOut = new QPropertyAnimation(oldPage, "geometry");
    slideOut->setDuration(duration);
    slideOut->setStartValue(oldPage->geometry());
    slideOut->setEndValue(QRect(-area.width(), 0, area.width(), area.height()));
    slideOut->setEasingCurve(QEasingCurve::InOutQuad);

    auto *slideIn = new QPropertyAnimation(newPage, "geometry");
    slideIn->setDuration(duration);
    slideIn->setStartValue(QRect(area.width(), 0, area.width(), area.height()));
    slideIn->setEndValue(QRect(0, 0, area.width(), area.height()));
    slideIn->setEasingCurve(QEasingCurve::InOutQuad);

    // Fade animations with delay: pause then fade over second half
    auto *fadeOutSeq = new QSequentialAnimationGroup(this);
    fadeOutSeq->addAnimation(new QPauseAnimation(half));
    auto *fadeOut = new QPropertyAnimation(oldEffect, "opacity");
    fadeOut->setDuration(half);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);
    fadeOutSeq->addAnimation(fadeOut);

    auto *fadeInSeq = new QSequentialAnimationGroup(this);
    fadeInSeq->addAnimation(new QPauseAnimation(half));
    auto *fadeIn = new QPropertyAnimation(newEffect, "opacity");
    fadeIn->setDuration(half);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
    fadeInSeq->addAnimation(fadeIn);

    // Parallel group: slides + fades (sequences)
    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(slideOut);
    group->addAnimation(slideIn);
    group->addAnimation(fadeOutSeq);
    group->addAnimation(fadeInSeq);

    pageSwitchAnimation = group;

    connect(group, &QParallelAnimationGroup::finished, this, [this, stackedWidget, newPage, oldPage]() {
        // finalize: switch the stacked widget page
        stackedWidget->setCurrentWidget(newPage);
        // restore geometries
        oldPage->setGeometry(stackedWidget->rect());
        newPage->setGeometry(stackedWidget->rect());

        oldPage->setGraphicsEffect(nullptr);
        newPage->setGraphicsEffect(nullptr);
        pageSwitchAnimation = nullptr;

        // effects will be cleaned up automatically with widgets
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionLogoutButton();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->searchInput) { // Проверяем, что событие пришло именно от нашего поля
        if (event->type() == QEvent::FocusIn) {
            // Когда пользователь кликнул в поле или перешел Tab-ом
            switchPageWithSlideAnimation(ui->chatsAndSearchListsWidgets, ui->searchListPage);
            //ui->chatsAndSearchListsWidgets->setCurrentWidget(ui->searchListPage); // Индекс страницы, на которую нужно перейти
        }
        else if (event->type() == QEvent::FocusOut) {
            // Возвращаем основную страницу
            switchPageWithSlideAnimation(ui->chatsAndSearchListsWidgets, ui->chatsListPage);
            //ui->chatsAndSearchListsWidgets->setCurrentWidget(ui->chatsListPage);
            ui->searchInput->clear();
        }
    }

    return QMainWindow::eventFilter(obj, event); // Важно пробросить событие дальше
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
        this->myUserId = userId;
        messagesItemDelegate->setCurrentUserId(this->myUserId);
#ifdef QT_DEBUG
        qDebug() << "currentUsername" << currentUsername;
        qDebug() << "currentUserId" << this->myUserId;
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
        if (!paObjects.empty())
        {
            // TODO: одинаковые сообещения накладываются друг на друга сверху
            //chatMessages[chatId].insert(chatMessages[chatId].cbegin(), paObjects.cbegin(), paObjects.cend());
            chatMessages[chatId] = paObjects; // TODO: если работает то что выше то это удалить
            if (currentChatId == chatId)
                messagesListModel->setMessages(chatMessages[chatId]);
            //messagesListModel->setMessages(paObjects); // TODO: если работает то что выше то это удалить
        }
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
    if (currentChatId != newMsgChatId)
        notificationSound->play();

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
            //TODO: может быть можно как то перерисовать без переприсваивания вектора
            messagesListModel->setMessages(messages);

        //TODO: на клиенте переставлсять все
        getChatsList();
        return;
    }
}

void MainWindow::on_textChanged()
{
    int docHeight = ui->messageInput->document()->size().height();

    int minHeight = 40;
    int maxHeight = 200;

    int newHeight = qMax(minHeight, docHeight);
    newHeight = qMin(newHeight, maxHeight);

    ui->messageInput->setFixedHeight(newHeight);
    saveDraftForChat(currentChatId);
}

void MainWindow::on_findUserInProgress()
{

}

void MainWindow::on_findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject> &paObjects)
{
    if (res.ok)
    {
        searchListModel->setUsers(paObjects);
#ifdef QT_DEBUG
        qDebug() << "on_findUserFinished = true!!!";
#endif
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "on_findUserFinished = false!!!";
#endif
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
        chatMessages.clear();
        draftsByChatId.clear();
        ui->messagesView->clearAllFilePaths();
        chatsList.clear();
        chatsListModel->clear();
        messagesListModel->clear();
        currentChatName.clear();
        ui->chatName->clear();
        currentChatId = ULONG_LONG_MAX;
        myUserId = ULONG_LONG_MAX;
        ui->messageInput->clear();
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

void MainWindow::on_searchView_clicked(const QModelIndex &user)
{
    if (user.isValid())
    {
        unsigned long long userId = user.data(SearchListModel::UserIdRole).toULongLong();
        for (const auto &chatListItem : std::as_const(chatsList))
        {
            if (chatListItem.userId == userId)
            {
                saveDraftForChat(currentChatId);
                currentChatId = chatListItem.chatId;
                ui->messagesView->setCurrentChatId(currentChatId);
                currentChatName = chatListItem.chatName;
                ui->chatName->setText(currentChatName);
                auto chatIt = chatMessages.constFind(currentChatId); // Итератор на список сообщений (vector<ParsedChatMessagesArrayObject>) для чата с выбранным chatId
                if (chatIt != chatMessages.constEnd())
                {
                    messagesListModel->setMessages(chatIt.value());
                } else
                {
                    messagesListModel->clear();
                    // Честно говоря не уверен когда нужно вызвать ее ведь у меня есть вебсокет по сути это лишняя нагрузка на сервер всегда ее вызывать поэтому пока ее else ветку засунул
                    // но может быть нужно ее вызывать и при каждом клике на чат для актуализации сообщений, хз
                    // по крайней мере пока не будет кэша сообщений придется вызывать
                    getChatMessages(currentChatId);
                }
                loadDraftForChat(currentChatId);
                updateSendButtonState(currentChatId);
                ui->searchInput->clearFocus();
                return;
            }
        }
        createDirectChat(userId);
        ui->searchInput->clearFocus();
    }
    else
    {
        // По сути он невалидным быть не может поэтому хз что тут добавить
        ui->searchInput->clearFocus();
    }
}

void MainWindow::saveDraftForChat(unsigned long long chatId)
{
    if (chatId == ULONG_LONG_MAX)
        return;

    ParsedChatMessagesArrayObject draft = draftsByChatId.value(chatId);
    draft.chatId = chatId;
    draft.message = stripAttachmentMarker(ui->messageInput->toPlainText());

    if (draft.message.trimmed().isEmpty() && draft.attachments.isEmpty())
    {
        draftsByChatId.remove(chatId);
        return;
    }

    draftsByChatId.insert(chatId, draft);
}

void MainWindow::loadDraftForChat(unsigned long long chatId)
{
    QSignalBlocker blocker(ui->messageInput);
    const auto draftIt = draftsByChatId.constFind(chatId);
    if (draftIt != draftsByChatId.constEnd())
    {
        QString text = draftIt.value().message;
        if (!draftIt.value().attachments.isEmpty())
        {
            QStringList ids;
            for (const QJsonValue &value : std::as_const(draftIt.value().attachments))
            {
                if (value.isObject())
                {
                    const QJsonObject obj = value.toObject();
                    const QString filename = obj.value("filename").toString();
                    if (!filename.isEmpty())
                    {
                        ids.append(filename);
                        continue;
                    }
                    if (obj.value("file_id").isDouble())
                        ids.append(QString::number(static_cast<qulonglong>(obj.value("file_id").toDouble())));
                }
                else if (value.isDouble())
                {
                    ids.append(QString::number(static_cast<qulonglong>(value.toDouble())));
                }
                else if (value.isString())
                {
                    ids.append(value.toString());
                }
            }
            if (!ids.isEmpty())
                text.prepend("[attachments: " + ids.join(", ") + "]\n");
        }
        ui->messageInput->setPlainText(text);
        QTextCursor cursor = ui->messageInput->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->messageInput->setTextCursor(cursor);
    }
    else
    {
        ui->messageInput->clear();
    }

    on_textChanged();
}

void MainWindow::appendAttachmentToDraft(unsigned long long chatId, const ParsedUploadedFileInfo &fileInfo)
{
    if (chatId == ULONG_LONG_MAX)
        return;

    ParsedChatMessagesArrayObject draft = draftsByChatId.value(chatId);
    draft.chatId = chatId;
    QJsonObject attachment;
    attachment.insert("file_id", static_cast<qint64>(fileInfo.fileId));
    attachment.insert("filename", fileInfo.filename);
    draft.attachments.append(attachment);
    draft.attachmentsCount = static_cast<unsigned int>(draft.attachments.size());
    draft.hasAttachments = !draft.attachments.isEmpty();

    draftsByChatId.insert(chatId, draft);
}

void MainWindow::updateSendButtonState(unsigned long long chatId)
{
    if (chatId == ULONG_LONG_MAX)
    {
        ui->sendMessageBtn->setEnabled(false);
        return;
    }

    bool hasPendingFiles = ui->messagesView->hasPendingFiles(chatId);
    ui->sendMessageBtn->setEnabled(!hasPendingFiles);
}

QString MainWindow::stripAttachmentMarker(const QString &text) const
{
    QStringList lines = text.split('\n');
    if (!lines.isEmpty() && lines.first().trimmed().startsWith("[attachments:"))
        lines.removeFirst();

    return lines.join("\n");
}


void MainWindow::on_searchInput_returnPressed()
{
    QString input = ui->searchInput->text();
    bool condToFindUser = !input.trimmed().isEmpty(); // Возможно нужно будет добавить поболее условий по типу проверки что конкретно написано только пока хз каких
    if (condToFindUser)
    {
        userInfoController->requestFindUser(accessToken, input);
    }
    else
        ui->searchInput->clear();
}

void MainWindow::on_gotDragNDropFiles()
{
    if (currentChatId == ULONG_LONG_MAX)
    {
        ui->messagesView->clearAllFilePaths(); // как будто бы можно просто return сделать
        return;
    }

    const QSet<QString> filePaths = ui->messagesView->getFilePaths(currentChatId);
    if (filePaths.isEmpty())
        return;

    updateSendButtonState(currentChatId);
    filesController->requestUploadFile(this->accessToken, filePaths, this->currentChatId);

}

void MainWindow::on_uploadFileInProgress()
{
    updateSendButtonState(currentChatId);
#ifdef QT_DEBUG
    qDebug() << "on_uploadFileInProgress";
#endif

}

void MainWindow::on_uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo)
{
#ifdef QT_DEBUG
    qDebug() << "on_uploadFileFinished " << res.ok;
#endif
    ui->messagesView->removeFileByPath(chatId, filePath);
    if (res.ok)
    {
        appendAttachmentToDraft(chatId, fileInfo);
        if (currentChatId == chatId)
            loadDraftForChat(chatId);
    }

    if (currentChatId == chatId)
        updateSendButtonState(chatId);

}

// Скачивание сразу всех вложений при нажатии на сообщение
void MainWindow::on_messagesView_clicked(const QModelIndex &index)
{
    bool hasAttachments = index.data(ChatMessagesListModel::HasAttachmentsRole).toBool();
    if (hasAttachments)
    {
        std::vector<quint64> fileIds{};
        QJsonArray attachments = index.data(ChatMessagesListModel::AttachmentsRole).toJsonArray();
        for (const auto &attachmentValue : std::as_const(attachments))
        {
            QJsonObject attachmentObj = attachmentValue.toObject();
            fileIds.push_back(static_cast<quint64>(attachmentObj.value("file_id").toInteger(-1)));
        }
        filesController->requestDownloadFileInfo(accessToken, fileIds);
    }

}

void MainWindow::on_needReadLastMessage(const std::pair<quint64, quint64> &message)
{
    chatsController->requestMarkMessageRead(message, accessToken);
}

void MainWindow::on_messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId)
{
    Q_UNUSED(userId);
    auto chatIt = chatMessages.find(chatId);
    if (chatIt != chatMessages.end())
    {
        auto &messages = chatIt.value();
        //Возможно есть какой-то более красивый алгоритм чем перебор всего вектора сообщений
        for (auto &message : messages)
        {
            if ((message.senderId == myUserId) && (message.messageId <= lastReadMessageId) && !message.read)
            {
                message.read = true;
                if (message.messageId == lastReadMessageId)
                {
                    if (currentChatId == chatId)
                    {
                        messagesListModel->setMessages(messages);
                    }
                    return;
                }
            }
        }

    }
}

void MainWindow::on_downloadFileInfoInProgress()
{

}

void MainWindow::on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo)
{

}

void MainWindow::on_downloadFileInProgress()
{

}

void MainWindow::on_downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo)
{

}

