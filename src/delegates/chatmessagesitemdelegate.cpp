#include "delegates/chatmessagesitemdelegate.h"
#include "models/chatmessageslistmodel.h"
#include "utils/parsed_objects.h"
#include "utils/paths.h"
#include "utils/videohelpers.h"
#include "utils/attachmenthelper.h"

#include <array>
#include <cmath>
#include <numbers>

#include <QAbstractItemView>
#include <QCursor>
#include <QDateTime>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

struct ChatMessagesItemDelegate::AttachmentLayout
{
    AttachmentKind kind = AttachmentKind::File;
    QString fileName;
    QString path;
    QString blurhash;
    QRect rect;
    QRect downloadButtonRect;
    QRect nameRect;
    QPixmap thumbnail;
};

struct ChatMessagesItemDelegate::MessageLayout
{
    quint64 messageId = ULONG_LONG_MAX;
    quint64 chatId = ULONG_LONG_MAX;
    bool isMine = false;
    bool isRead = false;
    bool isPending = false;
    QString messageText;
    QString timeText;
    QString dateText;
    QFont textFont;
    QFont timeFont;
    QFont attachmentsFont;
    QFont dateFont;
    QRect bubbleRect;
    QRect messageRect;
    QRect timeRect;
    QRect statusRect;
    QRect dateRect;
    QRect editButtonRect;
    QRect deleteButtonRect;
    int rowHeight = 0;
    std::vector<AttachmentLayout> attachments;
};

ChatMessagesItemDelegate::ChatMessagesItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_currentUserId(ULONG_LONG_MAX)
    , lastReadMessage{ULONG_LONG_MAX, ULONG_LONG_MAX}
{
    connect(VideoThumbnailManager::instance(), &VideoThumbnailManager::thumbnailReady, this, [this](const QString &) {
        if (auto view = qobject_cast<QAbstractItemView*>(this->parent())) {
            view->doItemsLayout();
            view->viewport()->update();
        }
    });
}

void ChatMessagesItemDelegate::setCurrentUserId(unsigned long long userId)
{
    m_currentUserId = userId;
}

QDateTime ChatMessagesItemDelegate::localDateTime(const QString &timestamp)
{
    const QString value = timestamp.trimmed();
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, Qt::ISODate);
    return dateTime.toLocalTime();
}

QString ChatMessagesItemDelegate::dateBadgeText(const QModelIndex &index, const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return {};

    if (index.row() > 0) {
        const QModelIndex previous = index.siblingAtRow(index.row() - 1);
        const QDateTime previousDateTime = localDateTime(previous.data(ChatMessagesListModel::TimestampRole).toString());
        if (previousDateTime.isValid() && previousDateTime.date() == dateTime.date())
            return {};
    }

    const QDate today = QDate::currentDate();
    const QDate date = dateTime.date();
    if (date == today)
        return tr("Сегодня");
    if (date == today.addDays(-1))
        return tr("Вчера");

    const QStringList months = {tr("января"), tr("февраля"), tr("марта"), tr("апреля"),
                                tr("мая"), tr("июня"), tr("июля"), tr("августа"),
                                tr("сентября"), tr("октября"), tr("ноября"), tr("декабря")};
    const QString text = tr("%1 %2").arg(date.day()).arg(months.at(date.month() - 1));
    return date.year() == today.year() ? text : tr("%1 %2").arg(text).arg(date.year());
}

QString ChatMessagesItemDelegate::messageTimeText(const QDateTime &dateTime, bool isEdited)
{
    if (!dateTime.isValid())
        return {};

    const QString time = dateTime.toString("HH:mm");
    return isEdited ? tr("Ред. %1").arg(time) : time;
}

ChatMessagesItemDelegate::AttachmentKind ChatMessagesItemDelegate::attachmentKind(const Attachment &attachment)
{
    const QString category = AttachmentHelper::category(attachment);
    if (category.compare("image", Qt::CaseInsensitive) == 0)
        return AttachmentKind::Image;
    if (category.compare("video", Qt::CaseInsensitive) == 0)
        return AttachmentKind::Video;
    return AttachmentKind::File;
}

QString ChatMessagesItemDelegate::attachmentFilePath(const Attachment &attachment)
{
    return AttachmentHelper::localFilePath(attachment);
}

QPixmap ChatMessagesItemDelegate::decodeBlurhash(const QString &hash)
{
    // https://github.com/woltapp/blurhash/blob/master/Algorithm.md
    constexpr int maxComponents = 9;
    constexpr int previewSize = 32;
    constexpr int maxHashLength = 4 + 2 * maxComponents * maxComponents;
    if (hash.size() < 6 || hash.size() > maxHashLength)
        return {};

    static const QString alphabet = QStringLiteral("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~");
    std::array<int, maxHashLength> digits{};
    for (qsizetype i = 0; i < hash.size(); ++i) {
        const int digit = alphabet.indexOf(hash.at(i));
        if (digit < 0)
            return {};
        digits[i] = digit;
    }
    const auto decode83 = [&digits](int offset, int length) {
        int value = 0;
        for (int i = 0; i < length; ++i)
            value = value * 83 + digits[offset + i];
        return value;
    };

    const int componentsX = digits[0] % maxComponents + 1;
    const int componentsY = digits[0] / maxComponents + 1;
    const int componentCount = componentsX * componentsY;
    if (componentsY > maxComponents || hash.size() != 4 + 2 * componentCount)
        return {};

    const auto srgbToLinear = [](int channel) {
        const double value = channel / 255.0;
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    const auto linearToSrgb = [](double channel) {
        const double value = qBound(0.0, channel, 1.0);
        const double srgb = value <= 0.0031308
            ? value * 12.92
            : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
        return static_cast<int>(srgb * 255 + 0.5);
    };

    std::array<std::array<double, 3>, maxComponents * maxComponents> colors{};
    const int dc = decode83(2, 4);
    if (dc > 0xffffff)
        return {};
    colors[0] = {srgbToLinear(dc >> 16), srgbToLinear((dc >> 8) & 255), srgbToLinear(dc & 255)};

    const double maximumValue = (digits[1] + 1) / 166.0;
    const auto decodeChannel = [maximumValue](int quantized) {
        const double value = (quantized - 9) / 9.0;
        return std::copysign(value * value, value) * maximumValue;
    };
    for (int i = 1; i < componentCount; ++i) {
        const int ac = decode83(4 + 2 * i, 2);
        if (ac >= 19 * 19 * 19)
            return {};
        colors[i] = {decodeChannel(ac / (19 * 19)), decodeChannel((ac / 19) % 19), decodeChannel(ac % 19)};
    }

    std::array<std::array<double, maxComponents>, previewSize> cosines{};
    for (int position = 0; position < previewSize; ++position)
        for (int component = 0; component < qMax(componentsX, componentsY); ++component)
            cosines[position][component] = std::cos(std::numbers::pi * position * component / previewSize);

    QImage image(previewSize, previewSize, QImage::Format_RGB32);
    for (int y = 0; y < previewSize; ++y) {
        auto pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < previewSize; ++x) {
            std::array<double, 3> color{};
            for (int j = 0; j < componentsY; ++j) {
                for (int i = 0; i < componentsX; ++i) {
                    const double basis = cosines[x][i] * cosines[y][j];
                    const auto &component = colors[i + j * componentsX];
                    for (int channel = 0; channel < 3; ++channel)
                        color[channel] += component[channel] * basis;
                }
            }
            pixels[x] = qRgb(linearToSrgb(color[0]), linearToSrgb(color[1]), linearToSrgb(color[2]));
        }
    }
    return QPixmap::fromImage(image);
}

QPixmap ChatMessagesItemDelegate::blurhashPlaceholder(const QString &hash) const
{
    if (hash.isEmpty() || hash.size() > 166)
        return {};
    if (const QPixmap *cached = m_blurhashCache.object(hash))
        return *cached;

    const QPixmap pixmap = decodeBlurhash(hash);
    m_blurhashCache.insert(hash, new QPixmap(pixmap));
    return pixmap;
}

ChatMessagesItemDelegate::MessageLayout ChatMessagesItemDelegate::createLayout(
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MessageLayout layout;
    layout.messageId = index.data(ChatMessagesListModel::MessageIdRole).toULongLong();
    layout.chatId = index.data(ChatMessagesListModel::ChatIdRole).toULongLong();
    layout.isMine = index.data(ChatMessagesListModel::SenderIdRole).toULongLong() == m_currentUserId;
    layout.isRead = index.data(ChatMessagesListModel::ReadRole).toBool();
    layout.isPending = index.data(ChatMessagesListModel::IsPendingRole).toBool();
    layout.messageText = index.data(ChatMessagesListModel::MessageTextRole).toString().trimmed();

    const QDateTime dateTime = localDateTime(index.data(ChatMessagesListModel::TimestampRole).toString());
    layout.timeText = messageTimeText(dateTime, index.data(ChatMessagesListModel::EditedRole).toBool());
    layout.dateText = dateBadgeText(index, dateTime);
    const int dateBadgeHeight = layout.dateText.isEmpty() ? 0 : (index.row() == 0 ? 116 : 36);

    layout.textFont = option.font;
    layout.timeFont = option.font;
    layout.timeFont.setPointSize(qMax(7, layout.timeFont.pointSize() - 1));
    layout.attachmentsFont = option.font;
    layout.attachmentsFont.setPointSize(qMax(8, layout.attachmentsFont.pointSize() - 1));
    layout.dateFont = layout.attachmentsFont;

    const QFontMetrics textFm(layout.textFont);
    const QFontMetrics timeFm(layout.timeFont);
    const QFontMetrics attachmentsFm(layout.attachmentsFont);
    const int baseHeight = textFm.height();
    const int horizontalPadding = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int verticalPadding = qMax(8, static_cast<int>(baseHeight * 0.5));
    const QRect rowRect = option.rect.adjusted(2, 4 + dateBadgeHeight, -8, -4);
    const int maxBubbleWidth = qMin(qMax(1, rowRect.width()), qMax(200, static_cast<int>(rowRect.width() * 0.72)));
    const int maxContentWidth = qMax(1, maxBubbleWidth - horizontalPadding * 2);
    const int attachmentSpacing = 6;
    const int downloadButtonWidth = 30;
    const int statusIconSize = qMax(12, static_cast<int>(baseHeight * 0.8));
    const int statusSpacing = 4;
    const int timeHeight = layout.timeText.isEmpty() ? 0 : timeFm.height();
    const int statusHeight = qMax(timeHeight, layout.isMine ? statusIconSize : 0) + statusSpacing;
    const int bottomWidth = timeFm.horizontalAdvance(layout.timeText)
        + (layout.isMine ? statusIconSize + statusSpacing : 0);

    const auto attachments = index.data(ChatMessagesListModel::AttachmentsRole).value<std::vector<Attachment>>();
    layout.attachments.reserve(attachments.size());
    int attachmentsHeight = 0;
    int attachmentsWidth = 0;
    for (const Attachment &attachment : attachments) {
        AttachmentLayout item;
        item.kind = attachmentKind(attachment);
        item.fileName = attachment.fileName.isEmpty() ? tr("file") : attachment.fileName;
        item.path = attachmentFilePath(attachment);
        item.blurhash = attachment.blurhash;
        QSize size;

        if (item.kind == AttachmentKind::Image || item.kind == AttachmentKind::Video) {
            QSize sourceSize;
            if (item.kind == AttachmentKind::Video) {
                item.thumbnail = VideoThumbnailManager::instance()->getThumbnail(item.path, attachment.thumbnailUrl);
                sourceSize = item.thumbnail.size();
            } else if (!item.path.isEmpty()) {
                QImageReader reader(item.path);
                sourceSize = reader.size();
            }
            size = sourceSize.isEmpty()
                ? QSize(maxContentWidth, 200)
                : sourceSize.scaled(maxContentWidth, 300, Qt::KeepAspectRatio);
        } else {
            const int rowHeight = qMax(20, qMax(static_cast<int>(baseHeight * 1.2), attachmentsFm.height()));
            size = QSize(qMin(maxContentWidth, downloadButtonWidth + attachmentSpacing
                              + attachmentsFm.horizontalAdvance(item.fileName)), rowHeight);
        }

        if (!layout.attachments.empty())
            attachmentsHeight += attachmentSpacing;
        item.rect = QRect(QPoint(0, attachmentsHeight), size);
        attachmentsHeight += size.height();
        attachmentsWidth = qMax(attachmentsWidth, size.width());
        layout.attachments.push_back(item);
    }

    const int textFlags = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop;
    QRect textBounds;
    if (!layout.messageText.isEmpty())
        textBounds = textFm.boundingRect(QRect(0, 0, maxContentWidth, 10000), textFlags, layout.messageText);

    const int minimumContentWidth = qMax(1, qMin(60, maxBubbleWidth) - horizontalPadding * 2);
    const int contentWidth = qMin(maxContentWidth,
        qMax(minimumContentWidth, qMax(attachmentsWidth, qMax(textBounds.width(), bottomWidth))));
    if (!layout.messageText.isEmpty())
        textBounds = textFm.boundingRect(QRect(0, 0, contentWidth, 10000), textFlags, layout.messageText);

    const int blockSpacing = !attachments.empty() && !layout.messageText.isEmpty() ? attachmentSpacing : 0;
    const int bubbleWidth = qMin(maxBubbleWidth, contentWidth + horizontalPadding * 2);
    const int bubbleHeight = textBounds.height() + verticalPadding * 2 + statusHeight + attachmentsHeight + blockSpacing;
    const int bubbleX = layout.isMine ? rowRect.right() - bubbleWidth + 1 : rowRect.left();
    layout.bubbleRect = QRect(bubbleX, rowRect.top(), bubbleWidth, bubbleHeight);
    const QPoint contentOrigin = layout.bubbleRect.topLeft() + QPoint(horizontalPadding, verticalPadding);

    for (AttachmentLayout &item : layout.attachments) {
        item.rect.translate(contentOrigin);
        if (item.kind == AttachmentKind::File) {
            item.rect.setWidth(contentWidth);
            item.downloadButtonRect = QRect(item.rect.left(), item.rect.center().y() - 10,
                                            qMin(downloadButtonWidth, contentWidth), 20);
            const int textLeft = item.downloadButtonRect.right() + attachmentSpacing + 1;
            item.nameRect = QRect(textLeft, item.rect.top(), qMax(0, item.rect.right() - textLeft + 1), item.rect.height());
        }
    }

    layout.messageRect = QRect(contentOrigin + QPoint(0, attachmentsHeight + blockSpacing),
                               QSize(contentWidth, textBounds.height()));
    layout.statusRect = QRect(layout.bubbleRect.right() - horizontalPadding - statusIconSize + 1,
                              layout.bubbleRect.bottom() - verticalPadding - statusIconSize + 1,
                              statusIconSize, statusIconSize);
    layout.timeRect = QRect(contentOrigin.x(), layout.bubbleRect.bottom() - verticalPadding - timeHeight + 1,
                            qMax(0, contentWidth - (layout.isMine ? statusIconSize + statusSpacing : 0)), timeHeight);

    if (!layout.dateText.isEmpty()) {
        const int badgeWidth = QFontMetrics(layout.dateFont).horizontalAdvance(layout.dateText) + 30;
        const int topOffset = index.row() == 0 ? 86 : 6;
        layout.dateRect = QRect(option.rect.left() + (option.rect.width() - badgeWidth) / 2,
                                option.rect.top() + topOffset, badgeWidth, 24);
    }

    const int buttonSize = qMax(24, static_cast<int>(baseHeight * 1.5));
    const int buttonTop = layout.bubbleRect.center().y() - buttonSize / 2;
    if (layout.isMine) {
        layout.editButtonRect = QRect(layout.bubbleRect.left() - buttonSize - 8, buttonTop, buttonSize, buttonSize);
        layout.deleteButtonRect = layout.editButtonRect.translated(-buttonSize - 8, 0);
    } else {
        layout.deleteButtonRect = QRect(layout.bubbleRect.right() + 8, buttonTop, buttonSize, buttonSize);
    }

    layout.rowHeight = qMax(44, bubbleHeight + 8) + dateBadgeHeight;
    if (index.row() == index.model()->rowCount(index.parent()) - 1)
        layout.rowHeight += 80;
    return layout;
}

void ChatMessagesItemDelegate::paintDateBadge(QPainter *painter, const MessageLayout &layout)
{
    if (layout.dateText.isEmpty())
        return;

    painter->setPen(QPen(QColor(51, 51, 51), 1));
    painter->setBrush(QColor(20, 20, 20));
    painter->drawRoundedRect(layout.dateRect, 12, 12);
    painter->setPen(QColor(230, 232, 235));
    painter->setFont(layout.dateFont);
    painter->drawText(layout.dateRect, Qt::AlignCenter, layout.dateText);
}

void ChatMessagesItemDelegate::paintAttachments(QPainter *painter, const MessageLayout &layout) const
{
    const QColor textColor = layout.isMine ? QColor(30, 30, 30) : QColor(230, 232, 235);
    const QFontMetrics fm(layout.attachmentsFont);
    painter->setFont(layout.attachmentsFont);

    for (const AttachmentLayout &item : layout.attachments) {
        if (item.kind == AttachmentKind::File) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(47, 107, 255));
            painter->drawRoundedRect(item.downloadButtonRect, 4, 4);
            painter->setPen(Qt::white);
            painter->drawText(item.downloadButtonRect, Qt::AlignCenter, QStringLiteral("⬇"));
            painter->setPen(textColor);
            painter->drawText(item.nameRect, Qt::AlignVCenter | Qt::AlignLeft,
                              fm.elidedText(item.fileName, Qt::ElideRight, item.nameRect.width()));
            continue;
        }

        const bool isVideo = item.kind == AttachmentKind::Video;
        QPixmap pixmap = item.thumbnail;
        if (!isVideo && !item.path.isEmpty())
            pixmap.load(item.path);
        const bool isPlaceholder = pixmap.isNull();
        if (isPlaceholder)
            pixmap = blurhashPlaceholder(item.blurhash);
        if (pixmap.isNull()) {
            painter->setBrush(layout.isMine ? QColor(230, 230, 230) : QColor(40, 40, 40));
            painter->setPen(Qt::NoPen);
            painter->drawRect(item.rect);
            painter->setPen(textColor);
            painter->drawText(item.rect, Qt::AlignCenter, isVideo ? tr("Загрузка видео...") : tr("Изображение..."));
            continue;
        }

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(item.rect, pixmap);
        if (isVideo && !isPlaceholder) {
            const QPoint center = item.rect.center();
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 0, 0, 150));
            painter->drawEllipse(QRect(center - QPoint(25, 25), QSize(50, 50)));
            painter->setBrush(Qt::white);
            QPolygon triangle;
            triangle << center + QPoint(-10, -15) << center + QPoint(-10, 15) << center + QPoint(15, 0);
            painter->drawPolygon(triangle);
        }
    }
}

void ChatMessagesItemDelegate::paintDeliveryStatus(QPainter *painter, const MessageLayout &layout)
{
    if (!layout.isMine)
        return;

    const QRectF rect = layout.statusRect;
    const qreal size = rect.width();
    painter->setBrush(Qt::NoBrush);
    if (layout.isPending) {
        painter->setPen(QPen(QColor(120, 120, 120), 1));
        painter->drawEllipse(rect);
        const QPointF center = rect.center();
        painter->drawLine(center, center + QPointF(0, -size * 0.25));
        painter->drawLine(center, center + QPointF(size * 0.2, 0));
        return;
    }

    const QColor color = layout.isRead ? QColor(47, 107, 255) : QColor(120, 120, 120);
    painter->setPen(QPen(color, qMax(1.0, size * 0.15), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPolygonF check;
    if (layout.isRead) {
        check << QPointF(rect.left() + size * 0.1, rect.top() + size * 0.5)
              << QPointF(rect.left() + size * 0.35, rect.bottom() - size * 0.2)
              << QPointF(rect.right() - size * 0.3, rect.top() + size * 0.2);
        painter->drawPolyline(check);
        check.clear();
        check << QPointF(rect.left() + size * 0.45, rect.top() + size * 0.5)
              << QPointF(rect.left() + size * 0.7, rect.bottom() - size * 0.2)
              << QPointF(rect.right() - size * 0.05, rect.top() + size * 0.2);
    } else {
        check << QPointF(rect.left() + size * 0.2, rect.top() + size * 0.5)
              << QPointF(rect.left() + size * 0.45, rect.bottom() - size * 0.2)
              << QPointF(rect.right() - size * 0.1, rect.top() + size * 0.2);
    }
    painter->drawPolyline(check);
}

void ChatMessagesItemDelegate::paintActionButtons(QPainter *painter, const QStyleOptionViewItem &option,
                                                 const MessageLayout &layout)
{
    if (!(option.state & QStyle::State_MouseOver))
        return;

    if (const QWidget *widget = option.widget) {
        if (const auto view = qobject_cast<const QAbstractItemView*>(widget))
            widget = view->viewport();
        const QPoint position = widget->mapFromGlobal(QCursor::pos());
        if (position.y() < layout.bubbleRect.top() || position.y() > layout.bubbleRect.bottom())
            return;
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 30));
    if (layout.editButtonRect.isValid()) {
        painter->drawEllipse(layout.editButtonRect);
        QIcon(":/icons/edit.svg").paint(painter, layout.editButtonRect.adjusted(4, 4, -4, -4), Qt::AlignCenter);
    }
    painter->drawEllipse(layout.deleteButtonRect);
    QIcon(":/icons/delete.svg").paint(painter, layout.deleteButtonRect.adjusted(4, 4, -4, -4), Qt::AlignCenter);
}

void ChatMessagesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    const MessageLayout layout = createLayout(option, index);
    const QColor textColor = layout.isMine ? QColor(30, 30, 30) : QColor(230, 232, 235);
    const QColor timeColor = layout.isMine ? QColor(120, 120, 120) : QColor(140, 150, 160);
    const QColor borderColor = layout.isMine ? QColor(235, 230, 220) : QColor(40, 45, 50);

    painter->save();
    painter->setClipRect(option.rect, Qt::IntersectClip);
    painter->setRenderHint(QPainter::Antialiasing, true);
    paintDateBadge(painter, layout);

    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(layout.isMine ? QColor(250, 249, 246) : QColor(20, 22, 25));
    painter->drawRoundedRect(layout.bubbleRect, 12, 12);
    paintAttachments(painter, layout);

    if (!layout.messageText.isEmpty()) {
        painter->setFont(layout.textFont);
        painter->setPen(textColor);
        painter->drawText(layout.messageRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, layout.messageText);
    }
    if (!layout.timeText.isEmpty()) {
        painter->setFont(layout.timeFont);
        painter->setPen(timeColor);
        painter->drawText(layout.timeRect, Qt::AlignRight | Qt::AlignVCenter, layout.timeText);
    }
    paintDeliveryStatus(painter, layout);
    paintActionButtons(painter, option, layout);
    painter->restore();

    if (!layout.isMine && !layout.isRead && !layout.isPending)
        setLastReadMessage(layout.chatId, layout.messageId);
}

QSize ChatMessagesItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return QStyledItemDelegate::sizeHint(option, index);
    return QSize(option.rect.width(), createLayout(option, index).rowHeight);
}

void ChatMessagesItemDelegate::setLastReadMessage(const quint64 chatId, const quint64 messageId) const
{
    if (lastReadMessage.first != chatId || lastReadMessage.second == ULONG_LONG_MAX || messageId > lastReadMessage.second)
        lastReadMessage = {chatId, messageId};
}

void ChatMessagesItemDelegate::openVideoPlayer(const QString &path)
{
    if (m_videoOpenScheduled)
        return;
    m_videoOpenScheduled = true;
    // Открываем окно после завершения mouseReleaseEvent у списка, когда захват мыши уже снят.
    QTimer::singleShot(0, this, [this, path]() {
        m_videoOpenScheduled = false;
        auto view = qobject_cast<QWidget*>(parent());
        auto dialog = new VideoPlayerDialog(path, view ? view->window() : nullptr);
        dialog->show();
    });
}

bool ChatMessagesItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                          const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (!index.isValid() || (event->type() != QEvent::MouseButtonRelease &&
        event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseButtonDblClick))
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    const auto mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->button() != Qt::LeftButton || event->type() == QEvent::MouseButtonPress)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    const MessageLayout layout = createLayout(option, index);
    const QPoint position = mouseEvent->position().toPoint();
    if (event->type() == QEvent::MouseButtonRelease) {
        if (layout.isMine && layout.editButtonRect.contains(position)) {
            emit editMessageRequested(layout.messageId, layout.messageText);
            return true;
        }
        if (layout.deleteButtonRect.contains(position)) {
            emit deleteMessageRequested(layout.messageId);
            return true;
        }
        for (const AttachmentLayout &item : layout.attachments) {
            if (item.kind == AttachmentKind::Video && item.rect.contains(position) && !item.path.isEmpty()) {
                openVideoPlayer(item.path);
                return true;
            }
        }
    } else {
        for (const AttachmentLayout &item : layout.attachments) {
            if (item.kind == AttachmentKind::Video && item.rect.contains(position)) {
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick && layout.isMine && layout.bubbleRect.contains(position)) {
            emit editMessageRequested(layout.messageId, layout.messageText);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
