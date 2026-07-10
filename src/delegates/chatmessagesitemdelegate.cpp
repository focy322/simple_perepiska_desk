#include "delegates/chatmessagesitemdelegate.h"
#include "models/chatmessageslistmodel.h"

#include <climits>
#include <QDateTime>
#include <QPainter>
#include <QJsonArray>
#include <QJsonObject>

ChatMessagesItemDelegate::ChatMessagesItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_currentUserId(ULONG_LONG_MAX)
    , lastReadMessage{ULONG_LONG_MAX, ULONG_LONG_MAX}
{}

void ChatMessagesItemDelegate::setCurrentUserId(unsigned long long userId)
{
    m_currentUserId = userId;
}

void ChatMessagesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
    const QString timestampIso = index.data(ChatMessagesListModel::TimestampRole).toString().trimmed();
    const bool isPending = index.data(ChatMessagesListModel::IsPendingRole).toBool();
    const unsigned long long senderId = index.data(ChatMessagesListModel::SenderIdRole).toULongLong();
    const QJsonArray attachments = index.data(ChatMessagesListModel::AttachmentsRole).toJsonArray();
    const bool hasAttachments = !attachments.isEmpty();
    const bool isMine = senderId == m_currentUserId;
    const bool isRead = index.data(ChatMessagesListModel::ReadRole).toBool();
    const quint64 messageId = index.data(ChatMessagesListModel::MessageIdRole).toULongLong();
    const quint64 chatId = index.data(ChatMessagesListModel::ChatIdRole).toULongLong();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rowRect = option.rect.adjusted(8, 4, -8, -4);
    const int maxBubbleWidth = static_cast<int>(rowRect.width() * 0.72);
    const int horizontalPadding = 12;
    const int verticalPadding = 8;

    QFont textFont = option.font;
    QFontMetrics textFm(textFont);
    const bool hasMessage = !messageText.isEmpty();
    const QString safeMessage = hasMessage ? messageText : QString();

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
    const int statusIconSize = 12;
    const int statusSpacing = 4;
    const bool showPending = isPending;
    const int statusHeight = qMax(timeHeight, statusIconSize) + statusSpacing;

    QFont attachmentsFont = option.font;
    attachmentsFont.setPointSize(qMax(8, attachmentsFont.pointSize() - 1));
    QFontMetrics attachmentsFm(attachmentsFont);
    const int attachmentIconSize = 18;
    const int attachmentSpacing = 6;
    const int attachmentsRowSpacing = 6;
    const int attachmentsBlockSpacing = 6;

    int attachmentsHeight = 0;
    int attachmentsMaxWidth = 0;
    if (hasAttachments)
    {
        for (int i = 0; i < attachments.size(); ++i)
        {
            const QJsonObject obj = attachments.at(i).toObject();
            const QString fileName = obj.value("filename").toString(QString("file"));
            const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());
            attachmentsHeight += rowHeight;
            if (i < attachments.size() - 1)
                attachmentsHeight += attachmentsRowSpacing;

            const int rowWidth = attachmentIconSize + attachmentSpacing + attachmentsFm.horizontalAdvance(fileName);
            attachmentsMaxWidth = qMax(attachmentsMaxWidth, rowWidth);
        }
    }

    const int maxContentWidth = maxBubbleWidth - horizontalPadding * 2;
    const int naturalTextWidth = hasMessage ? textFm.horizontalAdvance(safeMessage) : 0;
    const int contentWidth = qMin(maxContentWidth, qMax(attachmentsMaxWidth, naturalTextWidth));
    const QRect textBounds = hasMessage
        ? textFm.boundingRect(QRect(0, 0, qMax(1, contentWidth), 10000),
                              Qt::TextWordWrap,
                              safeMessage)
        : QRect(0, 0, 0, 0);
    const int bubbleWidth = qMax(60, contentWidth + horizontalPadding * 2);
    const int bubbleHeight = textBounds.height() + verticalPadding * 2 + statusHeight
        + (hasAttachments ? attachmentsHeight : 0)
        + (hasAttachments && hasMessage ? attachmentsBlockSpacing : 0);

    int bubbleX = rowRect.left();
    if (isMine)
        bubbleX = rowRect.right() - bubbleWidth;

    const QRect bubbleRect(bubbleX, rowRect.top(), bubbleWidth, bubbleHeight);
    const int contentLeft = bubbleRect.left() + horizontalPadding;
    int contentTop = bubbleRect.top() + verticalPadding;
    const int contentRight = bubbleRect.right() - horizontalPadding;

    const QColor ownBubbleColor(232, 248, 217);
    const QColor otherBubbleColor(255, 255, 255);
    const QColor bubbleBorderColor(210, 210, 210);
    const QColor textColor(35, 35, 35);
    const QColor timeColor(120, 120, 120);

    painter->setPen(QPen(bubbleBorderColor, 1));
    painter->setBrush(isMine ? ownBubbleColor : otherBubbleColor);
    painter->drawRoundedRect(bubbleRect, 12, 12);

    if (hasAttachments)
    {
        painter->setFont(attachmentsFont);
        painter->setPen(textColor);

        for (int i = 0; i < attachments.size(); ++i)
        {
            const QJsonObject obj = attachments.at(i).toObject();
            const QString fileName = obj.value("filename").toString(QString("file"));
            const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());

            const QRect iconRect(contentLeft, contentTop, attachmentIconSize, attachmentIconSize);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(bubbleBorderColor, 1));
            painter->drawRoundedRect(iconRect, 3, 3);
            painter->drawLine(iconRect.topRight() + QPoint(-5, 2), iconRect.topRight() + QPoint(-1, 6));
            painter->drawLine(iconRect.topRight() + QPoint(-5, 2), iconRect.topRight() + QPoint(-1, 2));

            const int textLeft = iconRect.right() + attachmentSpacing;
            const QRect textRect(textLeft, contentTop,
                                 contentRight - textLeft,
                                 rowHeight);
            const QString elidedName = attachmentsFm.elidedText(fileName, Qt::ElideRight, textRect.width());
            painter->setPen(textColor);
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedName);

            contentTop += rowHeight;
            if (i < attachments.size() - 1)
                contentTop += attachmentsRowSpacing;
        }

        if (hasMessage)
            contentTop += attachmentsBlockSpacing;
    }

    if (hasMessage)
    {
        const QRect messageRect(contentLeft, contentTop, contentRight - contentLeft,
                                textBounds.height());
        painter->setFont(textFont);
        painter->setPen(textColor);
        painter->drawText(messageRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, safeMessage);
    }

    if (!timeText.isEmpty())
    {
        const int statusReserve = isMine ? statusIconSize + statusSpacing : 0;
        const QRect timeRect = bubbleRect.adjusted(horizontalPadding,
                                                   bubbleRect.height() - verticalPadding - timeHeight,
                                                   -(horizontalPadding + statusReserve),
                                                   -verticalPadding);
        painter->setFont(timeFont);
        painter->setPen(timeColor);
        painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, timeText);
    }

    if (isMine)
    {
        if (showPending)
        {
            const QRect iconRect(bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                                 bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                                 statusIconSize,
                                 statusIconSize);
            const QColor pendingColor(130, 130, 130);
            painter->setPen(QPen(pendingColor, 1));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(iconRect);

            const QPoint center = iconRect.center();
            painter->drawLine(center, QPoint(center.x(), center.y() - 3));
            painter->drawLine(center, QPoint(center.x() + 2, center.y()));
        }
        else if (isRead)
        {
            const QRect iconRect(bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                                 bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                                 statusIconSize,
                                 statusIconSize);
            const QColor readColor(0, 120, 215);
            painter->setPen(QPen(readColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);

            const QPoint p1a(iconRect.left() + 2, iconRect.top() + iconRect.height() / 2);
            const QPoint p2a(iconRect.left() + iconRect.width() / 2, iconRect.bottom() - 3);
            const QPoint p3a(iconRect.right() - 2, iconRect.top() + 3);
            painter->drawLine(p1a, p2a);
            painter->drawLine(p2a, p3a);

            const QPoint p1b(iconRect.left() + 6, iconRect.top() + iconRect.height() / 2);
            const QPoint p2b(iconRect.left() + iconRect.width() / 2 + 4, iconRect.bottom() - 3);
            const QPoint p3b(iconRect.right() + 2, iconRect.top() + 3);
            painter->drawLine(p1b, p2b);
            painter->drawLine(p2b, p3b);
        }
        else
        {
            const QRect iconRect(bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                                 bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                                 statusIconSize,
                                 statusIconSize);
            const QColor deliveredColor(0, 150, 0);
            painter->setPen(QPen(deliveredColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);

            const QPoint p1(iconRect.left() + 2, iconRect.top() + iconRect.height() / 2);
            const QPoint p2(iconRect.left() + iconRect.width() / 2, iconRect.bottom() - 3);
            const QPoint p3(iconRect.right() - 2, iconRect.top() + 3);
            painter->drawLine(p1, p2);
            painter->drawLine(p2, p3);
        }
    }
    else
    {
        if (!isRead)
            setLastReadMessage(chatId, messageId);
    }

    painter->restore();
}

QSize ChatMessagesItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
    const bool isPending = index.data(ChatMessagesListModel::IsPendingRole).toBool();
    const QJsonArray attachments = index.data(ChatMessagesListModel::AttachmentsRole).toJsonArray();
    const bool hasAttachments = !attachments.isEmpty();
    const bool hasMessage = !messageText.isEmpty();
    const QString safeMessage = hasMessage ? messageText : QString();

    const int maxBubbleWidth = 340;
    const int horizontalPadding = 12;
    const int verticalPadding = 8;

    QFontMetrics textFm(option.font);

    QFont attachmentsFont = option.font;
    attachmentsFont.setPointSize(qMax(8, attachmentsFont.pointSize() - 1));
    QFontMetrics attachmentsFm(attachmentsFont);
    const int attachmentIconSize = 18;
    const int attachmentSpacing = 6;
    const int attachmentsRowSpacing = 6;
    const int attachmentsBlockSpacing = 6;

    int attachmentsHeight = 0;
    int attachmentsMaxWidth = 0;
    if (hasAttachments)
    {
        for (int i = 0; i < attachments.size(); ++i)
        {
            const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());
            attachmentsHeight += rowHeight;
            if (i < attachments.size() - 1)
                attachmentsHeight += attachmentsRowSpacing;
            const QJsonObject obj = attachments.at(i).toObject();
            const QString fileName = obj.value("filename").toString(QString("file"));
            const int rowWidth = attachmentIconSize + attachmentSpacing + attachmentsFm.horizontalAdvance(fileName);
            attachmentsMaxWidth = qMax(attachmentsMaxWidth, rowWidth);
        }
    }

    const int maxContentWidth = maxBubbleWidth - horizontalPadding * 2;
    const int naturalTextWidth = hasMessage ? textFm.horizontalAdvance(safeMessage) : 0;
    const int contentWidth = qMin(maxContentWidth, qMax(attachmentsMaxWidth, naturalTextWidth));
    const QRect textBounds = hasMessage
        ? textFm.boundingRect(QRect(0, 0, qMax(1, contentWidth), 10000),
                              Qt::TextWordWrap,
                              safeMessage)
        : QRect(0, 0, 0, 0);

    QFont timeFont = option.font;
    timeFont.setPointSize(qMax(7, timeFont.pointSize() - 1));
    QFontMetrics timeFm(timeFont);
    const int pendingIconSize = 12;
    const int statusSpacing = 4;
    const int statusHeight = qMax(timeFm.height(), isPending ? pendingIconSize : 0) + statusSpacing;

    const int rowHeight = textBounds.height() + verticalPadding * 2 + statusHeight + 6
        + (hasAttachments ? attachmentsHeight : 0)
        + (hasAttachments && hasMessage ? attachmentsBlockSpacing : 0);
    return QSize(option.rect.width(), qMax(44, rowHeight));
}

void ChatMessagesItemDelegate::setLastReadMessage(const quint64 chatId, const quint64 MessageId) const
{
    lastReadMessage = {chatId, MessageId};
}
