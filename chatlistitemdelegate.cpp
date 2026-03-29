#include "chatlistitemdelegate.h"

#include <QApplication>
#include <QPainter>

ChatListItemDelegate::ChatListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void ChatListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString rawText = index.data(Qt::DisplayRole).toString();
    const QStringList lines = rawText.split('\n');
    const QString title = lines.value(0).trimmed();
    const QString subtitle = lines.value(1).trimmed();

    painter->save();

    const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    const QRect contentRect = opt.rect.adjusted(10, 6, -10, -6);

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

    const QRect titleRect(contentRect.left(), contentRect.top(), contentRect.width(), contentRect.height() / 2);
    const QRect subtitleRect(contentRect.left(), titleRect.bottom(), contentRect.width(), contentRect.height() / 2);

    painter->setFont(titleFont);
    painter->setPen(titleColor);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

    painter->setFont(subtitleFont);
    painter->setPen(subtitleColor);
    painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(subtitleFont).elidedText(subtitle, Qt::ElideRight, subtitleRect.width()));

    painter->restore();
}

QSize ChatListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, 70);
}
