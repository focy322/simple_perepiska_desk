#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // SEARCHITEMDELEGATE_H