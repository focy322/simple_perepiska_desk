#ifndef CUSTOMIZATIONPREVIEW_H
#define CUSTOMIZATIONPREVIEW_H

#include <QPixmap>
#include <QColor>
#include <QPolygonF>
#include <QVariantAnimation>
#include <QWidget>
#include <vector>

class QAbstractItemModel;

/// Послойный предпросмотр формы. Снимки не принимают ввод и не меняют рабочие виджеты.
class CustomizationPreview : public QWidget
{
    Q_OBJECT
public:
    enum class Screen
    {
        Messenger,
        Loading,
        Registration,
        Login
    };
    Q_ENUM(Screen)

    explicit CustomizationPreview(QWidget *parent = nullptr);

    void setPreview(Screen screen, const QSize &windowSize, const QPixmap &avatar = QPixmap(),
                    QAbstractItemModel *chatsModel = nullptr, const QString &styleSheet = QString(),
                    bool preserveSelection = false);
    static QString screenName(Screen screen);
    bool hasSelection() const { return m_editing; }
    QString selectedObjectName() const;
    QColor selectedColor(const QString &property) const;
    bool selectElement(const QString &objectName);
    void clearSelection(bool animated = true);

signals:
    void hoveredElementChanged(const QString &title, const QString &objectName);
    void selectionChanged(const QString &title, const QString &objectName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Layer
    {
        QString title;
        QString objectName;
        QRectF geometry;
        QPixmap pixmap;
        qreal depth;
        qreal radius;
        qreal hover = 0.0;
        qreal startHover = 0.0;
        QColor background;
        QColor text;
        QColor border;
    };

    QPointF project(const QPointF &point, qreal depth) const;
    QPolygonF polygon(const Layer &layer, qreal hover) const;
    QPolygonF displayPolygon(int index) const;
    QRectF focusArea() const;
    int layerAt(const QPointF &point) const;
    void fitPreview();
    void setHoveredLayer(int index);

    std::vector<Layer> m_layers;
    QSize m_windowSize;
    qreal m_scale = 1.0;
    QPointF m_offset;
    int m_hoveredLayer = -1;
    QVariantAnimation m_hoverAnimation;
    QVariantAnimation m_focusAnimation;
    int m_selectedLayer = -1;
    qreal m_focusProgress = 0.0;
    bool m_editing = false;
};

#endif // CUSTOMIZATIONPREVIEW_H
