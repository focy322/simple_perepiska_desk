#include "chatmessagesitemdelegate.h"
#include "chatmessageslistmodel.h"

#include <climits>
#include <QDateTime>
#include <QPainter>

ChatMessagesItemDelegate::ChatMessagesItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_currentUserId(ULONG_LONG_MAX)
{}

void ChatMessagesItemDelegate::setCurrentUserId(unsigned long long userId)
{
    m_currentUserId = userId;
}

void ChatMessagesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
    const QString timestampIso = index.data(ChatMessagesListModel::TimestampRole).toString().trimmed();
    const unsigned long long senderId = index.data(ChatMessagesListModel::SenderIdRole).toULongLong();
    const bool isMine = senderId == m_currentUserId;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rowRect = option.rect.adjusted(8, 4, -8, -4);
    const int maxBubbleWidth = static_cast<int>(rowRect.width() * 0.72);
    const int horizontalPadding = 12;
    const int verticalPadding = 8;

    QFont textFont = option.font;
    QFontMetrics textFm(textFont);
    const QString safeMessage = messageText.isEmpty() ? QString(" ") : messageText;
    const QRect textBounds = textFm.boundingRect(QRect(0, 0, maxBubbleWidth - horizontalPadding * 2, 10000),
                                                 Qt::TextWordWrap,
                                                 safeMessage);

    QString timeText;
    if (!timestampIso.isEmpty())
    {
        QDateTime dt = QDateTime::fromString(timestampIso, Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(timestampIso, Qt::ISODate);
        if (dt.isValid())
            timeText = dt.toLocalTime().toString("HH:mm");
    }

    QFont timeFont = option.font;
    timeFont.setPointSize(qMax(7, timeFont.pointSize() - 1));
    QFontMetrics timeFm(timeFont);
    const int timeHeight = timeText.isEmpty() ? 0 : timeFm.height();

    const int bubbleWidth = qMax(60, textBounds.width() + horizontalPadding * 2);
    const int bubbleHeight = textBounds.height() + verticalPadding * 2 + (timeHeight > 0 ? timeHeight + 4 : 0);

    int bubbleX = rowRect.left();
    if (isMine)
        bubbleX = rowRect.right() - bubbleWidth;

    const QRect bubbleRect(bubbleX, rowRect.top(), bubbleWidth, bubbleHeight);
    const QRect messageRect = bubbleRect.adjusted(horizontalPadding, verticalPadding, -horizontalPadding, -verticalPadding - (timeHeight > 0 ? timeHeight + 4 : 0));

    const QColor ownBubbleColor(232, 248, 217);
    const QColor otherBubbleColor(255, 255, 255);
    const QColor bubbleBorderColor(210, 210, 210);
    const QColor textColor(35, 35, 35);
    const QColor timeColor(120, 120, 120);

    painter->setPen(QPen(bubbleBorderColor, 1));
    painter->setBrush(isMine ? ownBubbleColor : otherBubbleColor);
    painter->drawRoundedRect(bubbleRect, 12, 12);

    painter->setFont(textFont);
    painter->setPen(textColor);
    painter->drawText(messageRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, safeMessage);

    if (!timeText.isEmpty())
    {
        const QRect timeRect = bubbleRect.adjusted(horizontalPadding, bubbleRect.height() - verticalPadding - timeHeight, -horizontalPadding, -verticalPadding);
        painter->setFont(timeFont);
        painter->setPen(timeColor);
        painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, timeText);
    }

    painter->restore();
}

QSize ChatMessagesItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
    const QString safeMessage = messageText.isEmpty() ? QString(" ") : messageText;

    const int maxBubbleWidth = 340;
    const int horizontalPadding = 12;
    const int verticalPadding = 8;

    QFontMetrics textFm(option.font);
    const QRect textBounds = textFm.boundingRect(QRect(0, 0, maxBubbleWidth - horizontalPadding * 2, 10000),
                                                 Qt::TextWordWrap,
                                                 safeMessage);

    QFont timeFont = option.font;
    timeFont.setPointSize(qMax(7, timeFont.pointSize() - 1));
    QFontMetrics timeFm(timeFont);

    const int rowHeight = textBounds.height() + verticalPadding * 2 + timeFm.height() + 10;
    return QSize(option.rect.width(), qMax(44, rowHeight));
}
