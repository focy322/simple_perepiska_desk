#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QString>

class QWidget;

namespace StyleHelper
{
    // Загружает QSS из ресурса Qt; при ошибке возвращает пустую строку и пишет предупреждение.
    QString load(const QString &resourcePath);

    // Обновляет оформление виджета после изменения свойств, используемых в селекторах QSS.
    void repolish(QWidget *widget);
}

#endif // STYLEHELPER_H
