#include "delegates/chatlistitemdelegate.h"
#include "models/chatlistmodel.h"

#include <algorithm>
#include <QApplication>
#include <QDateTime>
#include <QPainter>

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

    const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
    const int avatarSize = 45;
    const QRect avatarRect(contentRect.left(), contentRect.top() + (contentRect.height() - avatarSize) / 2, avatarSize, avatarSize);
    const QRect textRect = contentRect.adjusted(avatarSize + 10, 0, 0, 0);

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

    const QColor avatarColor = (opt.state & QStyle::State_Selected)
        ? QColor(255, 235, 200)
        : QColor(225, 170, 110);

    const QColor avatarTextColor = (opt.state & QStyle::State_Selected)
        ? QColor(70, 50, 25)
        : QColor(255, 255, 255);

    const int timestampWidth = 50;
    const QRect timestampRect(textRect.right() - timestampWidth, avatarRect.top(), timestampWidth, avatarRect.height() / 2 + 5);
    const QRect titleRect(textRect.left(), avatarRect.top(), textRect.width() - timestampWidth - 6, avatarRect.height() / 2 + 5);
    const QRect subtitleRect(textRect.left(), titleRect.bottom(), textRect.width(), avatarRect.height() / 2 - 10);

    QFont avatarFont = opt.font;
    avatarFont.setBold(true);
    avatarFont.setPointSize(avatarFont.pointSize() + 1);

    QChar avatarLetter = QChar('#');
    if (!title.isEmpty())
        avatarLetter = title.at(0).toUpper();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(avatarColor);
    painter->drawEllipse(avatarRect);

    painter->setFont(avatarFont);
    painter->setPen(avatarTextColor);
    painter->drawText(avatarRect, Qt::AlignCenter, avatarLetter);

    painter->setFont(titleFont);
    painter->setPen(titleColor);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop,
                      QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

    painter->setFont(subtitleFont);
    painter->setPen(subtitleColor);
    painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignBottom,
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


    painter->restore();
}

QSize ChatListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, 70);
}
