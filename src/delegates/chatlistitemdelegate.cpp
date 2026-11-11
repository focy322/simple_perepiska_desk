#include "delegates/chatlistitemdelegate.h"
#include "models/chatlistmodel.h"
#include "utils/avatarhelper.h"
#include "utils/parsed_objects.h"

#include <algorithm>
#include <QAbstractItemView>
#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>
#include <QLabel>
#include <QStyle>
#include <QStyleOption>

int ChatListItemDelegate::avatarSizeForFont(const QFont &font)
{
    return qMax(40, static_cast<int>(QFontMetrics(font).height() * 2.5));
}

QString ChatListItemDelegate::messagePreviewText(const MessagePreview &message)
{
    const QString text = message.message.simplified();
    if (!text.isEmpty() || (!message.hasAttachments && message.attachmentsCount == 0))
        return text;

    const QString category = message.category.trimmed();
    QString label;
    if (category.compare("image", Qt::CaseInsensitive) == 0)
        label = tr("Фото");
    else if (category.compare("video", Qt::CaseInsensitive) == 0)
        label = tr("Видео");
    else if (category.compare("audio", Qt::CaseInsensitive) == 0)
        label = tr("Аудио");
    else if (category.compare("document", Qt::CaseInsensitive) == 0)
        label = tr("Файл");
    else
        label = tr("Вложение");

    return message.attachmentsCount > 1
        ? tr("%1 (%2)").arg(label).arg(message.attachmentsCount)
        : label;
}

QString ChatListItemDelegate::messageTimeText(const QString &timestamp)
{
    const QString value = timestamp.trimmed();
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, Qt::ISODate);

    return dateTime.isValid() ? dateTime.toLocalTime().toString("HH:mm") : QString();
}

void ChatListItemDelegate::drawAvatar(QPainter *painter, const QRect &rect, const QString &title,
                                    const QFont &font, const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(rect, AvatarHelper::makeRoundImage(pixmap, rect.width()));
        return;
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(AvatarHelper::getColdColor(title));
    painter->drawEllipse(rect);

    QFont avatarFont = font;
    avatarFont.setBold(true);
    avatarFont.setPointSize(avatarFont.pointSize() + 1);

    painter->setFont(avatarFont);
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter, title.isEmpty() ? QChar('#') : title.at(0).toUpper());
}

ChatListItemDelegate::ChatListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_animationTimer(new QTimer(this))
    , m_styleWidget(new QWidget(qobject_cast<QWidget *>(parent)))
    , m_subtitleStyleWidget(new QLabel(qobject_cast<QWidget *>(parent)))
{
    m_styleWidget->setObjectName("chatListItem");
    m_subtitleStyleWidget->setObjectName("chatListItemSubtitle");
    m_styleWidget->hide();
    m_subtitleStyleWidget->hide();
    m_animationTimer->setInterval(16);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        bool anyAnimating = false;
        for (auto it = m_avatarOpacities.begin(); it != m_avatarOpacities.end(); ++it) {
            const qreal target = m_targetOpacities.value(it.key(), 0.0);
            const qreal current = it.value();

            if (qAbs(current - target) > 1.0) {
                anyAnimating = true;
                it.value() = current < target
                    ? qMin(target, current + 10.0)
                    : qMax(target, current - 10.0);
            } else {
                it.value() = target;
            }
        }

        if (auto view = qobject_cast<QAbstractItemView*>(this->parent()))
            view->viewport()->update();

        if (!anyAnimating)
            m_animationTimer->stop();
    });
}

ChatListItemDelegate::~ChatListItemDelegate()
{
    delete m_styleWidget.data();
    delete m_subtitleStyleWidget.data();
}

void ChatListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString title = index.data(ChatListModel::ChatNameRole).toString().trimmed();
    const MessagePreview lastMessage = index.data(ChatListModel::LastMessageRole).value<MessagePreview>();

    painter->save();
    painter->setClipRect(opt.rect, Qt::IntersectClip);

    m_styleWidget->ensurePolished();
    m_subtitleStyleWidget->ensurePolished();
    QStyleOption background;
    background.initFrom(m_styleWidget);
    background.rect = QRect(QPoint(), opt.rect.size());
    painter->save();
    painter->translate(opt.rect.topLeft());
    m_styleWidget->style()->drawPrimitive(QStyle::PE_Widget, &background, painter, m_styleWidget);
    painter->restore();

    const QFontMetrics fm(opt.font);
    const int baseHeight = fm.height();
    const bool isCollapsed = opt.rect.width() < baseHeight * 6;
    const int avatarSize = avatarSizeForFont(opt.font);
    
    const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
    const QRect avatarRect = QRect(contentRect.left(), contentRect.top() + (contentRect.height() - avatarSize) / 2, avatarSize, avatarSize);

    const bool isHovered = (opt.state & QStyle::State_MouseOver);
    const bool isSelected = (opt.state & QStyle::State_Selected);

    painter->setRenderHint(QPainter::Antialiasing, true);

    const qulonglong chatId = index.data(ChatListModel::ChatIdRole).toULongLong();
    const qreal targetOpacity = isSelected ? 100.0 : (isHovered ? 40.0 : 0.0);
    m_targetOpacities[chatId] = targetOpacity;

    if (!m_avatarOpacities.contains(chatId)) {
        m_avatarOpacities[chatId] = 0.0;
    }
    const qreal currentOpacity = m_avatarOpacities[chatId];

    if (currentOpacity != targetOpacity && !m_animationTimer->isActive())
        m_animationTimer->start();

    if (currentOpacity > 0.0) {
        painter->setPen(QPen(QColor(255, 255, 255, static_cast<int>(currentOpacity)), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(avatarRect.adjusted(-3, -3, 3, 3));
    }

    QFont titleFont = opt.font;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);

    QFont subtitleFont = opt.font;
    subtitleFont.setBold(false);

    const QColor titleColor = isSelected
        ? opt.palette.color(QPalette::HighlightedText)
        : m_styleWidget->palette().color(QPalette::WindowText);

    const QColor subtitleColor = isSelected
        ? opt.palette.color(QPalette::HighlightedText)
        : m_subtitleStyleWidget->palette().color(QPalette::WindowText);

    drawAvatar(painter, avatarRect, title, opt.font,
               index.data(ChatListModel::AvatarPixmapRole).value<QPixmap>());

    if (!isCollapsed) {
        const QString subtitle = messagePreviewText(lastMessage);
        const QString timeDisplay = messageTimeText(lastMessage.timestamp);
        const uint unreadCount = index.data(ChatListModel::UnreadCountRole).toUInt();
        const QString badgeText = unreadCount > 99 ? QStringLiteral("99+") : QString::number(unreadCount);

        QFont timeFont = subtitleFont;
        timeFont.setPointSize(std::max(7, timeFont.pointSize() - 1));

        QFont badgeFont = timeFont;
        badgeFont.setBold(true);
        const QFontMetrics badgeFm(badgeFont);
        const int badgeHeight = std::max(20, badgeFm.height() + 2);
        const int badgeWidth = unreadCount > 0
            ? std::max(badgeHeight, badgeFm.horizontalAdvance(badgeText) + 10)
            : 0;

        const QRect textRect = contentRect.adjusted(avatarSize + 10, 0, 0, 0);
        const int timestampWidth = timeDisplay.isEmpty() ? 0 : QFontMetrics(timeFont).horizontalAdvance(timeDisplay);
        const QRect timestampRect(textRect.right() - timestampWidth + 1, avatarRect.top(), timestampWidth, avatarRect.height() / 2 + 5);
        const int titleRightPadding = timestampWidth > 0 ? timestampWidth + 6 : 0;
        const QRect titleRect(textRect.left(), avatarRect.top(), qMax(0, textRect.width() - titleRightPadding), avatarRect.height() / 2 + 5);
        
        const int subtitleRightPadding = badgeWidth > 0 ? badgeWidth + 6 : 0;
        const QRect subtitleRect(textRect.left(), titleRect.bottom(), qMax(0, textRect.width() - subtitleRightPadding), textRect.bottom() - titleRect.bottom());

        painter->setFont(titleFont);
        painter->setPen(titleColor);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop,
                          QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

        painter->setFont(subtitleFont);
        painter->setPen(subtitleColor);
        painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignTop,
                          QFontMetrics(subtitleFont).elidedText(subtitle, Qt::ElideRight, subtitleRect.width()));

        painter->setFont(timeFont);
        painter->setPen(subtitleColor);
        painter->drawText(timestampRect, Qt::AlignRight | Qt::AlignTop,
                          QFontMetrics(timeFont).elidedText(timeDisplay, Qt::ElideLeft, timestampRect.width()));

        if (unreadCount > 0) {
            const QRect badgeRect(textRect.right() - badgeWidth + 1, avatarRect.bottom() - badgeHeight + 1, badgeWidth, badgeHeight);
            
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(250, 249, 246)); // кремовый цвет
            painter->drawRoundedRect(badgeRect, badgeHeight / 2, badgeHeight / 2);
            
            painter->setPen(QColor(0, 0, 0)); // чёрный цвет
            painter->setFont(badgeFont);
            painter->drawText(badgeRect, Qt::AlignCenter, badgeText);
        }
    }

    painter->restore();
}

QSize ChatListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    const QFontMetrics fm(option.font);
    const int baseHeight = fm.height();
    const int avatarSize = avatarSizeForFont(option.font);
    const int rowHeight = avatarSize + qMax(10, static_cast<int>(baseHeight * 0.8));
    return QSize(0, rowHeight);
}
