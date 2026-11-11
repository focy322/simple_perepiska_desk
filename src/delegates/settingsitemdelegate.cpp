#include "settingsitemdelegate.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QWidget>

SettingsItemDelegate::SettingsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SettingsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.rect = opt.rect.adjusted(5, 3, -5, -3);
    opt.decorationSize = QSize(24, 24);
    opt.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    opt.font.setBold(true);
    opt.fontMetrics = QFontMetrics(opt.font);
    opt.textElideMode = Qt::ElideRight;

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

QSize SettingsItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.font.setBold(true);
    const QFontMetrics metrics(opt.font);
    return QSize(metrics.horizontalAdvance(opt.text) + 64, qMax(52, metrics.height() + 24));
}
