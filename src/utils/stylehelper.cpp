#include "stylehelper.h"

#include <QDebug>
#include <QFile>
#include <QStyle>
#include <QWidget>

namespace StyleHelper
{
    QString load(const QString &resourcePath)
    {
        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Cannot load style sheet:" << resourcePath << file.errorString();
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }

    void repolish(QWidget *widget)
    {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
}
