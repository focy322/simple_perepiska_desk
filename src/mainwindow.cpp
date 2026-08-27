#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "delegates/chatlistitemdelegate.h"
#include "delegates/searchitemdelegate.h"
#include "widgets/customtitlebar.h"
#include "widgets/imageviewerwindow.h"
#include "widgets/imagecropperdialog.h"
#include "utils/paths.h"
#include "utils/videohelpers.h"
#include "utils/avatarhelper.h"
#include "utils/endpoints.h"

#include <QMenu>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QCloseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QSignalBlocker>
#include <QJsonObject>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QGridLayout>
#include <QUrl>
#include <algorithm>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QScrollBar>
#include <QDesktopServices>
#include <QFileDialog>
#include <QCheckBox>
#include <QBuffer>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QEvent>
#include <functional>
#include <QAbstractButton>

#include <keychain.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif


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
    , requestsStatusManager(new RequestStatusManager(this))
    , retryableRequestErrorHandler(new RetryableRequestErrorHandler(this))
    , refreshAccessTokenTimer(new QTimer(this))
{
    ui->setupUi(this);

    ui->backBtn->setIcon(QIcon(":/icons/back.svg"));
    ui->attachFileBtn->setIcon(QIcon(":/icons/attach.svg"));
    ui->sendMessageBtn->setIcon(QIcon(":/icons/send.svg"));
    ui->backBtn->setIconSize(QSize(24, 24));
    ui->attachFileBtn->setIconSize(QSize(24, 24));
    ui->sendMessageBtn->setIconSize(QSize(24, 24));

    editStatusLabel = new QLabel("Редактирование", this);
    editStatusLabel->setStyleSheet("background-color: #141414; color: #E6E8EB; border: 1px solid #333333; border-radius: 12px; padding: 4px 15px; font-size: 11px;");
    editStatusLabel->hide();

    QVBoxLayout *inputVBox = new QVBoxLayout();
    inputVBox->setContentsMargins(0,0,0,0);
    inputVBox->setSpacing(5);

    QHBoxLayout *editStatusLayout = new QHBoxLayout();
    editStatusLayout->setContentsMargins(0,0,0,0);
    editStatusLayout->addWidget(editStatusLabel);
    editStatusLayout->addStretch();

    inputVBox->addLayout(editStatusLayout);

    stagingWidget = new QWidget(this);
    stagingWidget->setObjectName("stagingWidget");
    stagingWidget->setStyleSheet("QWidget#stagingWidget { background-color: #141414; border: 1px solid #333333; border-radius: 12px; }");
    QVBoxLayout *stagingMainLayout = new QVBoxLayout(stagingWidget);
    stagingMainLayout->setContentsMargins(10, 10, 10, 10);
    stagingMainLayout->setSpacing(5);

    stagingContentWidget = new QWidget(stagingWidget);
    stagingContentWidget->setObjectName("stagingContentWidget");
    stagingContentWidget->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *contentLayout = new QVBoxLayout(stagingContentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(5);

    stagingMediaLayout = new QGridLayout();
    stagingMediaLayout->setSpacing(5);
    contentLayout->addLayout(stagingMediaLayout);

    stagingFilesLayout = new QVBoxLayout();
    stagingFilesLayout->setSpacing(5);
    contentLayout->addLayout(stagingFilesLayout);

    stagingMainLayout->addWidget(stagingContentWidget);

    stagingWidget->hide();

    ui->horizontalSpacerInputLeft->changeSize(170, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->horizontalSpacerInputRight->changeSize(170, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->messageInputLayout->invalidate();

    ui->messageInput->setFixedHeight(40);

    messageInputHorizontalAnim = new QVariantAnimation(this);
    messageInputHorizontalAnim->setDuration(200);
    messageInputHorizontalAnim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(messageInputHorizontalAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        int w = value.toInt();
        ui->horizontalSpacerInputLeft->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
        ui->horizontalSpacerInputRight->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
        ui->messageInputLayout->invalidate();
    });


    messageInputHeightAnim = new QVariantAnimation(this);
    messageInputHeightAnim->setDuration(150);
    messageInputHeightAnim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(messageInputHeightAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        ui->messageInput->setFixedHeight(value.toInt());
    });

    inputVBox->addWidget(stagingWidget);
    inputVBox->addWidget(ui->messageInput);

    ui->messageInputLayout->insertLayout(2, inputVBox);
    ui->messageInputLayout->setAlignment(ui->attachFileBtn, Qt::AlignBottom);
    ui->messageInputLayout->setAlignment(ui->sendMessageBtn, Qt::AlignBottom);
    ui->messageInputLayout->setAlignment(inputVBox, Qt::AlignBottom);

    ui->topMessagesShadow->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->bottomMessagesShadow->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Оставляем стандартные флаги окна, чтобы Windows продолжала считать окно обычным
    // (сохраняются нативные анимации, Aero Snap, двойной клик). Рамка будет скрыта через WM_NCCALCSIZE.
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    CustomTitleBar *titleBar = new CustomTitleBar(this);
    setMenuWidget(titleBar);

    ui->appIcon->installEventFilter(this);
    QSizePolicy spAppIcon = ui->appIcon->sizePolicy();
    spAppIcon.setRetainSizeWhenHidden(true);
    ui->appIcon->setSizePolicy(spAppIcon);
    ui->currentUserAvatar->installEventFilter(this);
    ui->interlocutorAvatar->installEventFilter(this);
    ui->interlocutorAvatar->setCursor(Qt::PointingHandCursor);
    ui->interlocutorAvatar->setAlignment(Qt::AlignCenter);
    setupInterlocutorAvatarPanel();

    windowBorderFrame = new QFrame(this);
    windowBorderFrame->setObjectName("windowBorderFrame");
    windowBorderFrame->setStyleSheet("QFrame#windowBorderFrame { border: 1px solid #2A3037; background: transparent; }");
    windowBorderFrame->setAttribute(Qt::WA_TransparentForMouseEvents);
    windowBorderFrame->show();


    ui->currentUserName->installEventFilter(this);
    ui->currentUserName->setCursor(Qt::PointingHandCursor);
    ui->searchInput->installEventFilter(this);
    ui->messagesView->installEventFilter(this);
    ui->topPanelLayout->setAlignment(Qt::AlignLeft);
    ui->narrowLeftPanel->setMinimumWidth(280);
    ui->narrowLeftPanel->setMaximumWidth(280);
    ui->searchInput->show();
    ui->currentUserName->show();
    connect(ui->searchInput, &QLineEdit::textChanged, this, &MainWindow::on_searchInput_textChanged);
    this->isLeftPanelExpanded = true;

    chatTooltipWidget = new QFrame(this);
    chatTooltipWidget->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    chatTooltipWidget->setObjectName("chatTooltipWidget");
    chatTooltipWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    chatTooltipWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    chatTooltipWidget->setAttribute(Qt::WA_ShowWithoutActivating, true);
    chatTooltipWidget->setWindowOpacity(0.0);
    chatTooltipWidget->hide();

    QFrame *tooltipInnerFrame = new QFrame(chatTooltipWidget);
    tooltipInnerFrame->setObjectName("tooltipInnerFrame");
    tooltipInnerFrame->setStyleSheet("QFrame#tooltipInnerFrame { background-color: #141414; border-radius: 15px; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(chatTooltipWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(tooltipInnerFrame);

    QVBoxLayout *tooltipLayout = new QVBoxLayout(tooltipInnerFrame);
    tooltipLayout->setContentsMargins(20, 10, 15, 10);
    tooltipLayout->setSpacing(2);

    QHBoxLayout *tooltipTopLayout = new QHBoxLayout();
    tooltipTopLayout->setContentsMargins(0, 0, 0, 0);

    tooltipTitleLabel = new QLabel(tooltipInnerFrame);
    QFont titleFont = tooltipTitleLabel->font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    tooltipTitleLabel->setFont(titleFont);
    tooltipTitleLabel->setStyleSheet("color: white; background-color: transparent;");

    tooltipTimeLabel = new QLabel(tooltipInnerFrame);
    QFont timeFont = tooltipTimeLabel->font();
    timeFont.setPixelSize(11);
    tooltipTimeLabel->setFont(timeFont);
    tooltipTimeLabel->setStyleSheet("color: #5f5f5f; background-color: transparent;");

    tooltipTopLayout->addWidget(tooltipTitleLabel);
    tooltipTopLayout->addStretch();
    tooltipTopLayout->addWidget(tooltipTimeLabel);

    QHBoxLayout *tooltipBottomLayout = new QHBoxLayout();
    tooltipBottomLayout->setContentsMargins(0, 0, 0, 0);

    tooltipSubtitleLabel = new QLabel(tooltipInnerFrame);
    QFont subFont = tooltipSubtitleLabel->font();
    subFont.setPixelSize(12);
    tooltipSubtitleLabel->setFont(subFont);
    tooltipSubtitleLabel->setStyleSheet("color: #5f5f5f; background-color: transparent;");

    tooltipBadgeLabel = new QLabel(tooltipInnerFrame);
    QFont badgeFont = tooltipBadgeLabel->font();
    badgeFont.setPixelSize(11);
    badgeFont.setBold(true);
    tooltipBadgeLabel->setFont(badgeFont);
    tooltipBadgeLabel->setStyleSheet("background-color: #faf9f6; color: black; border-radius: 10px; padding: 2px 6px;");
    tooltipBadgeLabel->setAlignment(Qt::AlignCenter);

    tooltipBottomLayout->addWidget(tooltipSubtitleLabel);
    tooltipBottomLayout->addStretch();
    tooltipBottomLayout->addWidget(tooltipBadgeLabel);

    tooltipLayout->addLayout(tooltipTopLayout);
    tooltipLayout->addLayout(tooltipBottomLayout);

    chatTooltipWidget->setFixedSize(220, 60);

    tooltipOpacityAnim = new QPropertyAnimation(chatTooltipWidget, "windowOpacity", this);
    tooltipOpacityAnim->setDuration(150);

    tooltipHideTimer = new QTimer(this);
    tooltipHideTimer->setSingleShot(true);
    tooltipHideTimer->setInterval(50);
    connect(tooltipHideTimer, &QTimer::timeout, this, [this](){
        tooltipOpacityAnim->stop();
        tooltipOpacityAnim->setEndValue(0.0);
        tooltipOpacityAnim->start();
    });
    connect(tooltipOpacityAnim, &QPropertyAnimation::finished, this, [this](){
        if (chatTooltipWidget->windowOpacity() == 0.0) {
            chatTooltipWidget->hide();
        }
    });

    ui->chatsView->viewport()->installEventFilter(this);
    ui->chatsView->viewport()->setMouseTracking(true);
    ui->chatsView->setMouseTracking(true);

    qApp->installEventFilter(this);

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
    connect(chatsController, &ChatsController::createDirectChatFinished, this, &MainWindow::on_createDirectChatFinished);
    connect(chatsController, &ChatsController::editMessageFinished, this, &MainWindow::on_editMessageFinished);
    connect(chatsController, &ChatsController::deleteMessageFinished, this, &MainWindow::on_deleteMessageFinished);
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
    connect(retryableRequestErrorHandler, &RetryableRequestErrorHandler::needRefreshToken, this, &MainWindow::on_needRefreshToken);
    connect(retryableRequestErrorHandler, &RetryableRequestErrorHandler::needImmediateLogOut, this, &MainWindow::on_needImmediateLogOut);
    // Изменение высоты строки ввода собщения при переносе строки
    connect(ui->messageInput, &QTextEdit::textChanged, this, &MainWindow::on_textChanged);

    // Авторизация по Enter
    connect(ui->logInPassword, &QLineEdit::returnPressed, this, &MainWindow::on_logInPassword_returnPressed);

    // Оформление полей ввода пароля: кнопка показа пароля внутри поля
    auto setupRevealBtn = [](QLineEdit* lineEdit, QPushButton* btn) {
        QHBoxLayout *layout = new QHBoxLayout(lineEdit);
        layout->setContentsMargins(0, 0, 10, 0);
        layout->addWidget(btn, 0, Qt::AlignRight | Qt::AlignVCenter);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIcon(QIcon(":/icons/search.svg"));
    };
    setupRevealBtn(ui->logInPassword, ui->revealLogInPasswordBtn);
    setupRevealBtn(ui->registrationPassword, ui->revealRegistrationPasswordBtn);
    setupRevealBtn(ui->registrationPasswordConfirm, ui->revealRegistrationPasswordConfirmBtn);
    ui->messagesView->setMouseTracking(true);
    messagesItemDelegate = new ChatMessagesItemDelegate(ui->messagesView);
    ui->messagesView->setItemDelegate(messagesItemDelegate);
    connect(messagesItemDelegate, &ChatMessagesItemDelegate::editMessageRequested, this, &MainWindow::onEditMessageRequested);
    connect(messagesItemDelegate, &ChatMessagesItemDelegate::deleteMessageRequested, this, &MainWindow::onDeleteMessageRequested);
    connect(chatsController, &ChatsController::editMessageFinished, this, &MainWindow::on_editMessageFinished);

    ui->chatsView->setModel(chatsListModel);
    ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
    ui->chatsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->chatsView->verticalScrollBar()->setSingleStep(15);
    ui->chatsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->chatsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->messagesView->setModel(messagesListModel);
    ui->messagesView->setItemDelegate(messagesItemDelegate);
    ui->messagesView->setResizeMode(QListView::Adjust);
    ui->messagesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->interlocutorNameLabel->hide();
    ui->interlocutorAvatar->hide();
    ui->backBtn->hide();
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
    ui->messageInput->viewport()->installEventFilter(this);

    ui->messageInput->setPlaceholderText("Сообщение...");

#ifndef QT_DEBUG
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/enot_windows.ico"));

    trayIconMenu = new QMenu(this);
    quitAction = new QAction("Выйти из приложения", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayIconMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
#endif

    // Сюда добавлять новые контроллеры
    retryableRequestErrorHandler->getReady({authController, chatsController, filesController, userInfoController, websocketController}, requestsStatusManager);

    refreshAccessTokenTimer->setInterval(accessTokenRefreshInterval);
    refreshAccessTokenTimer->setSingleShot(true);
    connect(refreshAccessTokenTimer, &QTimer::timeout, this, [this]()
    {
        on_needRefreshToken();
    });
    // Эта хуйня должна быть самой последней паосле всех приготовлений
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
        ui->chatsView->setModel(chatsListModel);
        //FixIt: можно хранить в классе а не постоянно аллоцировать
        ui->chatsView->setItemDelegate(new ChatListItemDelegate(ui->chatsView));
    }
    else
    {
        if (ui->chatsView->model() != searchListModel) {
            ui->chatsView->setModel(searchListModel);
            ui->chatsView->setItemDelegate(new SearchItemDelegate(ui->chatsView));
        }
        userInfoController->requestFindUser(accessToken, arg1);
    }
}

void MainWindow::on_chatsView_clicked(const QModelIndex &chatItem)
{
    if (chatItem.isValid())
    {
        if (isLeftPanelExpanded) {
            toggleLeftPanel();
        }
        if (ui->chatsView->model() == searchListModel) {
            unsigned long long userId = chatItem.data(SearchListModel::UserIdRole).toULongLong();
            // fixIt: вместо того чтобы делать запрос на создание чата можно сначало пройтись по списку чатов на наличие уже существуещего
            // и если чат с таким userId уже есть то просто переключиться на него не запрашивая создания чата
            chatsController->requestCreateDirectChat(userId, accessToken);
            ui->searchInput->clear();
            ui->searchInput->clearFocus();
            return;
        }

        saveDraftForChat(currentChatId);
        editingMessageId = ULONG_LONG_MAX;
        hideEditStatusLabelSmoothly();
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
        ui->backBtn->show();

        // Получить информацию о пользователе, чтобы узнать время последнего посещения
        unsigned long long userId = chatItem.data(ChatListModel::UserIdRole).toULongLong();
        if (userId != ULONG_LONG_MAX && userId != 0) {
            RetryableRequest req
            {
                .type = RequestType::REQUEST_GET_USER_INFO,
                .requestFunction = [this, userId](RetryableRequest req)
                {
                    userInfoController->requestUserInfo(accessToken, userId, req);
                },
                .isReplaceable = false
            };
            userInfoController->requestUserInfo(accessToken, userId, req);
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
    bool isMessageEmpty = msgToSend.trimmed().isEmpty() && !hasDraftAttachments;

    if (currentChatId == ULONG_LONG_MAX)
        return;

    if (editingMessageId != ULONG_LONG_MAX && isMessageEmpty)
    {
        quint64 msgIdToDelete = editingMessageId;
        editingMessageId = ULONG_LONG_MAX;
        hideEditStatusLabelSmoothly();
        ui->messageInput->clear();
        onDeleteMessageRequested(msgIdToDelete);
        return;
    }

    if (!isMessageEmpty)
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
            hideEditStatusLabelSmoothly();
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
        updateStagingCloudsUI(currentChatId);

        auto currentChatIt = chatsList.find(currentChatId);
        refreshChatState(currentChatIt, localMessage, true, false);

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

void MainWindow::on_logInPassword_returnPressed()
{
    isLogInEnterPressed = true;
    ui->logInBtn->click();
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
    connect(job, &QKeychain::Job::finished, this, [this, job]()
    {
        const QString token = job->textData().trimmed();
        if (job->error() || token.isEmpty()) {
            isAuthorized = false;
            isFirstOpen = false;
            checkAuthorization(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)}, "", "");
            job->deleteLater();
            return;
        }

        refreshToken = token;
        RetryableRequest req
        {
            .type = RequestType::REQUEST_REFRESH_ACCESS_TOKEN,
            .requestFunction = [this](RetryableRequest req)
            {
                authController->requestRefreshAccessToken(refreshToken, req);
            },
            .isReplaceable = true,
        };
        authController->requestRefreshAccessToken(refreshToken, req);
        job->deleteLater();
    });
    job->start();
}

void MainWindow::getMyInfo()
{
    qDebug() << "Отправленный  accessToken в getMyInfo " << accessToken;
    RetryableRequest req
    {
        .type = RequestType::REQUEST_GET_MY_USER_INFO,
        .requestFunction = [this](RetryableRequest req)
        {
            userInfoController->requestMyUserInfo(accessToken, req);
        },
        .isReplaceable = true,
    };
    userInfoController->requestMyUserInfo(accessToken, req);
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
        on_logOutFinished({.ok = true});
        qDebug() << "authorization Failed!!!";
    }
}

void MainWindow::getChatsList()
{
    RetryableRequest req
    {
        .type = RequestType::REQUEST_MY_CHATS,
        .requestFunction = [this](RetryableRequest req)
            {
                chatsController->requestMyChats(accessToken, req);
            },
        .isReplaceable = true,
    };
    chatsController->requestMyChats(accessToken, req);
}

void MainWindow::getChatMessages(const unsigned long long &chatId)
{
    RetryableRequest req
    {
        .type = RequestType::REQUEST_CHAT_MESSAGES,
        .requestFunction = [this, chatId](RetryableRequest req)
        {
            chatsController->requestChatMessages(chatId, accessToken, req);
        },
        .isReplaceable = false,
    };
    chatsController->requestChatMessages(chatId, accessToken, req);
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
                // Windows увеличивает окно за пределы монитора, а мы сжимаем клиентскую область,
                // чтобы она идеально совпадала с экраном.
                int borderWidth = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int borderHeight = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

                params->rgrc[0].left += borderWidth;
                params->rgrc[0].top += borderHeight;
                params->rgrc[0].right -= borderWidth;
                params->rgrc[0].bottom -= borderHeight;
            }
            *result = 0;
            return true;
        }
        return false;
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
    if (windowBorderFrame) {
        windowBorderFrame->setGeometry(0, 0, width(), height());
        windowBorderFrame->setVisible(!isMaximized() && !isFullScreen());
        windowBorderFrame->raise();
    }

    if (m_baseWindowWidth > 0) {
        m_currentSpacerWidth = qMax(0, 170 + (width() - m_baseWindowWidth) / 2);

        if (stagingWidget && !stagingWidget->isVisible() &&
            (!messageInputHorizontalAnim || messageInputHorizontalAnim->state() != QAbstractAnimation::Running)) {
            ui->horizontalSpacerInputLeft->changeSize(m_currentSpacerWidth, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->horizontalSpacerInputRight->changeSize(m_currentSpacerWidth, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->messageInputLayout->invalidate();
        }
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_initialInputWidthSet) {
        m_initialInputWidthSet = true;
        m_baseWindowWidth = width();
        m_currentSpacerWidth = 170;
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (interlocutorAvatarPanel && interlocutorAvatarPanel->isVisible()) {
            QWidget *w = qobject_cast<QWidget*>(obj);
            bool clickedOnAvatar = false;
            while (w) {
                if (w == ui->interlocutorAvatar) {
                    clickedOnAvatar = true;
                    break;
                }
                w = w->parentWidget();
            }
            if (!clickedOnAvatar) {
                hideInterlocutorAvatarPanel();
            }
        }
    }

    if (obj == ui->currentUserAvatar && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            showAvatarContextMenu(ui->currentUserAvatar->mapToGlobal(mouseEvent->pos()));
            return true;
        }
    }
    if (obj == ui->interlocutorAvatar) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            updateInterlocutorAvatarOutline();
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (interlocutorAvatarPanel && interlocutorAvatarPanel->isVisible()) {
                    hideInterlocutorAvatarPanel();
                } else {
                    showInterlocutorAvatarPanel();
                }
                return true;
            }
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

    if (obj == ui->searchInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            ui->searchInput->clear();
            ui->searchInput->clearFocus();
            return true;
        }
    }

    if (obj == ui->messageInput || obj == ui->messageInput->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            QDropEvent *dragEvent = static_cast<QDropEvent *>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->setDropAction(Qt::CopyAction);
                dragEvent->accept();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
            if (dropEvent->mimeData()->hasUrls()) {
                if (currentChatId != ULONG_LONG_MAX) {
                    QSet<QString> paths;
                    for (const QUrl &url : dropEvent->mimeData()->urls()) {
                        QString filePath = url.toLocalFile();
                        QFileInfo fileChecker(filePath);
                        if (!filePath.isEmpty() && fileChecker.isFile()) {
                            paths.insert(filePath);
                        }
                    }
                    if (!paths.isEmpty()) {
                        ui->messagesView->addPendingFiles(currentChatId, paths);
                    }
                }
                dropEvent->accept();
                return true;
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
                hideEditStatusLabelSmoothly();
                ui->messageInput->clear();
                return true;
            } else if (currentChatId != ULONG_LONG_MAX) {
                closeCurrentChat();
                return true;
            }
        }
    }

    if (obj == ui->messagesView && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (currentChatId != ULONG_LONG_MAX) {
                closeCurrentChat();
                return true;
            }
        }
    }

    if (obj == ui->chatsView->viewport()) {
        if (!isLeftPanelExpanded && ui->narrowLeftPanel->minimumWidth() == 86) {
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                QModelIndex index = ui->chatsView->indexAt(mouseEvent->pos());
                if (index.isValid()) {
                    if (lastHoveredChatIndex != index || chatTooltipWidget->isHidden() || chatTooltipWidget->windowOpacity() == 0.0) {
                        lastHoveredChatIndex = index;
                        tooltipHideTimer->stop();

                        QString title = index.data(ChatListModel::ChatNameRole).toString().trimmed();
                        QString subtitle = index.data(ChatListModel::LastMessageRole).toString().trimmed();
                        QString timestamp = index.data(ChatListModel::LastMessageTimestampRole).toString().trimmed();
                        int unreadCount = index.data(ChatListModel::UnreadCountRole).toInt();

                        tooltipTitleLabel->setText(QFontMetrics(tooltipTitleLabel->font()).elidedText(title, Qt::ElideRight, 130));
                        tooltipSubtitleLabel->setText(QFontMetrics(tooltipSubtitleLabel->font()).elidedText(subtitle, Qt::ElideRight, 150));

                        QString timeDisplay;
                        if (!timestamp.isEmpty()) {
                            QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODateWithMs);
                            if (!dt.isValid()) dt = QDateTime::fromString(timestamp, Qt::ISODate);
                            if (dt.isValid()) timeDisplay = dt.toLocalTime().toString("HH:mm");
                        }
                        tooltipTimeLabel->setText(timeDisplay);

                        if (unreadCount > 0) {
                            tooltipBadgeLabel->setText(unreadCount > 99 ? "99+" : QString::number(unreadCount));
                            tooltipBadgeLabel->show();
                        } else {
                            tooltipBadgeLabel->hide();
                        }

                        QRect visualRect = ui->chatsView->visualRect(index);
                        QPoint globalPos = ui->chatsView->viewport()->mapToGlobal(visualRect.topRight());

                        tooltipOpacityAnim->stop();
                        chatTooltipWidget->hide();
                        chatTooltipWidget->move(globalPos.x() + 30, globalPos.y() + (visualRect.height() - chatTooltipWidget->height()) / 2);
                        chatTooltipWidget->setWindowOpacity(0.0);
                        chatTooltipWidget->show();

                        tooltipOpacityAnim->setStartValue(0.0);
                        tooltipOpacityAnim->setEndValue(1.0);
                        tooltipOpacityAnim->start();
                    }
                } else {
                    lastHoveredChatIndex = QModelIndex();
                    if (!chatTooltipWidget->isHidden() && chatTooltipWidget->windowOpacity() > 0.0) {
                        tooltipHideTimer->start();
                    }
                }
            } else if (event->type() == QEvent::Leave) {
                lastHoveredChatIndex = QModelIndex();
                if (!chatTooltipWidget->isHidden() && chatTooltipWidget->windowOpacity() > 0.0) {
                    tooltipHideTimer->start();
                }
            }
        } else {
            chatTooltipWidget->hide();
            chatTooltipWidget->setWindowOpacity(0.0);
            lastHoveredChatIndex = QModelIndex();
        }
    }

    return QMainWindow::eventFilter(obj, event); // важно пробросить событие дальше
}

void MainWindow::hideEditStatusLabelSmoothly()
{
    if (editStatusLabel && !editStatusLabel->isHidden()) {
        QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(editStatusLabel->graphicsEffect());
        if (!effect) {
            effect = new QGraphicsOpacityEffect(editStatusLabel);
            editStatusLabel->setGraphicsEffect(effect);
        }

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(200);
        anim->setStartValue(effect->opacity());
        anim->setEndValue(0.0);

        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
            editStatusLabel->hide();
            anim->deleteLater();
        });

        anim->start();
    }
}

void MainWindow::closeCurrentChat()
{
    if (currentChatId != ULONG_LONG_MAX) {
        editingMessageId = ULONG_LONG_MAX;
        hideEditStatusLabelSmoothly();
        saveDraftForChat(currentChatId);
        currentChatId = ULONG_LONG_MAX;
        currentChatName = "";
        ui->interlocutorNameLabel->hide();
        ui->interlocutorAvatar->hide();
        ui->backBtn->hide();
        if (interlocutorAvatarPanel && interlocutorAvatarPanel->isVisible()) {
            interlocutorAvatarPanel->hide();
        }
        ui->messageInput->clear();
        ui->messageInput->hide();
        ui->sendMessageBtn->hide();
        ui->attachFileBtn->hide();
        stagingWidget->hide();
        ui->chatsView->clearSelection();
        messagesListModel->clear();
        ui->messagesView->setCurrentChatId(ULONG_LONG_MAX);
    }

    if (!isLeftPanelExpanded) {
        toggleLeftPanel();
    }
}

void MainWindow::on_backBtn_clicked()
{
    closeCurrentChat();
}

void MainWindow::toggleLeftPanel()
{
    if (chatTooltipWidget && !chatTooltipWidget->isHidden()) {
        chatTooltipWidget->hide();
        chatTooltipWidget->setWindowOpacity(0.0);
        if (tooltipOpacityAnim) tooltipOpacityAnim->stop();
        lastHoveredChatIndex = QModelIndex();
    }

    if (isLeftPanelExpanded) {
        QPropertyAnimation *anim = new QPropertyAnimation(ui->narrowLeftPanel, "minimumWidth");
        anim->setDuration(300);
        anim->setStartValue(ui->narrowLeftPanel->width());
        anim->setEndValue(86);
        anim->setEasingCurve(QEasingCurve::InOutCubic);

        QPropertyAnimation *anim2 = new QPropertyAnimation(ui->narrowLeftPanel, "maximumWidth");
        anim2->setDuration(300);
        anim2->setStartValue(ui->narrowLeftPanel->width());
        anim2->setEndValue(86);
        anim2->setEasingCurve(QEasingCurve::InOutCubic);

        QPropertyAnimation *searchAnim = new QPropertyAnimation(ui->searchInput, "maximumWidth");
        searchAnim->setDuration(300);
        searchAnim->setStartValue(ui->searchInput->width());
        searchAnim->setEndValue(0);
        searchAnim->setEasingCurve(QEasingCurve::InOutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim);
        group->addAnimation(anim2);
        group->addAnimation(searchAnim);

        connect(group, &QParallelAnimationGroup::finished, this, [this]() {
            if (!isLeftPanelExpanded) {
                ui->searchInput->hide();
                ui->searchInput->clear();
                ui->currentUserName->hide();
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);

        isLeftPanelExpanded = false;
    } else {
        ui->searchInput->setMaximumWidth(0);
        ui->searchInput->show();

        QPropertyAnimation *anim = new QPropertyAnimation(ui->narrowLeftPanel, "minimumWidth");
        anim->setDuration(300);
        anim->setStartValue(ui->narrowLeftPanel->width());
        anim->setEndValue(280);
        anim->setEasingCurve(QEasingCurve::InOutCubic);

        QPropertyAnimation *anim2 = new QPropertyAnimation(ui->narrowLeftPanel, "maximumWidth");
        anim2->setDuration(300);
        anim2->setStartValue(ui->narrowLeftPanel->width());
        anim2->setEndValue(280);
        anim2->setEasingCurve(QEasingCurve::InOutCubic);

        QPropertyAnimation *searchAnim = new QPropertyAnimation(ui->searchInput, "maximumWidth");
        searchAnim->setDuration(300);
        searchAnim->setStartValue(0);
        searchAnim->setEndValue(184); // 280 - (18+50+10+18)
        searchAnim->setEasingCurve(QEasingCurve::InOutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim);
        group->addAnimation(anim2);
        group->addAnimation(searchAnim);

        connect(group, &QParallelAnimationGroup::finished, this, [this]() {
            if (isLeftPanelExpanded) {
                ui->searchInput->setMaximumWidth(16777215);
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);

        isLeftPanelExpanded = true;
        ui->currentUserName->show();
    }
}


void MainWindow::on_refreshAccessTokenInProgress()
{
    //TODO: везде также
    requestsStatusManager->setStatus(RequestType::REQUEST_REFRESH_ACCESS_TOKEN, RequestState::REQUEST_IN_PROGRESS);
}

void MainWindow::on_refreshAccessTokenFinished(const NetworkResult &res, RetryableRequest req, const QString &accToken, const QString &refToken)
{
    if (isFirstOpen && req.retryCount >= RetryableRequest::maxRetryCount || isFirstOpen && res.ok)
    {
        isFirstOpen = false;
        checkAuthorization(res, accToken, refToken);
    }

    if (res.ok)
    {
        accessToken = accToken;
        refreshToken = refToken;
        requestsStatusManager->setStatus(RequestType::REQUEST_REFRESH_ACCESS_TOKEN, RequestState::REQUEST_SUCCESS);
        retryableRequestErrorHandler->checkRetryableUnauthorizeRequests();
        refreshAccessTokenTimer->start();
        qDebug() << "on_RefreshAccessTokenFinished = true!!!";
    }
    else
    {
        requestsStatusManager->setStatus(RequestType::REQUEST_REFRESH_ACCESS_TOKEN, RequestState::REQUEST_FAILED);
        qDebug() << "on_RefreshAccessTokenFinished = false";
        if (res.error == ERROR_TYPES::UNAUTHORIZED)
            on_logOutFinished({.ok = true});
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

        QFontMetrics metrics(ui->currentUserName->font());
        QString elidedName = metrics.elidedText(currentUsername, Qt::ElideRight, 184);
        ui->currentUserName->setText(elidedName);
        ui->currentUserName->setToolTip(currentUsername);

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
                    RetryableRequest req
                    {
                        .type = RequestType::REQUEST_UPLOAD_AVATAR,
                        .requestFunction = [this, imageData](RetryableRequest req)
                        {
                            userInfoController->requestUploadAvatar(accessToken, imageData, req);
                        },
                        .isReplaceable = true,
                    };
                    userInfoController->requestUploadAvatar(accessToken, imageData, req);
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
        RetryableRequest req
        {
            .type = RequestType::REQUEST_GET_MY_USER_INFO,
            .requestFunction = [this](RetryableRequest req)
            {
                userInfoController->requestMyUserInfo(accessToken, req);
            },
            .isReplaceable = true,
        };
        userInfoController->requestMyUserInfo(accessToken, req);
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
            currentInterlocutorAvatarFull = AvatarHelper::generatePlaceholder(user.nickname.isEmpty() ? user.username : user.nickname, 150);
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
                        currentInterlocutorAvatarFull = pixmap;
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
        currentChatName.clear();
        currentChatId = ULONG_LONG_MAX;
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
        //TODO: можно и тут локально сделать доставая инфу о пользователе при попытки создания чата но как будто впадлу
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
    static int otherFailsCounter = 0;
    if (res.ok)
    {
        qDebug() << "on_socketConnectionFinished  = TRUE!!!";
        otherFailsCounter = 0;
    }
    else
    {
        RetryableRequest req
        {
            .type = RequestType::REQUEST_SOCKET_CONNECT,
            .requestFunction = [this](RetryableRequest req)
            {
                websocketController->requestConnectSocket(accessToken);
            },
            .isReplaceable = true,
        };
        if (res.error == ERROR_TYPES::UNAUTHORIZED)
        {
            retryableRequestErrorHandler->eraseRetryableRequestsByType(RequestType::REQUEST_SOCKET_CONNECT, 0);
            retryableRequestErrorHandler->addRetryableRequest(req, 0);

            if (requestsStatusManager->getStatus(RequestType::REQUEST_REFRESH_ACCESS_TOKEN) != RequestState::REQUEST_IN_PROGRESS)
            {
                RetryableRequest request
                {
                    .type = RequestType::REQUEST_REFRESH_ACCESS_TOKEN,
                    .requestFunction = [this](RetryableRequest request)
                    {
                        authController->requestRefreshAccessToken(refreshToken, request);
                    },
                    .isReplaceable = true,
                };
                authController->requestRefreshAccessToken(refreshToken, request);
            }
        }
        else
        {
            ++otherFailsCounter;
            req.retryCount = otherFailsCounter;
            req.delay = calculateRequestDelay(otherFailsCounter);
            retryableRequestErrorHandler->eraseRetryableRequestsByType(RequestType::REQUEST_SOCKET_CONNECT, 1);
            retryableRequestErrorHandler->addRetryableRequest(req, 1);
        }
        qDebug() << "on_socketConnectionFinished  = FALSE!!!";
        qDebug() << res.message;
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

    auto chatIt = chatsList.find(newMsgChatId);
    if (chatIt != chatsList.end())
        refreshChatState(chatIt, newMessage, true, true);
    else
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

        auto currentChatIt = chatsList.find(msgAccObj.chatId);
        refreshChatState(currentChatIt, *rit, false, false);
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

    if (ui->messageInput->height() != newHeight) {
        if (messageInputHeightAnim->endValue().toInt() != newHeight) {
            messageInputHeightAnim->stop();
            messageInputHeightAnim->setStartValue(ui->messageInput->height());
            messageInputHeightAnim->setEndValue(newHeight);
            messageInputHeightAnim->start();
        }
    }

    saveDraftForChat(currentChatId);
}

void MainWindow::on_findUserInProgress()
{
    requestsStatusManager->setStatus(RequestType::REQUEST_FIND_USER, RequestState::REQUEST_IN_PROGRESS);
}

void MainWindow::on_findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject> &paObjects, const QString& input)
{
    if (res.ok)
    {
        searchListModel->setUsers(paObjects);
        requestsStatusManager->setStatus(RequestType::REQUEST_FIND_USER, RequestState::REQUEST_SUCCESS);
    }
    else
    {
        requestsStatusManager->setStatus(RequestType::REQUEST_FIND_USER, RequestState::REQUEST_FAILED);
        qDebug() << "on_findUserFinished = false!!!";
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

    if (logInBtnAnimation && logInBtnAnimation->state() == QAbstractAnimation::Running)
    {
        logInBtnAnimation->stop();
        ui->logInBtn->setStyleSheet("");
    }
    isLogInEnterPressed = false;

    ui->logInBtn->setEnabled(true);
    ui->switchToRegistrationBtn->setEnabled(true);
}

void MainWindow::on_logOutFinished(const NetworkResult &res)
{
    if (res.ok)
    {
        closeCurrentChat();
        refreshAccessTokenTimer->stop();
        accessToken.clear();
        refreshToken.clear();
        isAuthorized = false;
        chatMessages.clear();
        draftsByChatId.clear();
        ui->messagesView->clearAllFilePaths();
        chatsList.clear();
        chatsListModel->clear();
        messagesListModel->clear();
        currentChatName.clear();
        retryableRequestErrorHandler->clearRetryableRequests(2);
        //TODO: запилить функции setCurrentChatID и setMyUserId чтобы они помимо mainWindow сразу меняли и в моделях
        //TODO: прерывать все запросы на сервер при логауте, чтобы не приходили ответы на них после логаута
        //TODO: бля нахуя я сделал в каждом сервисе по NetWorkManager'у надо объединить в один чтобы нормально все запросы сбросить может в синглтон тоже закатать
        websocketController->requestDisconnectSocket();
        currentChatId = ULONG_LONG_MAX;
        myUserId = ULONG_LONG_MAX;
        ui->messageInput->clear();
        switchPageWithFadeAnimation(ui->loadingAndContentWidgets, ui->contentPage);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
    }
    else
    {
        qDebug() << "LogOutError";
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

    if (isLogInEnterPressed)
    {
        if (!logInBtnAnimation)
        {
            logInBtnAnimation = new QVariantAnimation(this);
            logInBtnAnimation->setDuration(1500);
            logInBtnAnimation->setLoopCount(-1);
            logInBtnAnimation->setKeyValueAt(0.0, QColor("#141414"));
            logInBtnAnimation->setKeyValueAt(0.5, QColor("#E6E8EB"));
            logInBtnAnimation->setKeyValueAt(1.0, QColor("#141414"));

            connect(logInBtnAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
                QColor bgColor = value.value<QColor>();

                qreal progress = qAbs(bgColor.red() - 20) / (qreal)(230 - 20);

                int rFG = 230 - progress * (230 - 20);
                int gFG = 232 - progress * (232 - 20);
                int bFG = 235 - progress * (235 - 20);
                QColor fgC(rFG, gFG, bFG);

                ui->logInBtn->setStyleSheet(QString(
                    "QPushButton { "
                    "background-color: %1; "
                    "color: %2; "
                    "border: 1px solid #333333; "
                    "border-radius: 15px; "
                    "padding: 6px 10px; "
                    "outline: none; "
                    "}"
                ).arg(bgColor.name(), fgC.name()));
            });
        }
        logInBtnAnimation->start();
    }
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
        ui->messageInput->setPlainText(text);
        QTextCursor cursor = ui->messageInput->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->messageInput->setTextCursor(cursor);
    }
    else
    {
        ui->messageInput->clear();
    }
    
    updateStagingCloudsUI(chatId);

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
    updateStagingCloudsUI(chatId);
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

//FixIt : чуть позже

class PreviewOverlayButton : public QPushButton {
public:
    int m_alpha = 0;
    
    PreviewOverlayButton(const QColor& defaultColor, const QColor& hoverBgColor, QWidget* parent = nullptr)
        : QPushButton(parent), m_defaultColor(defaultColor), m_hoverBgColor(hoverBgColor), m_isHovered(false) {
        setFixedSize(18, 18);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("QPushButton { background: transparent; border: none; }");
    }
    
    void setAlpha(int alpha) {
        if (m_alpha != alpha) {
            m_alpha = alpha;
            update();
        }
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event);
        if (m_alpha <= 0) return;
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (m_isHovered) {
            QColor hc = m_hoverBgColor;
            hc.setAlpha((hc.alpha() * m_alpha) / 255);
            painter.setBrush(hc);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(rect());
        }

        QColor dc = m_defaultColor;
        dc.setAlpha((dc.alpha() * m_alpha) / 255);
        painter.setBrush(dc);
        painter.setPen(Qt::NoPen);
        // Внутренний круг
        painter.drawEllipse(4, 4, 10, 10);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override {
#else
    void enterEvent(QEvent* event) override {
#endif
        m_isHovered = true;
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        m_isHovered = false;
        update();
        QPushButton::leaveEvent(event);
    }

private:
    QColor m_defaultColor;
    QColor m_hoverBgColor;
    bool m_isHovered;
};

class OverlayContainer : public QLabel {
public:
    int m_alpha = 0;

    OverlayContainer(QWidget* parent = nullptr) : QLabel(parent) {
        setAttribute(Qt::WA_TranslucentBackground);
    }
    
    void setAlpha(int alpha) {
        if (m_alpha != alpha) {
            m_alpha = alpha;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QLabel::paintEvent(event);
        if (m_alpha <= 0) return;
        
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        int a = (204 * m_alpha) / 255;
        p.setBrush(QColor(36, 42, 49, a));
        p.drawRoundedRect(rect(), 12, 12);
    }
};

class StagingImageLabel : public QLabel {
public:
    QPixmap originalPix;
    OverlayContainer* overlayContainer;
    PreviewOverlayButton* btnEnlarge;
    PreviewOverlayButton* btnDelete;
    QVariantAnimation* overlayAnim;
    
    std::function<void()> onDeleteClicked;
    std::function<void()> onEnlargeClicked;

    StagingImageLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_StyledBackground, false);
        setAlignment(Qt::AlignCenter);

        overlayContainer = new OverlayContainer(this);
        overlayContainer->setFixedSize(60, 24);
        
        overlayAnim = new QVariantAnimation(this);
        overlayAnim->setDuration(150); // плавное появление
        connect(overlayAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            int alpha = value.toInt();
            overlayContainer->setAlpha(alpha);
            btnEnlarge->setAlpha(alpha);
            btnDelete->setAlpha(alpha);
            
            if (alpha == 0) {
                overlayContainer->hide();
            } else {
                overlayContainer->show();
            }
        });
        
        overlayContainer->hide(); // изначально скрыто (alpha = 0)

        QHBoxLayout* overlayLayout = new QHBoxLayout(overlayContainer);
        overlayLayout->setContentsMargins(6, 0, 6, 0);
        overlayLayout->setSpacing(4);

        QColor defaultCircleColor(250, 249, 246);
        
        btnEnlarge = new PreviewOverlayButton(defaultCircleColor, QColor(255, 255, 255, 30), overlayContainer);
        btnDelete = new PreviewOverlayButton(defaultCircleColor, QColor(255, 0, 0, 100), overlayContainer);

        overlayLayout->addWidget(btnEnlarge);
        overlayLayout->addWidget(btnDelete);

        connect(btnDelete, &QAbstractButton::clicked, this, [this]() {
            if (onDeleteClicked) onDeleteClicked();
        });
        connect(btnEnlarge, &QAbstractButton::clicked, this, [this]() {
            if (onEnlargeClicked) onEnlargeClicked();
        });
    }

    void setOriginalPixmap(const QPixmap& pix) {
        originalPix = pix;
        updateGeometry();
        update();
    }

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override {
        if (originalPix.isNull() || originalPix.width() == 0) return w;
        int h = (w * originalPix.height()) / originalPix.width();
        return qMin(h, 400); // 400px максимальная высота
    }

    QSize sizeHint() const override {
        int w = width() > 0 ? width() : 200;
        return QSize(w, heightForWidth(w));
    }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override {
#else
    void enterEvent(QEvent* event) override {
#endif
        QLabel::enterEvent(event);
        overlayAnim->stop();
        overlayAnim->setStartValue(overlayContainer->m_alpha);
        overlayAnim->setEndValue(255);
        overlayAnim->start();
    }

    void leaveEvent(QEvent* event) override {
        QLabel::leaveEvent(event);
        overlayAnim->stop();
        overlayAnim->setStartValue(overlayContainer->m_alpha);
        overlayAnim->setEndValue(0);
        overlayAnim->start();
    }

    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        overlayContainer->move(width() - overlayContainer->width() - 8, 8); // справа сверху
    }

    void paintEvent(QPaintEvent* event) override {
        if (originalPix.isNull()) {
            QLabel::paintEvent(event);
            return;
        }
        
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        
        QPixmap scaled = originalPix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        QPainterPath path;
        path.addRoundedRect(rect(), 8, 8);
        p.setClipPath(path);
        
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
    }
};

void MainWindow::updateStagingCloudsUI(unsigned long long chatId)
{
    if (chatId == ULONG_LONG_MAX) {
        QLayoutItem *item;
        while ((item = stagingMediaLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        while ((item = stagingFilesLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        stagingWidget->hide();
        return;
    }
    
    const auto draftIt = draftsByChatId.constFind(chatId);
    if (draftIt == draftsByChatId.constEnd() || draftIt.value().attachments.isEmpty()) {
        messageInputHorizontalAnim->disconnect();

        if (stagingWidget->isVisible()) {
            QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(stagingWidget->graphicsEffect());
            if (!effect) {
                effect = new QGraphicsOpacityEffect(stagingWidget);
                stagingWidget->setGraphicsEffect(effect);
                effect->setOpacity(1.0);
            }
            if (stagingContentWidget->graphicsEffect()) {
                stagingContentWidget->setGraphicsEffect(nullptr);
            }
            
            QPropertyAnimation *fadeAnim = new QPropertyAnimation(effect, "opacity", this);
            fadeAnim->setDuration(200);
            fadeAnim->setStartValue(effect->opacity());
            fadeAnim->setEndValue(0.0);
            
            connect(fadeAnim, &QPropertyAnimation::finished, this, [this, fadeAnim](){
                if (stagingWidget->graphicsEffect() != fadeAnim->targetObject()) {
                    fadeAnim->deleteLater();
                    return;
                }
                
                stagingWidget->hide();
                stagingWidget->setGraphicsEffect(nullptr);
                stagingContentWidget->setGraphicsEffect(nullptr);
                fadeAnim->deleteLater();
                
                QLayoutItem *item;
                while ((item = stagingMediaLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) item->widget()->deleteLater();
                    delete item;
                }
                while ((item = stagingFilesLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) item->widget()->deleteLater();
                    delete item;
                }
                
                messageInputHorizontalAnim->disconnect();
                connect(messageInputHorizontalAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
                    int w = value.toInt();
                    ui->horizontalSpacerInputLeft->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
                    ui->horizontalSpacerInputRight->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
                    ui->messageInputLayout->invalidate();
                    if (ui->messageInput->parentWidget()) {
                        ui->messageInput->parentWidget()->layout()->activate();
                    }
                });
                
                messageInputHorizontalAnim->setStartValue(ui->horizontalSpacerInputLeft->geometry().width());
                messageInputHorizontalAnim->setEndValue(m_currentSpacerWidth);
                messageInputHorizontalAnim->start();
            });
            fadeAnim->start();
        } else {
            QLayoutItem *item;
            while ((item = stagingMediaLayout->takeAt(0)) != nullptr) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }
            while ((item = stagingFilesLayout->takeAt(0)) != nullptr) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }
            
            ui->horizontalSpacerInputLeft->changeSize(170, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->horizontalSpacerInputRight->changeSize(170, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->messageInputLayout->invalidate();
            if (ui->messageInput->parentWidget()) {
                ui->messageInput->parentWidget()->layout()->activate();
            }
        }
        return;
    }

    messageInputHorizontalAnim->disconnect();
    messageInputHorizontalAnim->stop();
    if (stagingWidget->isVisible()) {
        stagingWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        stagingWidget->setGraphicsEffect(nullptr);
        stagingContentWidget->setGraphicsEffect(nullptr);
    }

    QLayoutItem *item;
    while ((item = stagingMediaLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    while ((item = stagingFilesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    
    int totalMediaCount = 0;
    for (const QJsonValue &value : std::as_const(draftIt.value().attachments)) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString filename = obj.value("filename").toString();
            bool isImage = filename.endsWith(".png", Qt::CaseInsensitive)  || 
                           filename.endsWith(".jpg", Qt::CaseInsensitive)  || 
                           filename.endsWith(".jpeg", Qt::CaseInsensitive) || 
                           filename.endsWith(".bmp", Qt::CaseInsensitive)  || 
                           filename.endsWith(".gif", Qt::CaseInsensitive);
            bool isVideo = filename.endsWith(".mp4", Qt::CaseInsensitive) || 
                           filename.endsWith(".avi", Qt::CaseInsensitive) || 
                           filename.endsWith(".mov", Qt::CaseInsensitive) || 
                           filename.endsWith(".mkv", Qt::CaseInsensitive);
            if (isImage || isVideo) 
                totalMediaCount++;
        }
    }
    
    int cols = 1;
    if (totalMediaCount == 2) cols = 2;
    else if (totalMediaCount == 3) cols = 3;
    else if (totalMediaCount == 4) cols = 2;
    else if (totalMediaCount >= 5) cols = 3;

    int mediaCount = 0;
    int attachIndex = 0;
    
    for (const QJsonValue &value : std::as_const(draftIt.value().attachments)) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString filename = obj.value("filename").toString();
            QString localPath = obj.value("local_path").toString();
            if (localPath.isEmpty()) localPath = appDownloadsDir + "/" + filename;
            
            bool isImage = filename.endsWith(".png", Qt::CaseInsensitive)  ||
                           filename.endsWith(".jpg", Qt::CaseInsensitive)  ||
                           filename.endsWith(".jpeg", Qt::CaseInsensitive) ||
                           filename.endsWith(".bmp", Qt::CaseInsensitive)  ||
                           filename.endsWith(".gif", Qt::CaseInsensitive);
            
            bool isVideo = filename.endsWith(".mp4", Qt::CaseInsensitive) ||
                           filename.endsWith(".avi", Qt::CaseInsensitive) ||
                           filename.endsWith(".mov", Qt::CaseInsensitive) ||
                           filename.endsWith(".mkv", Qt::CaseInsensitive);
            
            if (isImage || isVideo) {
                StagingImageLabel *lbl = new StagingImageLabel();
                lbl->onDeleteClicked = [this, chatId, attachIndex]() {
                    if (draftsByChatId.contains(chatId)) {
                        auto draft = draftsByChatId.value(chatId);
                        if (attachIndex >= 0 && attachIndex < draft.attachments.size()) {
                            draft.attachments.removeAt(attachIndex);
                            draft.attachmentsCount = static_cast<unsigned int>(draft.attachments.size());
                            draft.hasAttachments = !draft.attachments.isEmpty();
                            draftsByChatId.insert(chatId, draft);
                            updateStagingCloudsUI(chatId);
                        }
                    }
                };
                
                lbl->onEnlargeClicked = [this, isImage, localPath, lbl]() {
                    if (isImage && !lbl->originalPix.isNull()) {
                        ImageViewerWindow* viewer = new ImageViewerWindow(lbl->originalPix);
                        viewer->showFullScreen();
                    }
                };
                
                QPixmap pix;
                if (isImage) {
                    pix.load(localPath);
                } else if (isVideo) {
                    pix = VideoThumbnailManager::instance()->getThumbnail(localPath);
                    if (pix.isNull()) {
                        lbl->setText("Загрузка...");
                        // обновление превью после того как оно будет готово
                        connect(VideoThumbnailManager::instance(), &VideoThumbnailManager::thumbnailReady, lbl, [this, localPath, chatId](const QString &readyPath) {
                            if (localPath == readyPath) {
                                updateStagingCloudsUI(chatId);
                            }
                        });
                    }
                }
                
                if (!pix.isNull()) {
                    lbl->setOriginalPixmap(pix);
                }
                
                int row = mediaCount / cols;
                int col = mediaCount % cols;
                stagingMediaLayout->addWidget(lbl, row, col);
                mediaCount++;
            } else {
                QLabel *lbl = new QLabel(filename);
                lbl->setStyleSheet("background-color: #242A31; color: #E6E8EB; border-radius: 8px; padding: 4px 10px; font-size: 11px;");
                lbl->setFixedHeight(24);
                stagingFilesLayout->addWidget(lbl);
            }
        }
        attachIndex++;
    }
    
    if (stagingWidget->layout()) {
        stagingWidget->layout()->invalidate();
        stagingWidget->layout()->activate();
    }
    
    if (!stagingWidget->isVisible()) {
        messageInputHorizontalAnim->disconnect();
        messageInputHorizontalAnim->stop();
        
        connect(messageInputHorizontalAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            int w = value.toInt();
            ui->horizontalSpacerInputLeft->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->horizontalSpacerInputRight->changeSize(w, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
            ui->messageInputLayout->invalidate();
            if (ui->messageInput->parentWidget()) {
                ui->messageInput->parentWidget()->layout()->activate();
            }
        });
        
        messageInputHorizontalAnim->setStartValue(ui->horizontalSpacerInputLeft->geometry().width());
        messageInputHorizontalAnim->setEndValue(100);
        
        connect(messageInputHorizontalAnim, &QVariantAnimation::finished, this, [this](){
            stagingWidget->setMaximumHeight(QWIDGETSIZE_MAX);
            
            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(stagingWidget);
            eff->setOpacity(0.0);
            stagingWidget->setGraphicsEffect(eff);
            
            if (stagingContentWidget->graphicsEffect()) {
                stagingContentWidget->setGraphicsEffect(nullptr);
            }
            
            stagingWidget->show();
            
            QPropertyAnimation *fadeAnim = new QPropertyAnimation(eff, "opacity", this);
            fadeAnim->setDuration(300);
            fadeAnim->setStartValue(0.0);
            fadeAnim->setEndValue(1.0);
            connect(fadeAnim, &QPropertyAnimation::finished, fadeAnim, &QObject::deleteLater);
            fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        });
        
        messageInputHorizontalAnim->start();
    }
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
    on_messageMarkedRead(myUserId, message.first, message.second);
}

void MainWindow::on_messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId)
{
    auto chatIt = chatMessages.find(chatId);
    if (chatIt != chatMessages.end())
    {
        auto &messages = chatIt.value();
        int readCount = 0;

        quint64 lastInterlocutorMsgId = 0;
        for (auto &message : messages)
        {
            if (message.senderId != myUserId)
            {
                if (message.messageId > lastInterlocutorMsgId) {
                    lastInterlocutorMsgId = message.messageId;
                }
            }

            if (userId != myUserId)
            {
                // Собеседник прочитал наши сообщения
                if ((message.senderId == myUserId) && (message.messageId <= lastReadMessageId) && !message.read)
                {
                    message.read = true;
                }
            }
            else
            {
                // Мы прочитали сообщения собеседника
                if ((message.senderId != myUserId) && (message.messageId <= lastReadMessageId) && !message.read)
                {
                    message.read = true;
                    readCount++;
                }
            }
        }

        if (currentChatId == chatId)
        {
            messagesListModel->setMessages(messages);
        }

        if (userId == myUserId)
        {
            if (lastInterlocutorMsgId > 0 && lastReadMessageId >= lastInterlocutorMsgId)
                setUnreadCount(chatId, 0);
            else if (readCount > 0)
                decreaseUnreadCount(chatId, readCount);
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

void MainWindow::on_needRefreshToken()
{
    RetryableRequest req
    {
        .type = RequestType::REQUEST_REFRESH_ACCESS_TOKEN,
        .requestFunction = [this](RetryableRequest req)
        {
            authController->requestRefreshAccessToken(refreshToken, req);
        },
        .isReplaceable = true,
    };
    authController->requestRefreshAccessToken(refreshToken, req);
}

void MainWindow::on_needImmediateLogOut()
{
    on_logOutFinished({.ok = true});
}

void MainWindow::onEditMessageRequested(quint64 messageId, const QString &currentText)
{
    bool wasAlreadyEditing = (editingMessageId != ULONG_LONG_MAX);
    
    editingMessageId = messageId;
    ui->messageInput->setText(currentText);
    ui->messageInput->setFocus();
    
    if (wasAlreadyEditing) {
        if (editStatusLabel) {
            editStatusLabel->show();
            editStatusLabel->setGraphicsEffect(nullptr);
        }
        return;
    }
    
    if (editStatusLabel) {
        editStatusLabel->show();
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(editStatusLabel);
        editStatusLabel->setGraphicsEffect(effect);
        QPropertyAnimation *opacityAnim = new QPropertyAnimation(effect, "opacity");
        opacityAnim->setDuration(300);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void MainWindow::on_editMessageFinished(const NetworkResult &res)
{
    if (!res.ok) {
        // TODO: handle error
    }
}

void MainWindow::onDeleteMessageRequested(quint64 messageId)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Удаление сообщения");
    msgBox.setText("Вы уверены, что хотите удалить это сообщение?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    QCheckBox *cb = new QCheckBox("Удалить у всех", &msgBox);
    msgBox.setCheckBox(cb);

    if (msgBox.exec() == QMessageBox::Yes) {
        bool deleteForAll = cb->isChecked();
        std::vector<quint64> ids = {messageId};
        chatsController->requestDeleteMessage(ids, currentChatId, deleteForAll, accessToken);
        
        auto chatIt = chatMessages.find(currentChatId);
        if (chatIt != chatMessages.end()) {
            auto &msgs = chatIt.value();
            msgs.erase(std::remove_if(msgs.begin(), msgs.end(),
                [messageId](const ParsedChatMessagesArrayObject& m) { return m.messageId == messageId; }),
                msgs.end());
            messagesListModel->setMessages(msgs);
        }
    }
}

void MainWindow::on_deleteMessageFinished(const NetworkResult &res)
{
    if (!res.ok) {
        if (currentChatId != ULONG_LONG_MAX) {
            RetryableRequest req
            {
                .type = RequestType::REQUEST_CHAT_MESSAGES,
                .requestFunction = [this](RetryableRequest req)
                {
                    chatsController->requestChatMessages(currentChatId, accessToken, req);
                },
                .isReplaceable = false,
            };
            chatsController->requestChatMessages(currentChatId, accessToken, req);
        }
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
    //FixIt: целый вектор ради одного сообщения либо сделать тут одельную реализацию под одно сообщение либо что то придумать с оберткой потому что это смех какой то
    std::vector<ParsedChatMessagesArrayObject> messages = {message};
    autoDownloadImages(messages);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
#ifndef QT_DEBUG
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
#else
    event->accept();
#endif
}

#ifndef QT_DEBUG
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible() && !isMinimized()) {
            hide();
        } else {
            showNormal();
            activateWindow();
            raise();
        }
    }
}
#endif

class AvatarOutlineOverlay : public QLabel {
public:
    int m_alpha = 0;
    AvatarOutlineOverlay(QWidget* parent = nullptr) : QLabel(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    void setAlpha(int alpha) {
        if (m_alpha != alpha) {
            m_alpha = alpha;
            update();
        }
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        QLabel::paintEvent(event);
        if (m_alpha <= 0) return;
        
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(255, 255, 255, m_alpha), 2));
        p.setBrush(Qt::NoBrush);
        int cx = parentWidget() ? parentWidget()->width() / 2 : width() / 2;
        int cy = parentWidget() ? parentWidget()->height() / 2 : height() / 2;
        QRect avatarRect(cx - 20, cy - 20, 40, 40);
        // Draw the ellipse matching the original eventFilter logic
        p.drawEllipse(avatarRect.adjusted(-3, -3, 3, 3));
    }
};

void MainWindow::setupInterlocutorAvatarPanel()
{
    AvatarOutlineOverlay* outlineOverlay = new AvatarOutlineOverlay(ui->interlocutorAvatar);
    // Размещаем overlay поверх аватара собеседника
    QVBoxLayout* overlayLayout = new QVBoxLayout(ui->interlocutorAvatar);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addWidget(outlineOverlay);

    interlocutorAvatarPanel = new QFrame(this);
    interlocutorAvatarPanel->setObjectName("interlocutorAvatarPanel");
    interlocutorAvatarPanel->setStyleSheet("#interlocutorAvatarPanel { background-color: #0A0A0A; border: 1px solid #2A3037; border-radius: 8px; }");
    interlocutorAvatarPanel->setFixedSize(170, 190);
    interlocutorAvatarPanel->hide();

    interlocutorAvatarOpacityEffect = new QGraphicsOpacityEffect(interlocutorAvatarPanel);
    interlocutorAvatarPanel->setGraphicsEffect(interlocutorAvatarOpacityEffect);
    interlocutorAvatarOpacityEffect->setOpacity(0.0);

    interlocutorAvatarOutlineAnim = new QVariantAnimation(this);
    interlocutorAvatarOutlineAnim->setDuration(150);
    connect(interlocutorAvatarOutlineAnim, &QVariantAnimation::valueChanged, this, [this, outlineOverlay](const QVariant &val){
        interlocutorAvatarOutlineAlpha = val.toInt();
        outlineOverlay->setAlpha(interlocutorAvatarOutlineAlpha);
    });

    QPushButton* closeBtn = new QPushButton(interlocutorAvatarPanel);
    closeBtn->setFixedSize(12, 12);
    closeBtn->setStyleSheet("QPushButton { background-color: #faf9f6; border-radius: 6px; border: none; }"
                            "QPushButton:hover { background-color: #faf9f6; }");
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::hideInterlocutorAvatarPanel);

    interlocutorAvatarLarge = new QLabel(interlocutorAvatarPanel);
    interlocutorAvatarLarge->setAlignment(Qt::AlignCenter);

    QVBoxLayout* layout = new QVBoxLayout(interlocutorAvatarPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 10, 10, 0);
    topLayout->addStretch();
    topLayout->addWidget(closeBtn);
    
    QHBoxLayout* imgLayout = new QHBoxLayout();
    imgLayout->setContentsMargins(15, 0, 15, 15);
    imgLayout->addWidget(interlocutorAvatarLarge);
    
    layout->addLayout(topLayout);
    layout->addStretch();
    layout->addLayout(imgLayout);

    interlocutorAvatarOpacityAnim = new QPropertyAnimation(interlocutorAvatarOpacityEffect, "opacity");
    interlocutorAvatarOpacityAnim->setDuration(200);
    interlocutorAvatarOpacityAnim->setEasingCurve(QEasingCurve::InOutQuad);
}

void MainWindow::showInterlocutorAvatarPanel()
{
    if (interlocutorAvatarPanel->isVisible()) return;

    QPixmap pix;
    if (!currentInterlocutorAvatarFull.isNull()) {
        pix = currentInterlocutorAvatarFull.scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        pix = ui->interlocutorAvatar->pixmap().scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation);
#else
        pix = ui->interlocutorAvatar->pixmap(Qt::ReturnByValue).scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation);
#endif
    }
    
    interlocutorAvatarLarge->setPixmap(pix);

    QPoint globalAvatarPos = ui->interlocutorAvatar->mapToGlobal(QPoint(0, 0));
    QPoint localAvatarPos = this->mapFromGlobal(globalAvatarPos);

    int finalWidth = 170;
    int finalHeight = 180;
    int finalX = localAvatarPos.x() + ui->interlocutorAvatar->width() / 2 - finalWidth / 2;
    int finalY = localAvatarPos.y() + ui->interlocutorAvatar->height() + 20;

    QRect finalGeometry(finalX, finalY, finalWidth, finalHeight);

    interlocutorAvatarPanel->setGeometry(finalGeometry);
    interlocutorAvatarPanel->raise();
    interlocutorAvatarPanel->show();

    interlocutorAvatarOpacityAnim->setStartValue(0.0);
    interlocutorAvatarOpacityAnim->setEndValue(1.0);

    interlocutorAvatarOpacityAnim->disconnect();

    interlocutorAvatarOpacityAnim->start();
    updateInterlocutorAvatarOutline();
}

void MainWindow::hideInterlocutorAvatarPanel()
{
    if (!interlocutorAvatarPanel->isVisible()) return;

    interlocutorAvatarOpacityAnim->setStartValue(interlocutorAvatarOpacityEffect->opacity());
    interlocutorAvatarOpacityAnim->setEndValue(0.0);
    
    interlocutorAvatarOpacityAnim->disconnect();
    connect(interlocutorAvatarOpacityAnim, &QPropertyAnimation::finished, interlocutorAvatarPanel, [this]() {
        interlocutorAvatarPanel->hide();
        updateInterlocutorAvatarOutline();
    });

    interlocutorAvatarOpacityAnim->start();
    updateInterlocutorAvatarOutline();
}

void MainWindow::updateInterlocutorAvatarOutline()
{
    int targetAlpha = 0;
    if (interlocutorAvatarPanel && interlocutorAvatarPanel->isVisible()) {
        targetAlpha = 80;
    } else if (ui->interlocutorAvatar->underMouse()) {
        targetAlpha = 30;
    }
    
    if (interlocutorAvatarOutlineAnim->endValue().toInt() != targetAlpha) {
        interlocutorAvatarOutlineAnim->stop();
        interlocutorAvatarOutlineAnim->setStartValue(interlocutorAvatarOutlineAlpha);
        interlocutorAvatarOutlineAnim->setEndValue(targetAlpha);
        interlocutorAvatarOutlineAnim->start();
    }
}

void MainWindow::refreshChatState(QHash<unsigned long long, ParsedChatsListArrayObject>::iterator &chatIt,
    const ParsedChatMessagesArrayObject &newMessage, bool isNeedRotation, bool isNeedIncrementUnread)
{
    //TODO: При acctept сообщения можно просто timeStamp и pending менять
    chatIt.value().lastMessage = newMessage.message;
    chatIt.value().lastMessageTimestamp = newMessage.timestamp;
    if (isNeedIncrementUnread)
        chatIt.value().unreadCount += 1;
    chatIt.value().isPending = newMessage.isPending;
    chatIt.value().lastMessageId = newMessage.messageId;
    chatIt.value().lastMessageSenderId = newMessage.senderId;
    chatIt.value().lastMessageAttachmentsCount = newMessage.attachmentsCount;
    chatIt.value().lastMessageHasAttachments = newMessage.hasAttachments;
    if (!newMessage.hasAttachments)
        chatIt.value().lastMessageAttachmentType = "";
    else
    {
        bool hasImage = false;
        bool hasFile = false;
        bool hasVideo = false;
        for (const auto& attachmentValue : std::as_const(newMessage.attachments)) 
        {
            QJsonObject obj = attachmentValue.toObject();
            QString fileName = obj.value("filename").toString();
            bool isImage = fileName.endsWith(".png", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".jpg", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                            fileName.endsWith(".bmp", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".gif", Qt::CaseInsensitive);

            bool isVideo = fileName.endsWith(".mp4", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".m4v", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".mkv", Qt::CaseInsensitive) ||
                            fileName.endsWith(".avi", Qt::CaseInsensitive)  ||
                            fileName.endsWith(".mov", Qt::CaseInsensitive);
            if (isImage) 
                hasImage = true;
            else if (isVideo)
                hasVideo = true;
            else
                hasFile = true;

        }
        if (hasFile) 
            chatIt.value().lastMessageAttachmentType = "File";
        else if (hasImage && hasVideo)
            chatIt.value().lastMessageAttachmentType = "Media";
        else if (hasImage)
            chatIt.value().lastMessageAttachmentType = "Image";
        else if (hasVideo)
            chatIt.value().lastMessageAttachmentType = "Video";

    }
    chatsListModel->upChat(chatIt.value());
}

void MainWindow::setUnreadCount(quint64 chatId, int count)
{
    auto it = chatsList.find(chatId);
    if (it != chatsList.end()) 
    {
        it.value().unreadCount = count;
        chatsListModel->setUnreadCount(chatId, count);
    }
}

void MainWindow::decreaseUnreadCount(quint64 chatId, int count)
{
    auto it = chatsList.find(chatId);
        if (it != chatsList.end())
        {
            if (it.value().unreadCount >= static_cast<unsigned int>(count))
                it.value().unreadCount -= count;
            else
                it.value().unreadCount = 0;
                
            chatsListModel->decreaseUnreadCount(chatId, count);
        }
    
}


