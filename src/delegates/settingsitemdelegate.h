#ifndef SETTINGSITEMDELEGATE_H
#define SETTINGSITEMDELEGATE_H

#include <QStyledItemDelegate>

class SettingsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SettingsItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // SETTINGSITEMDELEGATE_H
