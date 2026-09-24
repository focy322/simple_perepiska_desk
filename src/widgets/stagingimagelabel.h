#pragma once

#include <QLabel>
#include <QPixmap>
#include <QPushButton>

#include <functional>

class QColor;
class QAbstractItemView;
class QResizeEvent;
class QVariantAnimation;

class PreviewOverlayButton : public QPushButton
{
public:
    PreviewOverlayButton(const QColor &defaultColor, const QColor &hoverBgColor, QWidget *parent = nullptr);

    void setAlpha(int alpha);

protected:
    void paintEvent(QPaintEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    int m_alpha = 0;
    QColor m_defaultColor;
    QColor m_hoverBgColor;
    bool m_isHovered = false;
};

class OverlayContainer : public QLabel
{
public:
    explicit OverlayContainer(QWidget *parent = nullptr);

    int alpha() const;
    void setAlpha(int alpha);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_alpha = 0;
};

class StagingImageLabel : public QLabel
{
public:
    explicit StagingImageLabel(QWidget *parent = nullptr);

    void setOriginalPixmap(const QPixmap &pixmap);
    const QPixmap &originalPixmap() const;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

    std::function<void()> onDeleteClicked;
    std::function<void()> onEnlargeClicked;

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_originalPixmap;
    OverlayContainer *m_overlayContainer;
    PreviewOverlayButton *m_enlargeButton;
    PreviewOverlayButton *m_deleteButton;
    QVariantAnimation *m_overlayAnimation;
};
