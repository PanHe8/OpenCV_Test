#include "ImageView.h"
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPen>
#include <QFont>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QStringList>
#include <cmath>

ImageView::ImageView(QWidget* parent) : QGraphicsView(parent), scene_(this)
{
    setScene(&scene_);
    setBackgroundBrush(QColor(35, 39, 45));
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMinimumSize(420, 360);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setToolTip(QStringLiteral("右键打开图像菜单；滚轮缩放图像"));
}

void ImageView::setImage(const QImage& image)
{
    overlays_.clear();
    scene_.clear();
    circles_.clear();
    mouseInViewport_ = false;
    image_ = image;
    if (!image.isNull()) scene_.addPixmap(QPixmap::fromImage(image));
    scene_.setSceneRect(QRectF(image.rect()));
    fitImage();
    viewport()->update();
}

void ImageView::setCircles(const std::vector<vision::Circle>& circles)
{
    circles_ = circles;
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
    viewport()->update();
}

void ImageView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void ImageView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.setObjectName(QStringLiteral("imageContextMenu"));
    auto* fitAction = menu.addAction(QStringLiteral("自适应图像"));
    fitAction->setObjectName(QStringLiteral("fitImageAction"));
    fitAction->setEnabled(!image_.isNull());
    connect(fitAction, &QAction::triggered, this, [this] { fitImage(); });

    auto* moveAction = menu.addAction(QStringLiteral("移动图像"));
    moveAction->setObjectName(QStringLiteral("moveImageAction"));
    moveAction->setCheckable(true);
    moveAction->setChecked(dragMode() == QGraphicsView::ScrollHandDrag);
    moveAction->setEnabled(!image_.isNull());
    connect(moveAction, &QAction::toggled, this, [this](bool checked) {
        setDragMode(checked ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
    });

    auto* centerAction = menu.addAction(QStringLiteral("显示中心线"));
    centerAction->setObjectName(QStringLiteral("showCenterAction"));
    centerAction->setCheckable(true);
    centerAction->setChecked(fieldCenterVisible_);
    centerAction->setEnabled(!image_.isNull());
    connect(centerAction, &QAction::toggled, this, [this](bool checked) {
        fieldCenterVisible_ = checked;
        viewport()->update();
    });

    auto* coordinatesAction = menu.addAction(QStringLiteral("显示像素坐标"));
    coordinatesAction->setObjectName(QStringLiteral("showCoordinatesAction"));
    coordinatesAction->setCheckable(true);
    coordinatesAction->setChecked(pixelCoordinatesVisible_);
    coordinatesAction->setEnabled(!image_.isNull());
    connect(coordinatesAction, &QAction::toggled, this, [this](bool checked) {
        pixelCoordinatesVisible_ = checked;
        viewport()->update();
    });

    menu.exec(event->globalPos());
    event->accept();
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    mousePosition_ = event->pos();
    mouseInViewport_ = true;
    QGraphicsView::mouseMoveEvent(event);
    if (pixelCoordinatesVisible_) viewport()->update();
}

bool ImageView::viewportEvent(QEvent* event)
{
    if (event->type() == QEvent::Leave) {
        mouseInViewport_ = false;
        if (pixelCoordinatesVisible_) viewport()->update();
    }
    return QGraphicsView::viewportEvent(event);
}

void ImageView::drawForeground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawForeground(painter, rect);
    if (!fieldCenterVisible_ || image_.isNull()) return;

    // 使用图片坐标，中心与缩放、拖动无关；线宽保持为一个界面像素。
    const QPointF center(image_.width() / 2.0, image_.height() / 2.0);
    painter->save();
    QPen pen(QColor(0, 210, 255), 1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawLine(QPointF(0, center.y()), QPointF(image_.width(), center.y()));
    painter->drawLine(QPointF(center.x(), 0), QPointF(center.x(), image_.height()));
    painter->restore();
}

void ImageView::paintEvent(QPaintEvent* event)
{
    QGraphicsView::paintEvent(event);
    if (image_.isNull()) return;

    QStringList lines;
    if (pixelCoordinatesVisible_) {
        // 从视口反算原图坐标，缩放、拖动后仍显示实际像素位置。
        const QPointF pixel = mapToScene(mousePosition_);
        if (mouseInViewport_ && pixel.x() >= 0 && pixel.x() < image_.width()
            && pixel.y() >= 0 && pixel.y() < image_.height()) {
            lines << QStringLiteral("像素坐标：X=%1，Y=%2（左上角为原点）")
                .arg(static_cast<int>(std::floor(pixel.x())))
                .arg(static_cast<int>(std::floor(pixel.y())));
        } else {
            lines << QStringLiteral("像素坐标：鼠标未在图像内");
        }
    }
    if (fieldCenterVisible_) {
        const QPointF center(image_.width() / 2.0, image_.height() / 2.0);
        lines << QStringLiteral("视场中心：X=%1，Y=%2 像素")
            .arg(center.x(), 0, 'f', 2).arg(center.y(), 0, 'f', 2);
        lines << QStringLiteral("偏差 = 圆心 - 视场中心（X 向右为正，Y 向下为正）");
        int index = 0;
        for (const auto& circle : circles_) {
            lines << QStringLiteral("#%1 圆心：X=%2，Y=%3；ΔX=%4，ΔY=%5 像素")
                .arg(++index).arg(circle.center.x, 0, 'f', 2).arg(circle.center.y, 0, 'f', 2)
                .arg(circle.center.x - center.x(), 0, 'f', 2)
                .arg(circle.center.y - center.y(), 0, 'f', 2);
        }
        if (circles_.empty()) lines << QStringLiteral("尚无圆检测结果");
    }
    if (lines.isEmpty()) return;
    const QString text = lines.join(QLatin1Char('\n'));

    // 在视口上绘制，使文字固定在左上角，不随图片缩放和平移。
    QPainter painter(viewport());
    painter.setFont(font());
    const QRect available = viewport()->rect().adjusted(10, 10, -10, -10);
    if (available.isEmpty()) return;
    const int flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap;
    const QRect content = available.adjusted(8, 6, -8, -6);
    const QRect bounds = painter.boundingRect(content, flags, text);
    painter.fillRect(bounds.adjusted(-8, -6, 8, 6), QColor(20, 25, 30, 215));
    painter.setPen(QColor(0, 230, 255));
    painter.drawText(content, flags, text);
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
