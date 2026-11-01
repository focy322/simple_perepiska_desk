#include "delegates/searchitemdelegate.h"
#include "models/searchlistmodel.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

SearchItemDelegate::SearchItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

int SearchItemDelegate::avatarSizeForFont(const QFont &font)
{
    return qMax(40, static_cast<int>(QFontMetrics(font).height() * 2.2));
}

QString SearchItemDelegate::lastSeenText(const QString &timestamp)
{
    const QString value = timestamp.trimmed();
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, Qt::ISODate);

    return dateTime.isValid() ? dateTime.toLocalTime().toString("HH:mm") : value;
}

void SearchItemDelegate::paintAvatar(QPainter *painter, const QRect &rect, const QString &name,
                                     const QFont &font, bool isSelected)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(isSelected ? QColor(200, 225, 255) : QColor(100, 150, 240));
    painter->drawEllipse(rect);
    painter->setFont(font);
    painter->setPen(isSelected ? QColor(40, 60, 90) : QColor(Qt::white));
    painter->drawText(rect, Qt::AlignCenter, name.isEmpty() ? QChar('?') : name.at(0).toUpper());
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString nickname = index.data(SearchListModel::NicknameRole).toString().trimmed();
    const QString username = index.data(SearchListModel::UsernameRole).toString().trimmed();
    QString name = nickname.isEmpty() ? username : nickname;
    if (name.isEmpty())
        name = tr("Пользователь");

    QFont nameFont = opt.font;
    nameFont.setBold(true);
    if (nameFont.pointSizeF() > 0)
        nameFont.setPointSizeF(nameFont.pointSizeF() + 1);
    else
        nameFont.setPixelSize(nameFont.pixelSize() + 1);

    const int baseHeight = QFontMetrics(opt.font).height();
    const int avatarSize = avatarSizeForFont(opt.font);
    const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
    const QRect avatarRect(contentRect.left(), contentRect.top() + (contentRect.height() - avatarSize) / 2,
                           avatarSize, avatarSize);
    const bool isSelected = opt.state & QStyle::State_Selected;
    const bool isCollapsed = opt.rect.width() < baseHeight * 6;

    painter->save();
    painter->setClipRect(opt.rect, Qt::IntersectClip);

    const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    paintAvatar(painter, avatarRect, name, nameFont, isSelected);

    if (!isCollapsed) {
        QFont subtitleFont = opt.font;
        subtitleFont.setBold(false);
        QFont timeFont = subtitleFont;
        if (timeFont.pointSizeF() > 0)
            timeFont.setPointSizeF(qMax(7.0, timeFont.pointSizeF() - 1));
        else
            timeFont.setPixelSize(qMax(9, timeFont.pixelSize() - 1));

        const QColor textColor = opt.palette.color(isSelected ? QPalette::HighlightedText : QPalette::Text);
        const QColor detailColor = isSelected ? opt.palette.color(QPalette::HighlightedText) : QColor(95, 95, 95);
        const QString timeText = lastSeenText(index.data(SearchListModel::LastSeenRole).toString());
        const QString usernameText = username.isEmpty() ? QString() : QStringLiteral("@") + username;
        const QFontMetrics timeFm(timeFont);

        const QRect textRect = contentRect.adjusted(avatarSize + 10, 0, 0, 0);
        const int textWidth = qMax(0, textRect.width());
        const int timeWidth = qMin(timeFm.horizontalAdvance(timeText), textWidth / 2);
        const int titleRightPadding = timeWidth > 0 ? timeWidth + 6 : 0;
        const int titleHeight = avatarRect.height() / 2;
        const QRect titleRect(textRect.left(), avatarRect.top(), qMax(0, textWidth - titleRightPadding), titleHeight);
        const QRect subtitleRect(textRect.left(), titleRect.bottom() + 1, textWidth, avatarRect.height() - titleHeight);
        const QRect timeRect(textRect.right() - timeWidth + 1, avatarRect.top(), timeWidth, titleHeight);

        painter->setFont(nameFont);
        painter->setPen(textColor);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                          QFontMetrics(nameFont).elidedText(name, Qt::ElideRight, titleRect.width()));

        painter->setFont(subtitleFont);
        painter->setPen(detailColor);
        painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignTop,
                          QFontMetrics(subtitleFont).elidedText(usernameText, Qt::ElideRight, subtitleRect.width()));

        if (!timeText.isEmpty()) {
            painter->setFont(timeFont);
            painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter,
                              timeFm.elidedText(timeText, Qt::ElideRight, timeRect.width()));
        }
    }

    painter->restore();
}

QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const int baseHeight = QFontMetrics(opt.font).height();
    return QSize(0, avatarSizeForFont(opt.font) + qMax(10, static_cast<int>(baseHeight * 0.6)));
}
