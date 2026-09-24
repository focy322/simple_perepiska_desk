#pragma once

#include <QLabel>

class AvatarOutlineOverlay : public QLabel
{
public:
    explicit AvatarOutlineOverlay(QWidget *parent = nullptr);

    void setAlpha(int alpha);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_alpha = 0;
};
