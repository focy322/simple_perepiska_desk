#ifndef CHATPREVIEWMODEL_H
#define CHATPREVIEWMODEL_H

#include <QAbstractListModel>
#include "utils/parsed_objects.h"

/// Локальный образец элемента чата для конструктора при пустом списке.
class ChatPreviewModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ChatPreviewModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    MessagePreview m_message;
};

#endif // CHATPREVIEWMODEL_H
