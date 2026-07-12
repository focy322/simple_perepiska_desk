#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "delegates/chatlistitemdelegate.h"
#include "delegates/searchitemdelegate.h"
#include "utils/paths.h"
#include "widgets/customtitlebar.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

#include <keychain.h>

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "utils/endpoints.h"
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QScrollBar>
#include <QMenu>
#include <QFileDialog>
#include <QBuffer>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include "widgets/imagecropperdialog.h"
#include "utils/avatarhelper.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QSignalBlocker>
#include <QJsonObject>
#include <QDir>

static void crossfadeAppIcon(QLabel *appIcon, const QString &iconPath)
{
    if (appIcon->pixmap(Qt::ReturnByValue).isNull()) {
        if (iconPath.endsWith(".svg")) {
            appIcon->setPixmap(QIcon(iconPath).pixmap(appIcon->size()));
        } else {
            appIcon->setPixmap(QPixmap(iconPath));
        }
        return;
    }

    QLabel *tempLabel = new QLabel(appIcon->parentWidget());
    tempLabel->setGeometry(appIcon->geometry());
    tempLabel->setPixmap(appIcon->pixmap(Qt::ReturnByValue));
    tempLabel->setScaledContents(appIcon->hasScaledContents());
    tempLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    tempLabel->setStyleSheet("background-color: transparent;");
    tempLabel->show();

    if (iconPath.endsWith(".svg")) {
        appIcon->setPixmap(QIcon(iconPath).pixmap(appIcon->size()));
    } else {
        appIcon->setPixmap(QPixmap(iconPath));
    }

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(tempLabel);
    tempLabel->setGraphicsEffect(effect);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(200);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    QObject::connect(anim, &QPropertyAnimation::finished, tempLabel, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

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
    
    editAnimationLabel = new QLabel("Редактирование", this);
    editAnimationLabel->setAlignment(Qt::AlignCenter);
    editAnimationLabel->hide();
    editAnimationLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->messagesViewLayout->addWidget(editAnimationLabel, 0, 0, Qt::AlignCenter);
    
    ui->topMessagesShadow->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->bottomMessagesShadow->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Оставляем стандартные флаги окна, чтобы Windows продолжала считать окно обычным 
    // (сохраняются нативные анимации, Aero Snap, двойной клик). Рамка будет скрыта через WM_NCCALCSIZE.
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    CustomTitleBar *titleBar = new CustomTitleBar(this);
    setMenuWidget(titleBar);

    ui->appIcon->installEventFilter(this);
    ui->currentUserAvatar->installEventFilter(this);
    ui->currentUserName->installEventFilter(this);
    ui->currentUserName->setCursor(Qt::PointingHandCursor);
    ui->searchInput->installEventFilter(this);
    ui->topPanelLayout->setAlignment(Qt::AlignLeft);
    ui->searchInput->hide();
    ui->currentUserName->hide();
    connect(ui->searchInput, &QLineEdit::textChanged, this, &MainWindow::on_searchInput_textChanged);
    this->isLeftPanelExpanded = false;

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
    connect(userInfoController, &UserInfoController::getUserInfoFinished, this, &MainWindow::on_getUserInfoFinished);
    connect(userInfoController, &UserInfoController::uploadAvatarFinished, this, &MainWindow::on_uploadAvatarFinished);
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

    ui->messagesView->setMouseTracking(true);
    connect(messagesItemDelegate, &ChatMessagesItemDelegate::editMessageRequested, this, &MainWindow::onEditMessageRequested);
    connect(chatsController, &ChatsController::editMessageFinished, this, &MainWindow::on_editMessageFinished);

    ui->chatsView->setModel(chatsListModel);
    ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
    ui->chatsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->chatsView->verticalScrollBar()->setSingleStep(15);
    ui->messagesView->setModel(messagesListModel);
    ui->messagesView->setItemDelegate(messagesItemDelegate);
    ui->messagesView->setResizeMode(QListView::Adjust);
    ui->messagesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->interlocutorNameLabel->hide();
    ui->interlocutorAvatar->hide();
    ui->messageInput->hide();
    ui->sendMessageBtn->hide();
    ui->attachFileBtn->hide();

    QGraphicsDropShadowEffect *nameShadow = new QGraphicsDropShadowEffect(this);
    nameShadow->setBlurRadius(15);
    nameShadow->setOffset(0, 4);
    nameShadow->setColor(QColor(0, 0, 0, 80));
    ui->interlocutorNameLabel->setGraphicsEffect(nameShadow);

    QGraphicsDropShadowEffect *inputShadow = new QGraphicsDropShadowEffect(this);
    inputShadow->setBlurRadius(15);
    inputShadow->setOffset(0, 4);
    inputShadow->setColor(QColor(0, 0, 0, 80));
    ui->messageInput->setGraphicsEffect(inputShadow);

    QGraphicsDropShadowEffect *btnShadow = new QGraphicsDropShadowEffect(this);
    btnShadow->setBlurRadius(15);
    btnShadow->setOffset(0, 4);
    btnShadow->setColor(QColor(0, 0, 0, 80));
    ui->sendMessageBtn->setGraphicsEffect(btnShadow);

    ui->loadingAndContentWidgets->setCurrentWidget(ui->loadingPage);
    ui->messageInput->installEventFilter(this);

    tryAuthorize();


}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_searchInput_textChanged(const QString &arg1)
{
    if (arg1.isEmpty()) {
        searchListModel->clear();
    } else {
        userInfoController->requestFindUser(accessToken, arg1);
    }
}

void MainWindow::on_chatsView_clicked(const QModelIndex &chatItem)
{
    if (chatItem.isValid())
    {
        if (ui->chatsView->model() == searchListModel) {
            unsigned long long userId = chatItem.data(SearchListModel::UserIdRole).toULongLong();
            chatsController->requestCreateDirectChat(userId, accessToken);
            ui->searchInput->clear();
            ui->searchInput->clearFocus();
            return;
        }

        saveDraftForChat(currentChatId);
        currentChatId = chatItem.data(ChatListModel::ChatIdRole).toULongLong();
        ui->messagesView->setCurrentChatId(currentChatId);
        currentChatName = chatItem.data(ChatListModel::ChatNameRole).toString().trimmed();
        
        // Сначала установка стандартного названия чата
        ui->interlocutorNameLabel->setText(currentChatName);
        ui->interlocutorNameLabel->show();
        ui->interlocutorAvatar->show();
        ui->messageInput->show();
        ui->sendMessageBtn->show();
        ui->attachFileBtn->show();
        
        // Получить информацию о пользователе, чтобы узнать время последнего посещения
        unsigned long long userId = chatItem.data(ChatListModel::UserIdRole).toULongLong();
        if (userId != ULONG_LONG_MAX && userId != 0) {
            userInfoController->requestUserInfo(accessToken, userId);
        }
        
        auto chatIt = chatMessages.constFind(currentChatId); // Итератор на список сообщений (vector<ParsedChatMessagesArrayObject>) для чата с выбранным chatId
        if (chatIt != chatMessages.constEnd())
        {
            messagesListModel->setMessages(chatIt.value());
            ui->messagesView->scrollToBottom();
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


void MainWindow::on_attachFileBtn_clicked()
{
    if (currentChatId == ULONG_LONG_MAX)
        return;

    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы для отправки", "", "Все файлы (*.*)");
    if (files.isEmpty())
        return;

    QSet<QString> paths(files.begin(), files.end());
    ui->messagesView->addPendingFiles(currentChatId, paths);
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
        if (editingMessageId != ULONG_LONG_MAX)
        {
            chatsController->requestEditMessage(editingMessageId, currentChatId, msgToSend, accessToken);
            
            auto chatIt = chatMessages.find(currentChatId);
            if (chatIt != chatMessages.end())
            {
                for (auto &msg : chatIt.value())
                {
                    if (msg.messageId == editingMessageId)
                    {
                        msg.message = msgToSend;
                        msg.edited = true;
                        break;
                    }
                }
                messagesListModel->setMessages(chatIt.value());
            }
            ui->messageInput->clear();
            editingMessageId = ULONG_LONG_MAX;
            return;
        }

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
        ui->messagesView->scrollToBottom();
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
        switchPageWithFadeAnimation(ui->loadingAndContentWidgets, ui->contentPage);
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

void MainWindow::switchPageWithFadeAnimation(QStackedWidget *stackedWidget, QWidget *newPage)
{
    if (!stackedWidget || !newPage) return;
    QWidget *oldPage = stackedWidget->currentWidget();
    if (oldPage == newPage) return;

    auto *effect = new QGraphicsOpacityEffect(oldPage);
    oldPage->setGraphicsEffect(effect);

    auto *fadeOut = new QPropertyAnimation(effect, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    connect(fadeOut, &QPropertyAnimation::finished, this, [=, this]() {
        stackedWidget->setCurrentWidget(newPage);
        oldPage->setGraphicsEffect(nullptr);

        auto *effect2 = new QGraphicsOpacityEffect(newPage);
        newPage->setGraphicsEffect(effect2);
        auto *fadeIn = new QPropertyAnimation(effect2, "opacity");
        fadeIn->setDuration(300);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        connect(fadeIn, &QPropertyAnimation::finished, this, [=, this]() {
            newPage->setGraphicsEffect(nullptr);
        });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateStartupTransition()
{
    if (ui->loadingAndContentWidgets->currentWidget() != ui->loadingPage) {
        return;
    }

    QLabel *floatingLogo = new QLabel(this);
    floatingLogo->setPixmap(QPixmap(":/images/enot.png"));
    floatingLogo->setScaledContents(true);

    QPoint startPos = ui->loadingLabel->mapTo(this, QPoint(0, 0));
    QRect startGeometry(startPos, ui->loadingLabel->size());
    floatingLogo->setGeometry(startGeometry);
    floatingLogo->show();

    // Скрыть оригинальное лого
    ui->loadingLabel->hide();

    ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
    ui->contentPage->setGeometry(ui->loadingAndContentWidgets->rect());
    ui->contentPage->layout()->activate();

    QPoint endPos = ui->appIcon->mapTo(this, QPoint(0, 0));
    QRect endGeometry(endPos, ui->appIcon->size());

    // Переключиться обратно
    ui->loadingAndContentWidgets->setCurrentWidget(ui->loadingPage);

    if (endGeometry.width() == 0) {
        endGeometry = QRect(10, 10, 40, 40);
    }

    ui->appIcon->hide();

    auto *effect = new QGraphicsOpacityEffect(ui->loadingPage);
    ui->loadingPage->setGraphicsEffect(effect);
    auto *fadeOut = new QPropertyAnimation(effect, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    QPropertyAnimation *flyAnim = new QPropertyAnimation(floatingLogo, "geometry");
    flyAnim->setDuration(600);
    flyAnim->setStartValue(startGeometry);
    flyAnim->setEndValue(endGeometry);
    flyAnim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(fadeOut, &QPropertyAnimation::finished, this, [=, this]() {
        ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
        ui->loadingPage->setGraphicsEffect(nullptr);

        auto *effect2 = new QGraphicsOpacityEffect(ui->contentPage);
        ui->contentPage->setGraphicsEffect(effect2);
        auto *fadeIn = new QPropertyAnimation(effect2, "opacity");
        fadeIn->setDuration(300);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        connect(fadeIn, &QPropertyAnimation::finished, this, [=, this]() {
            ui->contentPage->setGraphicsEffect(nullptr);
            ui->appIcon->show();
            floatingLogo->deleteLater();
            ui->loadingLabel->show();
        });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    flyAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE) {
            NCCALCSIZE_PARAMS *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            if (IsZoomed(msg->hwnd)) {
                HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONULL);
                if (monitor) {
                    MONITORINFO mi;
                    mi.cbSize = sizeof(mi);
                    GetMonitorInfoW(monitor, &mi);
                    params->rgrc[0] = mi.rcWork;
                }
            }
            *result = 0;
            return true;
        }
        return false;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfoW(monitor, &mi)) {
            mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
            mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
            mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
            mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_NCHITTEST) {
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);
        QPoint pos = mapFromGlobal(QPoint(x, y));

        int border_width = 8; // Ширина зоны для захвата курсором

        bool left = pos.x() < border_width;
        bool right = pos.x() > width() - border_width;
        bool top = pos.y() < border_width;
        bool bottom = pos.y() > height() - border_width;

        if (top && left) {
            *result = HTTOPLEFT;
            return true;
        }
        if (top && right) {
            *result = HTTOPRIGHT;
            return true;
        }
        if (bottom && left) {
            *result = HTBOTTOMLEFT;
            return true;
        }
        if (bottom && right) {
            *result = HTBOTTOMRIGHT;
            return true;
        }
        if (left) {
            *result = HTLEFT;
            return true;
        }
        if (right) {
            *result = HTRIGHT;
            return true;
        }
        if (top) {
            *result = HTTOP;
            return true;
        }
        if (bottom) {
            *result = HTBOTTOM;
            return true;
        }

        // Если курсор не на краях, но находится на кастомном TitleBar (высота 24px)
        // и не на кнопках управления (отступ ~90px справа)
        if (pos.y() < 24 && pos.x() < width() - 90) {
            *result = HTCAPTION;
            return true;
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->currentUserAvatar && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            showAvatarContextMenu(ui->currentUserAvatar->mapToGlobal(mouseEvent->pos()));
            return true;
        }
    }

    if (obj == ui->currentUserName && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            showUserNameContextMenu(ui->currentUserName->mapToGlobal(mouseEvent->pos()));
            return true;
        }
    }

    if (obj == ui->appIcon) {
        if (event->type() == QEvent::Enter) {
            crossfadeAppIcon(ui->appIcon, isLeftPanelExpanded ? ":/icons/left_panel_close.svg" : ":/icons/left_panel_open.svg");
            return true;
        } else if (event->type() == QEvent::Leave) {
            crossfadeAppIcon(ui->appIcon, ":/images/enot.png");
            return true;
        } else if (event->type() == QEvent::MouseButtonPress) {
            toggleLeftPanel();
            return true;
        }
    }

    if (obj == ui->searchInput) {
        if (event->type() == QEvent::FocusIn) {
            ui->chatsView->setModel(searchListModel);
            ui->chatsView->setItemDelegate(new SearchItemDelegate(ui->chatsView));
        } else if (event->type() == QEvent::FocusOut) {
            if (ui->searchInput->text().isEmpty()) {
                ui->chatsView->setModel(chatsListModel);
                ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
            }
        }
    }

    if (obj == ui->messageInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (ui->messageInput->toPlainText().isEmpty() && editingMessageId == ULONG_LONG_MAX) {
                if (chatMessages.contains(currentChatId)) {
                    const auto &msgs = chatMessages[currentChatId];
                    for (auto it = msgs.rbegin(); it != msgs.rend(); ++it) {
                        if (it->senderId == myUserId) {
                            onEditMessageRequested(it->messageId, it->message);
                            return true;
                        }
                    }
                }
            }
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier || keyEvent->modifiers() & Qt::ControlModifier) {
                QTextCursor cursor = ui->messageInput->textCursor();
                cursor.insertBlock();
                return true;
            } else {
                on_sendMessageBtn_clicked();
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Escape) {
            if (editingMessageId != ULONG_LONG_MAX) {
                editingMessageId = ULONG_LONG_MAX;
                ui->messageInput->clear();
                return true;
            } else if (currentChatId != ULONG_LONG_MAX) {
                saveDraftForChat(currentChatId);
                currentChatId = ULONG_LONG_MAX;
                currentChatName = "";
                ui->interlocutorNameLabel->hide();
                ui->interlocutorAvatar->hide();
                ui->messageInput->clear();
                ui->messageInput->hide();
                ui->sendMessageBtn->hide();
                ui->attachFileBtn->hide();
                ui->chatsView->clearSelection();
                messagesListModel->clear();
                ui->messagesView->setCurrentChatId(ULONG_LONG_MAX);
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event); // Важно пробросить событие дальше
}

void MainWindow::toggleLeftPanel()
{
    if (isLeftPanelExpanded) {
        QPropertyAnimation *anim = new QPropertyAnimation(ui->narrowLeftPanel, "minimumWidth");
        anim->setDuration(300);
        anim->setStartValue(ui->narrowLeftPanel->width());
        anim->setEndValue(85);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        
        QPropertyAnimation *anim2 = new QPropertyAnimation(ui->narrowLeftPanel, "maximumWidth");
        anim2->setDuration(300);
        anim2->setStartValue(ui->narrowLeftPanel->width());
        anim2->setEndValue(85);
        anim2->setEasingCurve(QEasingCurve::InOutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim);
        group->addAnimation(anim2);
        group->start(QAbstractAnimation::DeleteWhenStopped);

        isLeftPanelExpanded = false;
        ui->searchInput->hide();
        ui->currentUserName->hide();
        ui->searchInput->clear();
        crossfadeAppIcon(ui->appIcon, ":/icons/left_panel_open.svg");
    } else {
        QPropertyAnimation *anim = new QPropertyAnimation(ui->narrowLeftPanel, "minimumWidth");
        anim->setDuration(300);
        anim->setStartValue(ui->narrowLeftPanel->width());
        anim->setEndValue(250);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        
        QPropertyAnimation *anim2 = new QPropertyAnimation(ui->narrowLeftPanel, "maximumWidth");
        anim2->setDuration(300);
        anim2->setStartValue(ui->narrowLeftPanel->width());
        anim2->setEndValue(250);
        anim2->setEasingCurve(QEasingCurve::InOutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim);
        group->addAnimation(anim2);
        group->start(QAbstractAnimation::DeleteWhenStopped);

        isLeftPanelExpanded = true;
        ui->searchInput->show();
        ui->currentUserName->show();
        crossfadeAppIcon(ui->appIcon, ":/icons/left_panel_close.svg");
    }
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

void MainWindow::on_getMyUserInfoFinished(const NetworkResult &res, const QString &username, unsigned long long userId, const QString &avatarUrl)
{
    if (res.ok)
    {
        currentUsername = username;
        this->myUserId = userId;
        this->currentAvatarUrl = avatarUrl;
        messagesItemDelegate->setCurrentUserId(this->myUserId);
        ui->currentUserName->setText(currentUsername);
        
        // Если аватара нет, устанавливаем заглушку
        if (avatarUrl.isEmpty()) {
            ui->currentUserAvatar->setPixmap(AvatarHelper::generatePlaceholder(currentUsername, 50));
        } else {
            // TODO: Скачать настоящий аватар и реализовать кэширование.
            QString fullUrl = avatarUrl;
            if (!fullUrl.startsWith("http")) {
                fullUrl = baseHttpUrl + fullUrl;
            }
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            QNetworkRequest request((QUrl(fullUrl)));
            QNetworkReply *reply = manager->get(request);
            connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        ui->currentUserAvatar->setPixmap(AvatarHelper::makeRoundImage(pixmap, 50));
                    }
                }
                reply->deleteLater();
                manager->deleteLater();
            });
        }
        
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

void MainWindow::showAvatarContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);
    
    auto uploadFunc = [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Выберите аватар", "", "Изображения (*.png *.jpg *.jpeg)");
        if (!filePath.isEmpty()) {
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                ImageCropperDialog cropper(pixmap, this);
                if (cropper.exec() == QDialog::Accepted) {
                    QPixmap cropped = cropper.getCroppedImage();
                    
                    QByteArray imageData;
                    QBuffer buffer(&imageData);
                    buffer.open(QIODevice::WriteOnly);
                    bool saveOk = cropped.save(&buffer, "PNG");
                    if (!saveOk || imageData.isEmpty()) {
                        QMessageBox::warning(this, "Ошибка", "Не удалось обработать изображение. Попробуйте другой файл.");
                        return;
                    }
                    
                    userInfoController->requestUploadAvatar(accessToken, imageData);
                }
            }
        }
    };

    if (this->currentAvatarUrl.isEmpty()) {
        QAction *uploadAction = contextMenu.addAction("Загрузить аватар");
        connect(uploadAction, &QAction::triggered, this, uploadFunc);
    } else {
        QAction *changeAction = contextMenu.addAction("Сменить аватар");
        connect(changeAction, &QAction::triggered, this, uploadFunc);

        QAction *deleteAction = contextMenu.addAction("Удалить аватар");
        connect(deleteAction, &QAction::triggered, this, [this]() {
            QMessageBox::information(this, "Информация", "К сожалению, сервер пока не поддерживает удаление аватара. Попробуйте установить новый.");
        });
    }
    
    contextMenu.exec(pos);
}

void MainWindow::showUserNameContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);
    
    QAction *logoutAction = contextMenu.addAction("Выйти из аккаунта");
    connect(logoutAction, &QAction::triggered, this, [this]() {
        authController->requestLogOut(accessToken, refreshToken);
    });
    
    contextMenu.exec(pos);
}

void MainWindow::on_uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl)
{
    if (res.ok) {
        // Запрашиваем информацию о пользователе, чтобы обновить аватар
        userInfoController->requestMyUserInfo(accessToken);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить аватар: " + res.message);
    }
}

void MainWindow::on_getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user)
{
    if (res.ok && !user.lastSeen.isEmpty())
    {
        QDateTime dt = QDateTime::fromString(user.lastSeen, Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(user.lastSeen, Qt::ISODate);
            
        QString lastSeenText;
        if (dt.isValid()) {
            QDateTime now = QDateTime::currentDateTime();
            if (dt.daysTo(now) == 0) {
                lastSeenText = "Был(а) сегодня в " + dt.toLocalTime().toString("HH:mm");
            } else if (dt.daysTo(now) == 1) {
                lastSeenText = "Был(а) вчера в " + dt.toLocalTime().toString("HH:mm");
            } else {
                lastSeenText = "Был(а) " + dt.toLocalTime().toString("dd.MM.yyyy в HH:mm");
            }
        } else {
            lastSeenText = user.lastSeen;
        }

        QString richText = QString("<div style='text-align: center;'>"
                                   "<span style='font-size: 10pt; font-weight: bold; color: #E6E8EB;'>%1</span><br>"
                                   "<span style='font-size: 8pt; font-weight: normal; color: #8C96A0;'>%2</span>"
                                   "</div>")
                               .arg(currentChatName.toHtmlEscaped())
                               .arg(lastSeenText.toHtmlEscaped());
        
        ui->interlocutorNameLabel->setText(richText);
        
        if (user.avatarFileUrl.isEmpty() || user.avatarFileUrl.isNull()) {
            ui->interlocutorAvatar->setPixmap(AvatarHelper::generatePlaceholder(user.nickname.isEmpty() ? user.username : user.nickname, 40));
        } else {
            // TODO: Загрузить актуальный аватар собеседника
            QString fullUrl = user.avatarFileUrl;
            if (!fullUrl.startsWith("http")) {
                fullUrl = baseHttpUrl + fullUrl;
            }
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            QNetworkRequest request((QUrl(fullUrl)));
            QNetworkReply *reply = manager->get(request);
            connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        ui->interlocutorAvatar->setPixmap(AvatarHelper::makeRoundImage(pixmap, 40));
                    }
                }
                reply->deleteLater();
                manager->deleteLater();
            });
        }
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
        currentChatId = ULONG_LONG_MAX;
        messagesListModel->clear();
#ifdef QT_DEBUG
        qDebug() << "on_getMyChatsFinished = false!!!";
#endif
    }
    
    if (ui->loadingAndContentWidgets->currentWidget() == ui->loadingPage) {
        animateStartupTransition();
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
            if (currentChatId == chatId) {
                messagesListModel->setMessages(chatMessages[chatId]);
                ui->messagesView->scrollToBottom();
            }
            //messagesListModel->setMessages(paObjects); // TODO: если работает то что выше то это удалить
            autoDownloadImages(paObjects);
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

    autoDownloadImages(newMessage);

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

void MainWindow::appendAttachmentToDraft(unsigned long long chatId, const ParsedUploadedFileInfo &fileInfo, const QString &localPath)
{
    if (chatId == ULONG_LONG_MAX)
        return;

    ParsedChatMessagesArrayObject draft = draftsByChatId.value(chatId);
    draft.chatId = chatId;
    QJsonObject attachment;
    attachment.insert("file_id", static_cast<qint64>(fileInfo.fileId));
    attachment.insert("filename", fileInfo.filename);
    if (!localPath.isEmpty()) {
        attachment.insert("local_path", localPath);
    }
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
        appendAttachmentToDraft(chatId, fileInfo, filePath);
        if (currentChatId == chatId)
            loadDraftForChat(chatId);
    }
    else
    {
        QMessageBox::warning(this, "Ошибка загрузки файла", QString("Не удалось загрузить файл.\nНомер ошибки: %1\nОписание: %2").arg(res.error).arg(res.message));
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
    if (res.ok) {
        ui->messagesView->viewport()->update();
    }
}

void MainWindow::onEditMessageRequested(quint64 messageId, const QString &currentText)
{
    editingMessageId = messageId;
    ui->messageInput->setText(currentText);
    ui->messageInput->setFocus();
    
    editAnimationLabel->raise();
    editAnimationLabel->show();
    
    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(400); // Анимация редактирования
    
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
        qreal v = value.toReal();
        int fontSize = 24 + int(60 * v); // Увеличиваем шрифт с 24 до 84
        qreal opacity = 1.0;
        if (v < 0.2) {
            opacity = v / 0.2; // Быстрое появление
        } else {
            opacity = 1.0 - (v - 0.2) / 0.8; // Плавное растворение
        }
        int alpha = int(opacity * 255);
        editAnimationLabel->setStyleSheet(QString("color: rgba(255, 255, 255, %1); font-size: %2px; font-weight: bold; background: transparent;").arg(alpha).arg(fontSize));
    });
    
    connect(anim, &QVariantAnimation::finished, this, [this, anim](){
        editAnimationLabel->hide();
        anim->deleteLater();
    });
    
    anim->start();
}

void MainWindow::on_editMessageFinished(const NetworkResult &res)
{
    if (!res.ok) {
        // TODO: handle error
    }
}

void MainWindow::autoDownloadImages(const std::vector<ParsedChatMessagesArrayObject>& messages)
{
    std::vector<quint64> imageFileIds;
    for (const auto& msg : messages) {
        if (!msg.hasAttachments) continue;
        for (const auto& attachmentValue : std::as_const(msg.attachments)) {
            QJsonObject obj = attachmentValue.toObject();
            QString fileName = obj.value("filename").toString();
            bool isImage = fileName.endsWith(".png", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpg", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                           fileName.endsWith(".bmp", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".gif", Qt::CaseInsensitive);
            
            if (isImage) {
                QString localPath = obj.value("local_path").toString();
                if (!localPath.isEmpty() && QFileInfo::exists(localPath)) continue;

                QString path = appDownloadsDir + "/" + fileName;
                if (!QFileInfo::exists(path)) {
                    quint64 fileId = static_cast<quint64>(obj.value("file_id").toInteger(-1));
                    if (fileId != static_cast<quint64>(-1)) {
                        imageFileIds.push_back(fileId);
                    }
                }
            }
        }
    }
    
    if (!imageFileIds.empty()) {
        filesController->requestDownloadFileInfo(accessToken, imageFileIds);
    }
}

void MainWindow::autoDownloadImages(const ParsedChatMessagesArrayObject& message)
{
    std::vector<ParsedChatMessagesArrayObject> messages = {message};
    autoDownloadImages(messages);
}
