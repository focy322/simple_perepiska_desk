#include "delegates/chatmessagesitemdelegate.h"
#include "models/chatmessageslistmodel.h"

#include <climits>
#include <QDateTime>
#include <QPainter>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <QImageReader>
#include <QFileInfo>
#include <QPixmap>
#include "utils/paths.h"
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
    const bool isEdited = index.data(ChatMessagesListModel::EditedRole).toBool();
    const quint64 messageId = index.data(ChatMessagesListModel::MessageIdRole).toULongLong();
    const quint64 chatId = index.data(ChatMessagesListModel::ChatIdRole).toULongLong();

    bool showDateBadge = false;
    QString dateBadgeText;
    if (!timestampIso.isEmpty()) {
        QDateTime currentDt = QDateTime::fromString(timestampIso, Qt::ISODateWithMs);
        if (!currentDt.isValid()) currentDt = QDateTime::fromString(timestampIso, Qt::ISODate);
        if (currentDt.isValid()) {
            QDateTime prevDt;
            if (index.row() > 0) {
                QString prevDateStr = index.model()->index(index.row() - 1, 0).data(ChatMessagesListModel::TimestampRole).toString().trimmed();
                prevDt = QDateTime::fromString(prevDateStr, Qt::ISODateWithMs);
                if (!prevDt.isValid()) prevDt = QDateTime::fromString(prevDateStr, Qt::ISODate);
            }
            if (index.row() == 0 || (prevDt.isValid() && currentDt.toLocalTime().date() != prevDt.toLocalTime().date())) {
                showDateBadge = true;
                QDate today = QDate::currentDate();
                QDate msgDate = currentDt.toLocalTime().date();
                if (msgDate == today) {
                    dateBadgeText = "Сегодня";
                } else if (msgDate == today.addDays(-1)) {
                    dateBadgeText = "Вчера";
                } else {
                    QStringList months = {"января", "февраля", "марта", "апреля", "мая", "июня", "июля", "августа", "сентября", "октября", "ноября", "декабря"};
                    if (msgDate.year() == today.year()) {
                        dateBadgeText = QString("%1 %2").arg(msgDate.day()).arg(months.at(msgDate.month() - 1));
                    } else {
                        dateBadgeText = QString("%1 %2 %3").arg(msgDate.day()).arg(months.at(msgDate.month() - 1)).arg(msgDate.year());
                    }
                }
            }
        }
    }
    const int dateBadgeHeight = showDateBadge ? 36 : 0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (showDateBadge) {
        QFont badgeFont = option.font;
        badgeFont.setPointSize(qMax(8, badgeFont.pointSize() - 1));
        QFontMetrics badgeFm(badgeFont);
        int textWidth = badgeFm.horizontalAdvance(dateBadgeText);
        int badgeWidth = textWidth + 30;
        int badgeHeight = 24;
        QRect badgeRect(option.rect.left() + (option.rect.width() - badgeWidth) / 2, option.rect.top() + 6, badgeWidth, badgeHeight);
        
        painter->setPen(QPen(QColor(51, 51, 51), 1));
        painter->setBrush(QColor(20, 20, 20));
        painter->drawRoundedRect(badgeRect, 12, 12);
        
        painter->setPen(QColor(230, 232, 235));
        painter->setFont(badgeFont);
        painter->drawText(badgeRect, Qt::AlignCenter, dateBadgeText);
    }

    const QRect rowRect = option.rect.adjusted(2, 4 + dateBadgeHeight, -8, -4);
    const int maxBubbleWidth = qMax(200, static_cast<int>(rowRect.width() * 0.72));

    QFont textFont = option.font;
    QFontMetrics textFm(textFont);
    const int baseHeight = textFm.height();
    const int horizontalPadding = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int verticalPadding = qMax(8, static_cast<int>(baseHeight * 0.5));

    const bool hasMessage = !messageText.isEmpty();
    const QString safeMessage = hasMessage ? messageText : QString();

    QString timeText;
    if (!timestampIso.isEmpty())
    {
        QDateTime dt = QDateTime::fromString(timestampIso, Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(timestampIso, Qt::ISODate);
        if (dt.isValid()) {
            timeText = dt.toLocalTime().toString("HH:mm");
            if (isEdited) {
                timeText = "Ред. " + timeText;
            }
        }
    }

    QFont timeFont = option.font;
    timeFont.setPointSize(qMax(7, timeFont.pointSize() - 1));
    QFontMetrics timeFm(timeFont);
    const int timeHeight = timeText.isEmpty() ? 0 : timeFm.height();
    const int statusIconSize = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int statusSpacing = 4;
    const bool showPending = isPending;
    const int statusHeight = qMax(timeHeight, statusIconSize) + statusSpacing;

    QFont attachmentsFont = option.font;
    attachmentsFont.setPointSize(qMax(8, attachmentsFont.pointSize() - 1));
    QFontMetrics attachmentsFm(attachmentsFont);
    const int attachmentIconSize = qMax(18, static_cast<int>(baseHeight * 1.2));
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
            bool isImage = fileName.endsWith(".png", Qt::CaseInsensitive) ||
                           fileName.endsWith(".jpg", Qt::CaseInsensitive) ||
                           fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                           fileName.endsWith(".bmp", Qt::CaseInsensitive) ||
                           fileName.endsWith(".gif", Qt::CaseInsensitive);

            if (isImage) {
                int imgWidth = maxBubbleWidth - horizontalPadding * 2;
                int imgHeight = 200;
                QString path = appDownloadsDir + "/" + fileName;
                QString localPath = obj.value("local_path").toString();
                if (!localPath.isEmpty() && QFileInfo::exists(localPath)) {
                    path = localPath;
                }
                if (QFileInfo::exists(path)) {
                    QImageReader reader(path);
                    QSize sz = reader.size();
                    if (sz.isValid()) {
                        QSize scaled = sz.scaled(imgWidth, 300, Qt::KeepAspectRatio);
                        imgWidth = scaled.width();
                        imgHeight = scaled.height();
                    }
                }
                attachmentsHeight += imgHeight;
                attachmentsMaxWidth = qMax(attachmentsMaxWidth, imgWidth);
            } else {
                const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());
                attachmentsHeight += rowHeight;
                int btnWidth = 30;
                const int rowWidth = attachmentIconSize + attachmentSpacing + attachmentsFm.horizontalAdvance(fileName) + attachmentSpacing + btnWidth;
                attachmentsMaxWidth = qMax(attachmentsMaxWidth, rowWidth);
            }

            if (i < attachments.size() - 1)
                attachmentsHeight += attachmentsRowSpacing;
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

    const int statusReserve = isMine ? 18 : 0;
    const int timeTextWidth = timeText.isEmpty() ? 0 : QFontMetrics(timeFont).horizontalAdvance(timeText);
    const int bottomWidth = timeTextWidth + statusReserve;
    const int trueContentWidth = qMax(attachmentsMaxWidth, qMax(hasMessage ? textBounds.width() : 0, bottomWidth));
    const int bubbleWidth = qMax(60, trueContentWidth + horizontalPadding * 2);
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

    const QColor ownBubbleColor(250, 249, 246); // #FAF9F6 Кремовый
    const QColor otherBubbleColor(20, 22, 25); // Ближе к черному
    const QColor ownBubbleBorderColor(235, 230, 220);
    const QColor otherBubbleBorderColor(40, 45, 50);
    const QColor ownTextColor(30, 30, 30); // Темный текст на кремовом фоне
    const QColor otherTextColor(230, 232, 235);
    const QColor ownTimeColor(120, 120, 120);
    const QColor otherTimeColor(140, 150, 160);

    const QColor currentTextColor = isMine ? ownTextColor : otherTextColor;
    const QColor currentTimeColor = isMine ? ownTimeColor : otherTimeColor;
    const QColor currentBorderColor = isMine ? ownBubbleBorderColor : otherBubbleBorderColor;

    painter->setPen(QPen(currentBorderColor, 1));
    painter->setBrush(isMine ? ownBubbleColor : otherBubbleColor);
    painter->drawRoundedRect(bubbleRect, 12, 12);

    if (hasAttachments)
    {
        painter->setFont(attachmentsFont);
        painter->setPen(currentTextColor);

        for (int i = 0; i < attachments.size(); ++i)
        {
            const QJsonObject obj = attachments.at(i).toObject();
            const QString fileName = obj.value("filename").toString(QString("file"));
            bool isImage = fileName.endsWith(".png", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpg", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                           fileName.endsWith(".bmp", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".gif", Qt::CaseInsensitive);

            if (isImage) {
                int imgWidth = contentRight - contentLeft;
                int imgHeight = 200;
                QString path = appDownloadsDir + "/" + fileName;
                QString localPath = obj.value("local_path").toString();
                if (!localPath.isEmpty() && QFileInfo::exists(localPath)) {
                    path = localPath;
                }
                QPixmap pix;
                if (QFileInfo::exists(path) && pix.load(path)) {
                    QSize scaled = pix.size().scaled(imgWidth, 300, Qt::KeepAspectRatio);
                    imgWidth = scaled.width();
                    imgHeight = scaled.height();
                    QRect imgRect(contentLeft, contentTop, imgWidth, imgHeight);
                    painter->drawPixmap(imgRect, pix.scaled(imgWidth, imgHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    QRect placeholderRect(contentLeft, contentTop, imgWidth, imgHeight);
                    painter->setBrush(QColor(isMine ? 230 : 40, isMine ? 230 : 40, isMine ? 230 : 40));
                    painter->setPen(Qt::NoPen);
                    painter->drawRect(placeholderRect);
                    painter->setPen(currentTextColor);
                    painter->drawText(placeholderRect, Qt::AlignCenter, "Изображение...");
                }
                contentTop += imgHeight;
            } else {
                const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());

                const QRect iconRect(contentLeft, contentTop, attachmentIconSize, attachmentIconSize);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(currentBorderColor, 1));
                painter->drawRoundedRect(iconRect, 3, 3);
                painter->drawLine(iconRect.topRight() + QPoint(-attachmentIconSize*0.3, attachmentIconSize*0.1),
                                  iconRect.topRight() + QPoint(-attachmentIconSize*0.1, attachmentIconSize*0.3));
                painter->drawLine(iconRect.topRight() + QPoint(-attachmentIconSize*0.3, attachmentIconSize*0.1),
                                  iconRect.topRight() + QPoint(-attachmentIconSize*0.1, attachmentIconSize*0.1));

                int btnWidth = 30;
                const int textLeft = iconRect.right() + attachmentSpacing;
                const QRect textRect(textLeft, contentTop,
                                     contentRight - textLeft - btnWidth - attachmentSpacing,
                                     rowHeight);
                const QString elidedName = attachmentsFm.elidedText(fileName, Qt::ElideRight, textRect.width());
                painter->setPen(currentTextColor);
                painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedName);

                QRect downloadBtnRect(contentRight - btnWidth, contentTop + (rowHeight - 20) / 2, btnWidth, 20);
                painter->setBrush(QColor(47, 107, 255));
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(downloadBtnRect, 4, 4);
                painter->setPen(Qt::white);
                painter->drawText(downloadBtnRect, Qt::AlignCenter, "⬇");

                contentTop += rowHeight;
            }

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
        painter->setPen(currentTextColor);
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
        painter->setPen(currentTimeColor);
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
            const QColor pendingColor = ownTimeColor;
            painter->setPen(QPen(pendingColor, 1));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(iconRect);

            const QPoint center = iconRect.center();
            painter->drawLine(center, QPoint(center.x(), center.y() - statusIconSize * 0.25));
            painter->drawLine(center, QPoint(center.x() + statusIconSize * 0.2, center.y()));
        }
        else if (isRead)
        {
            const QRectF iconRect(bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                                  bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                                  statusIconSize,
                                  statusIconSize);
            const QColor readColor(47, 107, 255); // Синий цвет для отчетов о прочтении, как во многих мессенджерах
            painter->setPen(QPen(readColor, qMax(1.0, statusIconSize * 0.15), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);

            const QPointF p1a(iconRect.left() + iconRect.width() * 0.1, iconRect.top() + iconRect.height() * 0.5);
            const QPointF p2a(iconRect.left() + iconRect.width() * 0.35, iconRect.bottom() - iconRect.height() * 0.2);
            const QPointF p3a(iconRect.right() - iconRect.width() * 0.3, iconRect.top() + iconRect.height() * 0.2);
            painter->drawLine(p1a, p2a);
            painter->drawLine(p2a, p3a);

            const QPointF p1b(iconRect.left() + iconRect.width() * 0.45, iconRect.top() + iconRect.height() * 0.5);
            const QPointF p2b(iconRect.left() + iconRect.width() * 0.7, iconRect.bottom() - iconRect.height() * 0.2);
            const QPointF p3b(iconRect.right() - iconRect.width() * 0.05, iconRect.top() + iconRect.height() * 0.2);
            painter->drawLine(p1b, p2b);
            painter->drawLine(p2b, p3b);
        }
        else
        {
            const QRectF iconRect(bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                                  bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                                  statusIconSize,
                                  statusIconSize);
            const QColor deliveredColor = ownTimeColor;
            painter->setPen(QPen(deliveredColor, qMax(1.0, statusIconSize * 0.15), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);

            const QPointF p1(iconRect.left() + iconRect.width() * 0.2, iconRect.top() + iconRect.height() * 0.5);
            const QPointF p2(iconRect.left() + iconRect.width() * 0.45, iconRect.bottom() - iconRect.height() * 0.2);
            const QPointF p3(iconRect.right() - iconRect.width() * 0.1, iconRect.top() + iconRect.height() * 0.2);
            painter->drawLine(p1, p2);
            painter->drawLine(p2, p3);
        }

        if (option.state & QStyle::State_MouseOver)
        {
            const int btnSize = qMax(24, static_cast<int>(baseHeight * 1.5));
            QRect editBtnRect(bubbleX - btnSize - 8, bubbleRect.center().y() - btnSize / 2, btnSize, btnSize);
            QRect deleteBtnRect(editBtnRect.left() - btnSize - 8, bubbleRect.center().y() - btnSize / 2, btnSize, btnSize);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 30));
            painter->drawEllipse(editBtnRect);
            painter->drawEllipse(deleteBtnRect);
            
            painter->setPen(QColor(200, 200, 200));
            QFont iconFont = option.font;
            iconFont.setPointSize(12);
            painter->setFont(iconFont);
            painter->drawText(editBtnRect, Qt::AlignCenter, "✎");
            painter->drawText(deleteBtnRect, Qt::AlignCenter, "🗑");
            m_editBtnRects[messageId] = editBtnRect;
            m_deleteBtnRects[messageId] = deleteBtnRect;
        } else {
            m_editBtnRects.remove(messageId);
            m_deleteBtnRects.remove(messageId);
        }
    }
    else
    {
        if (!isRead)
            setLastReadMessage(chatId, messageId);

        if (option.state & QStyle::State_MouseOver)
        {
            const int btnSize = qMax(24, static_cast<int>(baseHeight * 1.5));
            QRect deleteBtnRect(bubbleRect.right() + 8, bubbleRect.center().y() - btnSize / 2, btnSize, btnSize);
            
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 30));
            painter->drawEllipse(deleteBtnRect);
            
            painter->setPen(QColor(200, 200, 200));
            QFont iconFont = option.font;
            iconFont.setPointSize(12);
            painter->setFont(iconFont);
            painter->drawText(deleteBtnRect, Qt::AlignCenter, "🗑");
            
            m_editBtnRects.remove(messageId);
            m_deleteBtnRects[messageId] = deleteBtnRect;
        } else {
            m_editBtnRects.remove(messageId);
            m_deleteBtnRects.remove(messageId);
        }
    }

    painter->restore();
}

QSize ChatMessagesItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString timestampIso = index.data(ChatMessagesListModel::TimestampRole).toString().trimmed();
    bool showDateBadge = false;
    if (!timestampIso.isEmpty()) {
        QDateTime currentDt = QDateTime::fromString(timestampIso, Qt::ISODateWithMs);
        if (!currentDt.isValid()) currentDt = QDateTime::fromString(timestampIso, Qt::ISODate);
        if (currentDt.isValid()) {
            QDateTime prevDt;
            if (index.row() > 0) {
                QString prevDateStr = index.model()->index(index.row() - 1, 0).data(ChatMessagesListModel::TimestampRole).toString().trimmed();
                prevDt = QDateTime::fromString(prevDateStr, Qt::ISODateWithMs);
                if (!prevDt.isValid()) prevDt = QDateTime::fromString(prevDateStr, Qt::ISODate);
            }
            if (index.row() == 0 || (prevDt.isValid() && currentDt.toLocalTime().date() != prevDt.toLocalTime().date())) {
                showDateBadge = true;
            }
        }
    }
    const int dateBadgeHeight = showDateBadge ? 36 : 0;

    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
    const bool isPending = index.data(ChatMessagesListModel::IsPendingRole).toBool();
    const QJsonArray attachments = index.data(ChatMessagesListModel::AttachmentsRole).toJsonArray();
    const bool hasAttachments = !attachments.isEmpty();
    const bool hasMessage = !messageText.isEmpty();
    const QString safeMessage = hasMessage ? messageText : QString();

    const QRect rowRect = option.rect.adjusted(2, 4, -8, -4);
    const int maxBubbleWidth = qMax(200, static_cast<int>(rowRect.width() * 0.72));
    
    QFontMetrics textFm(option.font);
    const int baseHeight = textFm.height();
    const int horizontalPadding = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int verticalPadding = qMax(8, static_cast<int>(baseHeight * 0.5));


    QFont attachmentsFont = option.font;
    attachmentsFont.setPointSize(qMax(8, attachmentsFont.pointSize() - 1));
    QFontMetrics attachmentsFm(attachmentsFont);
    const int attachmentIconSize = qMax(18, static_cast<int>(baseHeight * 1.2));
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
            bool isImage = fileName.endsWith(".png", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpg", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                           fileName.endsWith(".bmp", Qt::CaseInsensitive)  ||
                           fileName.endsWith(".gif", Qt::CaseInsensitive);

            if (isImage) {
                int imgWidth = maxBubbleWidth - horizontalPadding * 2;
                int imgHeight = 200;
                QString path = appDownloadsDir + "/" + fileName;
                QString localPath = obj.value("local_path").toString();
                if (!localPath.isEmpty() && QFileInfo::exists(localPath)) {
                    path = localPath;
                }
                if (QFileInfo::exists(path)) {
                    QImageReader reader(path);
                    QSize sz = reader.size();
                    if (sz.isValid()) {
                        QSize scaled = sz.scaled(imgWidth, 300, Qt::KeepAspectRatio);
                        imgWidth = scaled.width();
                        imgHeight = scaled.height();
                    }
                }
                attachmentsHeight += imgHeight;
                attachmentsMaxWidth = qMax(attachmentsMaxWidth, imgWidth);
            } else {
                const int rowHeight = qMax(attachmentIconSize, attachmentsFm.height());
                attachmentsHeight += rowHeight;
                int btnWidth = 30;
                const int rowWidth = attachmentIconSize + attachmentSpacing + attachmentsFm.horizontalAdvance(fileName) + attachmentSpacing + btnWidth;
                attachmentsMaxWidth = qMax(attachmentsMaxWidth, rowWidth);
            }

            if (i < attachments.size() - 1)
                attachmentsHeight += attachmentsRowSpacing;
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
    const int pendingIconSize = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int statusSpacing = 4;
    const int statusHeight = qMax(timeFm.height(), isPending ? pendingIconSize : 0) + statusSpacing;

    const int rowHeight = textBounds.height() + verticalPadding * 2 + statusHeight + 6
        + (hasAttachments ? attachmentsHeight : 0)
        + (hasAttachments && hasMessage ? attachmentsBlockSpacing : 0);

    int finalHeight = qMax(44, rowHeight) + dateBadgeHeight;
    if (index.row() == index.model()->rowCount() - 1) {
        finalHeight += 80;
    }
    return QSize(option.rect.width(), finalHeight);
}

void ChatMessagesItemDelegate::setLastReadMessage(const quint64 chatId, const quint64 MessageId) const
{
    lastReadMessage = {chatId, MessageId};
}

bool ChatMessagesItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const unsigned long long senderId = index.data(ChatMessagesListModel::SenderIdRole).toULongLong();
            const bool isMine = senderId == m_currentUserId;
            const quint64 messageId = index.data(ChatMessagesListModel::MessageIdRole).toULongLong();
            QRect editBtnRect = m_editBtnRects.value(messageId);
            QRect deleteBtnRect = m_deleteBtnRects.value(messageId);
            
            if (isMine) {
                if (editBtnRect.isValid() && editBtnRect.contains(mouseEvent->pos())) {
                    const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
                    emit editMessageRequested(messageId, messageText);
                    return true;
                } else if (deleteBtnRect.isValid() && deleteBtnRect.contains(mouseEvent->pos())) {
                    emit deleteMessageRequested(messageId);
                    return true;
                }
            } else {
                if (deleteBtnRect.isValid() && deleteBtnRect.contains(mouseEvent->pos())) {
                    emit deleteMessageRequested(messageId);
                    return true;
                }
            }
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const unsigned long long senderId = index.data(ChatMessagesListModel::SenderIdRole).toULongLong();
            const bool isMine = senderId == m_currentUserId;
            
            if (isMine) {
                const quint64 messageId = index.data(ChatMessagesListModel::MessageIdRole).toULongLong();
                const QString messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();
                emit editMessageRequested(messageId, messageText);
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
