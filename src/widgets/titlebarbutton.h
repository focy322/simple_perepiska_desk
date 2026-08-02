#ifndef TITLEBARBUTTON_H
#define TITLEBARBUTTON_H

#include <QAbstractButton>
#include <QColor>

class TitleBarButton : public QAbstractButton {
    Q_OBJECT
public:
    TitleBarButton(const QColor& defaultColor, const QColor& hoverBgColor, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QColor m_defaultColor;
    QColor m_hoverBgColor;
    bool m_isHovered;
};

#endif // TITLEBARBUTTON_H
