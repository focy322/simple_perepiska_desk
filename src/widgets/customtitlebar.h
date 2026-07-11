#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QPushButton>

class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget *parent = nullptr);



private slots:
    void onMinimizeClicked();
    void onMaximizeRestoreClicked();
    void onCloseClicked();

private:
    QPushButton *m_minimizeBtn;
    QPushButton *m_maximizeBtn;
    QPushButton *m_closeBtn;
};

#endif // CUSTOMTITLEBAR_H
