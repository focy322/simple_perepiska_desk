#include "searchitemdelegate.h"
#include "searchlistmodel.h"

#include <QApplication>
#include <QPainter>
#include <QFontMetrics>

SearchItemDelegate::SearchItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Извлекаем данные из модели поиска
    const QString nickname = index.data(SearchListModel::NicknameRole).toString().trimmed();
    const QString username = index.data(SearchListModel::UsernameRole).toString().trimmed();
    const QString lastSeenRaw = index.data(SearchListModel::LastSeenRole).toString().trimmed();

    painter->save();

    // 1. Отрисовка фона выделения
    const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
    const int avatarSize = 45;
    const QRect avatarRect(contentRect.left(), contentRect.top() + (contentRect.height() - avatarSize) / 2, avatarSize, avatarSize);
    const QRect textRect = contentRect.adjusted(avatarSize + 10, 0, 0, 0);

    // 2. Настройка шрифтов и цветов
    QFont nameFont = opt.font;
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);

    QFont subFont = opt.font;
    subFont.setBold(false);

    const QColor mainTextColor = (opt.state & QStyle::State_Selected)
                                     ? opt.palette.color(QPalette::HighlightedText)
                                     : opt.palette.color(QPalette::Text);

    const QColor subTextColor = (opt.state & QStyle::State_Selected)
                                    ? opt.palette.color(QPalette::HighlightedText)
                                    : QColor(95, 95, 95);

    // Цвета аватара (сделаем чуть отличными от основного чата для визуального разделения)
    const QColor avatarColor = (opt.state & QStyle::State_Selected)
                                   ? QColor(200, 225, 255)
                                   : QColor(100, 150, 240);

    const QColor avatarTextColor = (opt.state & QStyle::State_Selected)
                                       ? QColor(40, 60, 90)
                                       : Qt::white;

    // 3. Форматирование времени
    QString timeDisplay;
    if (!lastSeenRaw.isEmpty())
    {
        QDateTime dt = QDateTime::fromString(lastSeenRaw, Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(lastSeenRaw, Qt::ISODate);

        if (dt.isValid())
            timeDisplay = dt.toLocalTime().toString("HH:mm");
        else
            timeDisplay = lastSeenRaw;
    }

    // 4. Геометрия текста
    const int timestampWidth = 50;
    const QRect timestampRect(textRect.right() - timestampWidth, avatarRect.top(), timestampWidth, avatarRect.height() / 2);
    const QRect titleRect(textRect.left(), avatarRect.top(), textRect.width() - timestampWidth - 6, avatarRect.height() / 2);
    const QRect subtitleRect(textRect.left(), titleRect.bottom(), textRect.width(), avatarRect.height() / 2);

    // 5. Отрисовка аватара-заглушки
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(avatarColor);
    painter->drawEllipse(avatarRect);

    painter->setFont(nameFont);
    painter->setPen(avatarTextColor);
    QChar avatarLetter = nickname.isEmpty() ? QChar('?') : nickname.at(0).toUpper();
    painter->drawText(avatarRect, Qt::AlignCenter, avatarLetter);

    // 6. Отрисовка Никнейма[cite: 1]
    painter->setFont(nameFont);
    painter->setPen(mainTextColor);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(nameFont).elidedText(nickname, Qt::ElideRight, titleRect.width()));

    // 7. Отрисовка @username (Subtitle)
    painter->setFont(subFont);
    painter->setPen(subTextColor);
    QString displayUser = username.isEmpty() ? "" : "@" + username;
    painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignTop,
                      QFontMetrics(subFont).elidedText(displayUser, Qt::ElideRight, subtitleRect.width()));

    // 8. Отрисовка времени
    if (!timeDisplay.isEmpty()) {
        QFont timeFont = subFont;
        timeFont.setPointSize(std::max(7, timeFont.pointSize() - 1));
        painter->setFont(timeFont);
        painter->setPen(subTextColor);
        painter->drawText(timestampRect, Qt::AlignRight | Qt::AlignVCenter, timeDisplay);
    }

    painter->restore();
}
QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, 60); // Чуть компактнее, чем основной список чатов
}