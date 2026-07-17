#include "delegates/chatlistitemdelegate.h"
#include "models/chatlistmodel.h"

#include <algorithm>
#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include "utils/avatarhelper.h"

ChatListItemDelegate::ChatListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void ChatListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString title = index.data(ChatListModel::ChatNameRole).toString().trimmed();
    const QString subtitle = index.data(ChatListModel::LastMessageRole).toString().trimmed();
    const QString timestamp = index.data(ChatListModel::LastMessageTimestampRole).toString().trimmed();

    painter->save();

    QFontMetrics fm(opt.font);
    const int baseHeight = fm.height();
    bool isCollapsed = opt.rect.width() < baseHeight * 6;
    const int avatarSize = qMax(40, static_cast<int>(baseHeight * 2.5));
    
    const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
    const QRect avatarRect = QRect(contentRect.left(), contentRect.top() + (contentRect.height() - avatarSize) / 2, avatarSize, avatarSize);

    bool isHovered = (opt.state & QStyle::State_MouseOver);
    bool isSelected = (opt.state & QStyle::State_Selected);

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (isHovered || isSelected) {
        painter->setPen(QPen(QColor(255, 255, 255, 30), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(avatarRect.adjusted(-3, -3, 3, 3));
    }

    QFont titleFont = opt.font;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);

    QFont subtitleFont = opt.font;
    subtitleFont.setBold(false);

    const QColor titleColor = (opt.state & QStyle::State_Selected)
        ? opt.palette.color(QPalette::HighlightedText)
        : opt.palette.color(QPalette::Text);

    const QColor subtitleColor = (opt.state & QStyle::State_Selected)
        ? opt.palette.color(QPalette::HighlightedText)
        : QColor(95, 95, 95);

    const QColor avatarColor = AvatarHelper::getColdColor(title);

    const QColor avatarTextColor = QColor(255, 255, 255);

    QFont avatarFont = opt.font;
    avatarFont.setBold(true);
    avatarFont.setPointSize(avatarFont.pointSize() + 1);

    QChar avatarLetter = QChar('#');
    if (!title.isEmpty())
        avatarLetter = title.at(0).toUpper();

    painter->setRenderHint(QPainter::Antialiasing, true);

    QVariant avatarPixmapVar = index.data(ChatListModel::AvatarPixmapRole);
    if (avatarPixmapVar.isValid() && !avatarPixmapVar.value<QPixmap>().isNull()) {
        QPixmap avatarPix = avatarPixmapVar.value<QPixmap>();
        QPixmap roundedPix = AvatarHelper::makeRoundImage(avatarPix, avatarSize);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(avatarRect, roundedPix);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(avatarColor);
        painter->drawEllipse(avatarRect);

        painter->setFont(avatarFont);
        painter->setPen(avatarTextColor);
        painter->drawText(avatarRect, Qt::AlignCenter, avatarLetter);
    }

    if (!isCollapsed) {
        const int unreadCount = index.data(ChatListModel::UnreadCountRole).toInt();

        const QRect textRect = contentRect.adjusted(avatarSize + 10, 0, 0, 0);
        const int timestampWidth = 50;
        const QRect timestampRect(textRect.right() - timestampWidth, avatarRect.top(), timestampWidth, avatarRect.height() / 2 + 5);
        const QRect titleRect(textRect.left(), avatarRect.top(), textRect.width() - timestampWidth - 6, avatarRect.height() / 2 + 5);
        
        int subtitleRightPadding = 0;
        if (unreadCount > 0) {
            subtitleRightPadding = 30;
        }
        const QRect subtitleRect(textRect.left(), titleRect.bottom(), textRect.width() - subtitleRightPadding, textRect.bottom() - titleRect.bottom());

        painter->setFont(titleFont);
        painter->setPen(titleColor);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop,
                          QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

        painter->setFont(subtitleFont);
        painter->setPen(subtitleColor);
        painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignTop,
                          QFontMetrics(subtitleFont).elidedText(subtitle, Qt::ElideRight, subtitleRect.width()));

        QFont timeFont = subtitleFont;
        timeFont.setPointSize(std::max(7, timeFont.pointSize() - 1));
        painter->setFont(timeFont);
        painter->setPen(subtitleColor);
        QString timeDisplay;
        if (!timestamp.isEmpty())
        {
            QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODateWithMs);
            if (!dt.isValid())
                dt = QDateTime::fromString(timestamp, Qt::ISODate);

            if (dt.isValid())
                timeDisplay = dt.toLocalTime().toString("HH:mm");
        }
        painter->drawText(timestampRect, Qt::AlignRight | Qt::AlignTop,
                          QFontMetrics(timeFont).elidedText(timeDisplay, Qt::ElideLeft, timestampRect.width()));

        if (unreadCount > 0) {
            QString badgeText = QString::number(unreadCount);
            if (unreadCount > 99) {
                badgeText = "99+";
            }
            
            QFont badgeFont = subtitleFont;
            badgeFont.setPointSize(std::max(7, badgeFont.pointSize() - 1));
            badgeFont.setBold(true);
            QFontMetrics badgeFm(badgeFont);
            
            int textWidth = badgeFm.horizontalAdvance(badgeText);
            int badgeHeight = 20;
            int badgeSize = std::max(badgeHeight, textWidth + 10);
            
            // Выравнивае круга по правому краю текста
            QRect badgeRect(textRect.right() - badgeSize, avatarRect.bottom() - badgeHeight, badgeSize, badgeHeight);
            
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
    QFontMetrics fm(option.font);
    const int baseHeight = fm.height();
    const int avatarSize = qMax(40, static_cast<int>(baseHeight * 2.5));
    const int rowHeight = avatarSize + qMax(10, static_cast<int>(baseHeight * 0.8));
    return QSize(0, rowHeight);
}
