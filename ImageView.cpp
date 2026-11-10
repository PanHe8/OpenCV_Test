#include "ImageView.h"
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPen>
#include <QFont>
#include <cmath>

ImageView::ImageView(QWidget* parent) : QGraphicsView(parent), scene_(this)
{
    setScene(&scene_);
    setBackgroundBrush(QColor(35, 39, 45));
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMinimumSize(420, 360);
}

void ImageView::setImage(const QImage& image)
{
    overlays_.clear();
    scene_.clear();
    image_ = image;
    if (!image.isNull()) scene_.addPixmap(QPixmap::fromImage(image));
    scene_.setSceneRect(QRectF(image.rect()));
    fitImage();
}

void ImageView::setCircles(const std::vector<vision::Circle>& circles)
{
    for (auto* item : overlays_) {
        scene_.removeItem(item);
        delete item;
    }
    overlays_.clear();
    QPen circlePen(QColor(0, 220, 100), 2);
    QPen centerPen(QColor(255, 70, 60), 1);
    circlePen.setCosmetic(true);
    centerPen.setCosmetic(true);
    // 十字固定为 6 个界面像素，不随图片缩放而变大。
    const double crossSize = 3.0;
    QPainterPath crossPath;
    crossPath.moveTo(-crossSize, 0);
    crossPath.lineTo(crossSize, 0);
    crossPath.moveTo(0, -crossSize);
    crossPath.lineTo(0, crossSize);
    int index = 0;
    for (const auto& circle : circles) {
        const double x = circle.center.x, y = circle.center.y, r = circle.radius;
        overlays_ << scene_.addEllipse(x-r, y-r, 2*r, 2*r, circlePen);
        auto* centerMarker = scene_.addPath(crossPath, centerPen);
        centerMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        centerMarker->setPos(x, y);
        overlays_ << centerMarker;
        auto* label = scene_.addText(QStringLiteral("#%1").arg(++index));
        label->setDefaultTextColor(QColor(255, 80, 40));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setPos(x + r + crossSize, y - crossSize);
        overlays_ << label;
    }
}

void ImageView::fitImage()
{
    autoFit_ = true;
    if (!image_.isNull()) fitInView(QRectF(image_.rect()), Qt::KeepAspectRatio);
}

void ImageView::wheelEvent(QWheelEvent* event)
{
    if (image_.isNull()) return;
    const double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
    const double next = transform().m11() * factor;
    if (next > 0.01 && next < 30.0) {
        autoFit_ = false;
        scale(factor, factor);
    }
    event->accept();
}

void ImageView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (autoFit_) fitImage();
}
