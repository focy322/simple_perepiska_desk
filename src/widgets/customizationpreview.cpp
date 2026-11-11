#include "customizationpreview.h"

#include "ui_mainwindow.h"
#include "customtitlebar.h"
#include "delegates/chatlistitemdelegate.h"
#include "models/chatpreviewmodel.h"
#include "utils/avatarhelper.h"
#include "utils/stylehelper.h"

#include <QHideEvent>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOption>
#include <QVector3D>

namespace
{
    constexpr qreal panelDepth = 112.0;
    constexpr qreal controlDepth = 230.0;
    constexpr qreal chatItemDepth = controlDepth + 72.0;
    constexpr qreal hoverDepth = 32.0;

    QPixmap snapshot(QWidget *widget, qreal devicePixelRatio, bool withChildren)
    {
        QPixmap pixmap(widget->size() * devicePixelRatio);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        pixmap.fill(Qt::transparent);
        auto flags = QWidget::RenderFlags(QWidget::DrawWindowBackground);
        if (withChildren)
            flags |= QWidget::DrawChildren;
        widget->render(&pixmap, QPoint(), QRegion(), flags);
        return pixmap;
    }

    void setupRevealButton(QLineEdit *field, QPushButton *button)
    {
        auto *layout = new QHBoxLayout(field);
        layout->setContentsMargins(0, 0, 10, 0);
        layout->addWidget(button, 0, Qt::AlignRight | Qt::AlignVCenter);
        button->setIcon(QIcon(":/icons/search.svg"));
    }
}

CustomizationPreview::CustomizationPreview(QWidget *parent)
    : QWidget(parent)
    , m_hoverAnimation(this)
    , m_focusAnimation(this)
{
    setObjectName("customizationPreview");
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(0, 240);
    setAccessibleName(tr("Предпросмотр интерфейса по слоям"));

    m_hoverAnimation.setDuration(180);
    m_hoverAnimation.setStartValue(0.0);
    m_hoverAnimation.setEndValue(1.0);
    m_hoverAnimation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_hoverAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const qreal progress = value.toReal();
        for (size_t i = 0; i < m_layers.size(); ++i) {
            auto &layer = m_layers[i];
            const qreal target = static_cast<int>(i) == m_hoveredLayer ? 1.0 : 0.0;
            layer.hover = layer.startHover + (target - layer.startHover) * progress;
        }
        update();
    });

    m_focusAnimation.setDuration(320);
    m_focusAnimation.setEasingCurve(QEasingCurve::InOutCubic);
    connect(&m_focusAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusProgress = value.toReal();
        update();
    });
    connect(&m_focusAnimation, &QVariantAnimation::finished, this, [this]() {
        if (!m_editing)
            m_selectedLayer = -1;
    });
}

void CustomizationPreview::setPreview(Screen screen, const QSize &windowSize, const QPixmap &avatar,
                                      QAbstractItemModel *chatsModel, const QString &styleSheet,
                                      bool preserveSelection)
{
    const QString selected = preserveSelection ? selectedObjectName() : QString();
    qreal selectedDetail = 1.0;
    if (!selected.isEmpty()) {
        const QSizeF size = m_layers[m_selectedLayer].geometry.size();
        const QRectF available = focusArea();
        selectedDetail = qMax(1.0, qMin(4.0, qMin(available.width() / size.width(),
                                                  available.height() / size.height())));
    }
    if (!preserveSelection)
        clearSelection(false);
    m_hoverAnimation.stop();
    m_hoveredLayer = -1;
    m_layers.clear();
    unsetCursor();
    emit hoveredElementChanged(QString(), QString());

    // Используем ту же .ui и QSS, но без MainWindow, контроллеров и авторизации.
    // Рабочий чат, его геометрия, фокус и черновик остаются в исходном окне.
    QMainWindow previewWindow;
    Ui::MainWindow previewUi;
    previewUi.setupUi(&previewWindow);
    previewWindow.setAttribute(Qt::WA_DontShowOnScreen);
    previewWindow.setProperty("themeScreen", screenName(screen));
    previewWindow.setStyleSheet(styleSheet.isEmpty() ? StyleHelper::load(":/styles/main.qss") : styleSheet);
    auto *titleBar = new CustomTitleBar(&previewWindow);
    titleBar->setObjectName("titleBar");
    previewWindow.setMenuWidget(titleBar);

    switch (screen) {
    case Screen::Messenger:
        previewUi.windowPages->setCurrentWidget(previewUi.contentPage);
        previewUi.authAndAppWidgets->setCurrentWidget(previewUi.pageApp);
        previewUi.narrowLeftPanel->setFixedWidth(280);
        previewUi.topPanelLayout->setAlignment(Qt::AlignLeft);
        previewUi.currentUserAvatar->setPixmap(avatar.isNull()
            ? AvatarHelper::generatePlaceholder(tr("Вы"), 50) : avatar);
        previewUi.messageInput->setMaximumWidth(320);
        previewUi.messageInput->setFixedHeight(40);
        previewUi.messageInput->setPlaceholderText(tr("Сообщение..."));
        previewUi.attachFileBtn->setIcon(QIcon(":/icons/attach.svg"));
        previewUi.sendMessageBtn->setIcon(QIcon(":/icons/send.svg"));
        previewUi.attachFileBtn->setIconSize(QSize(24, 24));
        previewUi.sendMessageBtn->setIconSize(QSize(24, 24));
        previewUi.horizontalSpacerInputLeft->changeSize(24, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        previewUi.horizontalSpacerInputRight->changeSize(24, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        previewUi.chatsView->setModel(chatsModel && chatsModel->rowCount() > 0
            ? chatsModel : new ChatPreviewModel(previewUi.chatsView));
        previewUi.chatsView->setItemDelegate(new ChatListItemDelegate(previewUi.chatsView));
        previewUi.chatsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        previewUi.chatsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        break;
    case Screen::Loading:
        previewUi.windowPages->setCurrentWidget(previewUi.loadingPage);
        break;
    case Screen::Registration:
        previewUi.windowPages->setCurrentWidget(previewUi.contentPage);
        previewUi.authAndAppWidgets->setCurrentWidget(previewUi.pageAuth);
        previewUi.registrationAndLogInWidgets->setCurrentWidget(previewUi.pageRegistration);
        setupRevealButton(previewUi.registrationPassword, previewUi.revealRegistrationPasswordBtn);
        setupRevealButton(previewUi.registrationPasswordConfirm, previewUi.revealRegistrationPasswordConfirmBtn);
        break;
    case Screen::Login:
        previewUi.windowPages->setCurrentWidget(previewUi.contentPage);
        previewUi.authAndAppWidgets->setCurrentWidget(previewUi.pageAuth);
        previewUi.registrationAndLogInWidgets->setCurrentWidget(previewUi.pageLogIn);
        setupRevealButton(previewUi.logInPassword, previewUi.revealLogInPasswordBtn);
        break;
    }

    m_windowSize = QSize(qBound(1000, windowSize.width(), 1440),
                         qBound(680, windowSize.height(), 960));
    previewWindow.resize(m_windowSize);
    // render() подготавливает компоновки скрытой формы без показа и перехвата фокуса.
    previewWindow.ensurePolished();
    const QPixmap preparedForm = snapshot(&previewWindow, devicePixelRatioF(), true);
    Q_UNUSED(preparedForm)

    const auto add = [this, &previewWindow, &selected, selectedDetail](QWidget *widget, const QString &title,
                                           qreal depth, qreal radius, bool withChildren,
                                           const QPointF &offset = QPointF()) {
        const QRectF geometry = QRectF(widget->mapTo(&previewWindow, QPoint()), widget->size()).translated(offset);
        const bool detailed = widget->objectName() == selected
            || (selected == "chatListItem" && widget->objectName() == "chatsView");
        m_layers.push_back({title, widget->objectName(), geometry,
                           snapshot(widget, devicePixelRatioF() * (detailed ? selectedDetail : 1.0), withChildren), depth, radius});
        m_layers.back().background = widget->palette().color(QPalette::Window);
        m_layers.back().text = widget->palette().color(QPalette::WindowText);
        m_layers.back().border = widget->palette().color(QPalette::Mid);
        return m_layers.size() - 1;
    };

    // Порядок совпадает с глубиной: контейнеры снимаются без дочерних элементов.
    add(&previewWindow, tr("Фон окна"), 0.0, 10.0, false);
    add(titleBar, tr("Заголовок окна"), 0.0, 4.0, true,
        QPointF(0.0, -titleBar->height() - 32.0));
    switch (screen) {
    case Screen::Messenger: {
        add(previewUi.narrowLeftPanel, tr("Боковая панель"), panelDepth, 25.0, false);
        add(previewUi.messagesView, tr("Фон сообщений"), panelDepth, 12.0, false);
        add(previewUi.messageInput, tr("Поле сообщения"), controlDepth, 20.0, true);
        add(previewUi.sendMessageBtn, tr("Кнопка отправки"), controlDepth, 20.0, true);
        add(previewUi.attachFileBtn, tr("Прикрепление файла"), controlDepth, 20.0, true);
        add(previewUi.appIcon, tr("Иконка приложения"), controlDepth, 10.0, true);
        add(previewUi.searchInput, tr("Поле поиска"), controlDepth, 17.0, true);
        const auto chatsLayerIndex = add(previewUi.chatsView, tr("Список чатов"), controlDepth, 10.0, true);
        add(previewUi.currentUserAvatar, tr("Ваш аватар"), controlDepth, 25.0, true);
        add(previewUi.settingsBtn, tr("Кнопка настроек"), controlDepth, 12.0, true);

        // Отделяем первую строку от текстуры списка, сохраняя геометрию остальных строк.
        auto *view = previewUi.chatsView;
        const QRect rowRect = view->visualRect(view->model()->index(0, 0)).intersected(view->viewport()->rect());
        if (!rowRect.isEmpty()) {
            auto &listPixmap = m_layers[chatsLayerIndex].pixmap;
            const qreal ratio = listPixmap.devicePixelRatioF();
            const QRect localRect(view->viewport()->mapTo(view, rowRect.topLeft()), rowRect.size());
            const QRect pixelRect = QRectF(localRect.x() * ratio, localRect.y() * ratio,
                                           localRect.width() * ratio, localRect.height() * ratio).toAlignedRect();
            QPixmap rowPixmap = listPixmap.copy(pixelRect);
            rowPixmap.setDevicePixelRatio(ratio);
            {
                QPainter painter(&listPixmap);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.fillRect(localRect, Qt::transparent);
            }
            // Оставляем поиск и иконку приложения открытыми за выдвинутой строкой.
            const QRectF geometry = QRectF(view->viewport()->mapTo(&previewWindow, rowRect.topLeft()),
                                            rowRect.size()).translated(0.0, 24.0);
            m_layers.push_back({tr("Элемент чата"), QStringLiteral("chatListItem"), geometry,
                               rowPixmap, chatItemDepth, 10.0});
            if (auto *style = view->findChild<QWidget *>("chatListItem")) {
                m_layers.back().background = style->palette().color(QPalette::Window);
                m_layers.back().text = style->palette().color(QPalette::WindowText);
                m_layers.back().border = style->palette().color(QPalette::Mid);
            }
        }
        break;
    }
    case Screen::Loading:
        add(previewUi.loadingLabel, tr("Логотип загрузки"), controlDepth, 10.0, true);
        break;
    case Screen::Registration:
        add(previewUi.registrationWidget, tr("Панель регистрации"), panelDepth, 12.0, false);
        add(previewUi.registrationLogin, tr("Логин"), controlDepth, 15.0, true);
        add(previewUi.registrationPassword, tr("Пароль"), controlDepth, 15.0, false);
        add(previewUi.registrationPasswordConfirm, tr("Подтверждение пароля"), controlDepth, 15.0, false);
        add(previewUi.registrationBtn, tr("Зарегистрироваться"), controlDepth, 15.0, true);
        add(previewUi.switchToLogInBtn, tr("Переход ко входу"), controlDepth, 15.0, true);
        add(previewUi.revealRegistrationPasswordBtn, tr("Показать пароль"), controlDepth, 6.0, true);
        add(previewUi.revealRegistrationPasswordConfirmBtn, tr("Показать повтор пароля"), controlDepth, 6.0, true);
        break;
    case Screen::Login:
        add(previewUi.logInWidget, tr("Панель входа"), panelDepth, 12.0, false);
        add(previewUi.logInLogIn, tr("Логин"), controlDepth, 15.0, true);
        add(previewUi.logInPassword, tr("Пароль"), controlDepth, 15.0, false);
        add(previewUi.logInBtn, tr("Войти"), controlDepth, 15.0, true);
        add(previewUi.switchToRegistrationBtn, tr("Переход к регистрации"), controlDepth, 15.0, true);
        add(previewUi.revealLogInPasswordBtn, tr("Показать пароль"), controlDepth, 6.0, true);
        break;
    }

    if (preserveSelection && !selected.isEmpty()) {
        m_selectedLayer = -1;
        for (int i = 0; i < static_cast<int>(m_layers.size()); ++i)
            if (m_layers[i].objectName == selected)
                m_selectedLayer = i;
        if (m_selectedLayer < 0)
            clearSelection(false);
    }
    fitPreview();
    update();
}

QString CustomizationPreview::screenName(Screen screen)
{
    switch (screen) {
    case Screen::Messenger: return QStringLiteral("messenger");
    case Screen::Loading: return QStringLiteral("loading");
    case Screen::Registration: return QStringLiteral("registration");
    case Screen::Login: return QStringLiteral("login");
    }
    return {};
}

QString CustomizationPreview::selectedObjectName() const
{
    return m_selectedLayer >= 0 ? m_layers[m_selectedLayer].objectName : QString();
}

QColor CustomizationPreview::selectedColor(const QString &property) const
{
    if (m_selectedLayer < 0)
        return Qt::transparent;
    const auto &layer = m_layers[m_selectedLayer];
    const QColor value = property == "text" ? layer.text : (property == "border" ? layer.border : layer.background);
    return value.isValid() ? value : QColor(Qt::transparent);
}

bool CustomizationPreview::selectElement(const QString &objectName)
{
    if (m_selectedLayer >= 0)
        return false;
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        if (m_layers[i].objectName != objectName)
            continue;
        m_hoverAnimation.stop();
        for (auto &layer : m_layers)
            layer.hover = layer.startHover = 0.0;
        m_hoveredLayer = -1;
        m_selectedLayer = i;
        m_editing = true;
        m_focusAnimation.stop();
        m_focusAnimation.setStartValue(m_focusProgress);
        m_focusAnimation.setEndValue(1.0);
        m_focusAnimation.start();
        unsetCursor();
        emit selectionChanged(m_layers[i].title, objectName);
        return true;
    }
    return false;
}

void CustomizationPreview::clearSelection(bool animated)
{
    const bool wasSelected = m_selectedLayer >= 0 || m_editing;
    m_focusAnimation.stop();
    m_editing = false;
    if (animated && m_selectedLayer >= 0) {
        m_focusAnimation.setStartValue(m_focusProgress);
        m_focusAnimation.setEndValue(0.0);
        m_focusAnimation.start();
    } else {
        m_selectedLayer = -1;
        m_focusProgress = 0.0;
    }
    if (wasSelected)
        emit selectionChanged(QString(), QString());
    update();
}

QPointF CustomizationPreview::project(const QPointF &point, qreal depth) const
{
    QMatrix4x4 rotation;
    rotation.rotate(12.0f, 1.0f, 0.0f, 0.0f);
    rotation.rotate(-28.0f, 0.0f, 1.0f, 0.0f);
    const QVector3D position = rotation.map(QVector3D(
        point.x() - m_windowSize.width() / 2.0,
        point.y() - m_windowSize.height() / 2.0, depth));
    const qreal perspective = 2200.0 / (2200.0 - position.z());
    return QPointF(position.x(), position.y()) * perspective;
}

QPolygonF CustomizationPreview::polygon(const Layer &layer, qreal hover) const
{
    const QRectF rect = layer.geometry.adjusted(-hover * 3.0, -hover * 3.0,
                                               hover * 3.0, hover * 3.0);
    const qreal depth = layer.depth + hover * hoverDepth;
    QPolygonF result;
    for (const QPointF &corner : {rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft()})
        result << project(corner, depth) * m_scale + m_offset;
    return result;
}

QRectF CustomizationPreview::focusArea() const
{
    const qreal horizontalMargin = width() < 600 ? 16.0 : 52.0;
    return QRectF(rect()).adjusted(horizontalMargin, 42, -horizontalMargin, -42);
}

QPolygonF CustomizationPreview::displayPolygon(int index) const
{
    QPolygonF result = polygon(m_layers[index], m_layers[index].hover);
    if (index != m_selectedLayer || m_focusProgress <= 0.0)
        return result;

    const QRectF available = focusArea();
    const QSizeF size = m_layers[index].geometry.size();
    const qreal scale = qMin(4.0, qMin(available.width() / size.width(), available.height() / size.height()));
    QRectF target(QPointF(), size * scale);
    target.moveCenter(available.center());
    const QPolygonF flat({target.topLeft(), target.topRight(), target.bottomRight(), target.bottomLeft()});
    for (int i = 0; i < result.size(); ++i)
        result[i] = result[i] * (1.0 - m_focusProgress) + flat[i] * m_focusProgress;
    return result;
}

void CustomizationPreview::fitPreview()
{
    if (m_layers.empty())
        return;

    m_scale = 1.0;
    m_offset = QPointF();
    QRectF bounds;
    for (const auto &layer : m_layers)
        bounds = bounds.united(polygon(layer, 1.0).boundingRect())
                       .united(polygon(layer, 0.0).boundingRect());

    const QRectF available = QRectF(rect()).adjusted(38, 32, -38, -40);
    m_scale = qMax(0.01, qMin(available.width() / bounds.width(), available.height() / bounds.height()));
    m_offset = available.center() - bounds.center() * m_scale;
}

void CustomizationPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QStyleOption option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    painter.setPen(QColor(34, 42, 53));
    for (int x = 22; x < width() - 16; x += 28)
        for (int y = 22; y < height() - 16; y += 28)
            painter.drawPoint(x, y);

    // Тонкие направляющие показывают, откуда отделился каждый слой.
    painter.save();
    painter.setOpacity(1.0 - m_focusProgress);
    painter.setPen(QPen(QColor(58, 74, 94, 120), 1.0, Qt::DashLine));
    for (const auto &layer : m_layers) {
        if (layer.depth == panelDepth) {
            const QPointF origin = project(layer.geometry.topRight(), 0.0) * m_scale + m_offset;
            painter.drawLine(origin, polygon(layer, layer.hover).at(1));
        }
    }
    painter.restore();

    const auto drawLayer = [&](int index) {
        const auto &layer = m_layers[index];
        const QPolygonF target = displayPolygon(index);
        const QRectF localRect(QPointF(), layer.geometry.size());
        QPolygonF source;
        source << localRect.topLeft() << localRect.topRight() << localRect.bottomRight() << localRect.bottomLeft();
        QTransform transform;
        if (!QTransform::quadToQuad(source, target, transform))
            return;

        painter.save();
        const bool selected = index == m_selectedLayer;
        painter.setOpacity(selected ? 1.0 : 1.0 - m_focusProgress);

        QPainterPath surface;
        surface.addRoundedRect(localRect, layer.radius, layer.radius);
        const QPainterPath projected = transform.map(surface);
        painter.fillPath(projected.translated(3.0, 7.0), QColor(0, 0, 0, 85));
        painter.fillPath(projected.translated(0.0, 3.0), QColor(6, 8, 11));

        painter.save();
        painter.setTransform(transform, true);
        painter.setClipPath(surface);
        // Прозрачные QListView тоже получают отдельную видимую плоскость.
        painter.fillPath(surface, QColor(16, 19, 24, selected ? qRound(255 * (1.0 - m_focusProgress)) : 255));
        painter.drawPixmap(localRect, layer.pixmap, QRectF(layer.pixmap.rect()));
        painter.restore();

        QColor outline(64, 78, 98);
        const qreal highlight = selected ? 1.0 - m_focusProgress : layer.hover;
        if (highlight > 0.0) {
            outline.setRedF(outline.redF() + (0.42 - outline.redF()) * highlight);
            outline.setGreenF(outline.greenF() + (0.66 - outline.greenF()) * highlight);
            outline.setBlueF(outline.blueF() + (1.0 - outline.blueF()) * highlight);
            painter.fillPath(projected, QColor(82, 148, 255, qRound(18.0 * highlight)));
        }
        painter.setPen(QPen(outline, 1.0 + highlight * 1.2));
        painter.drawPath(projected);
        painter.restore();
    };
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i)
        if (i != m_selectedLayer)
            drawLayer(i);
    if (m_selectedLayer >= 0)
        drawLayer(m_selectedLayer);
}

int CustomizationPreview::layerAt(const QPointF &point) const
{
    // Считаем попадание по исходной и выдвинутой границе: элемент не теряет
    // наведение из-за собственной анимации. Верхние слои имеют приоритет.
    for (int i = static_cast<int>(m_layers.size()) - 1; i >= 0; --i) {
        const auto &layer = m_layers[i];
        if (polygon(layer, 0.0).containsPoint(point, Qt::OddEvenFill)
            || polygon(layer, layer.hover).containsPoint(point, Qt::OddEvenFill)) {
            return i;
        }
    }
    return -1;
}

void CustomizationPreview::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selectedLayer < 0)
        setHoveredLayer(layerAt(event->position()));
}

void CustomizationPreview::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selectedLayer < 0) {
        const int index = layerAt(event->position());
        if (index >= 0 && selectElement(m_layers[index].objectName)) {
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void CustomizationPreview::setHoveredLayer(int index)
{
    if (m_hoveredLayer == index)
        return;

    m_hoverAnimation.stop();
    for (auto &layer : m_layers)
        layer.startHover = layer.hover;
    m_hoveredLayer = index;
    if (index >= 0) {
        setCursor(Qt::PointingHandCursor);
        const auto &layer = m_layers[index];
        emit hoveredElementChanged(layer.title, layer.objectName);
    } else {
        unsetCursor();
        emit hoveredElementChanged(QString(), QString());
    }
    m_hoverAnimation.start();
}

void CustomizationPreview::leaveEvent(QEvent *event)
{
    setHoveredLayer(-1);
    QWidget::leaveEvent(event);
}

void CustomizationPreview::hideEvent(QHideEvent *event)
{
    clearSelection(false);
    m_hoverAnimation.stop();
    m_hoveredLayer = -1;
    for (auto &layer : m_layers)
        layer.hover = layer.startHover = 0.0;
    unsetCursor();
    emit hoveredElementChanged(QString(), QString());
    QWidget::hideEvent(event);
}

void CustomizationPreview::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    setHoveredLayer(-1);
    fitPreview();
    update();
}
