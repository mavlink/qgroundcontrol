/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PhotoGalleryView.h"

#include <QPainter>
#include <QtQml>

#include <cmath>

#include "AbstractPhotoTrigger.h"

namespace {

void interpolateTransform(QTransform & t, double z)
{
    t.setMatrix(
        t.m11() * z + 1 - z, t.m12() * z, t.m13() * z,
        t.m21() * z, t.m22() * z + 1 - z, t.m23() * z,
        t.m31() * z, t.m32() * z, t.m33() * z + 1 - z);
}

QRectF trashCanBounds(const QSizeF & view_bounds)
{
    double scale = std::min(view_bounds.width(), view_bounds.height()) * 0.1;
    return QRectF(scale * 0.4, view_bounds.height() - scale * 1.4, scale, scale);
}

void drawTrashCan(QPainter * painter, const QRectF & bounds)
{
    double w = bounds.width();
    double h = bounds.height();

    QPainterPath path;
    path.moveTo(0.00 * w, 0.05 * h);
    path.lineTo(0.45 * w, 0.05 * h);
    path.lineTo(0.45 * w, 0.00 * h);
    path.lineTo(0.55 * w, 0.00 * h);
    path.lineTo(0.55 * w, 0.05 * h);
    path.lineTo(1.00 * w, 0.05 * h);
    path.lineTo(1.00 * w, 0.20 * h);
    path.lineTo(0.00 * w, 0.20 * h);
    path.closeSubpath();
    path.moveTo(0.00 * w, 0.25 * h);
    path.lineTo(1.00 * w, 0.25 * h);
    path.lineTo(1.00 * w, 1.00 * h);
    path.lineTo(0.00 * w, 1.00 * h);
    path.closeSubpath();
    path.translate(bounds.left(), bounds.top());
    painter->fillPath(path, Qt::white);
    painter->setPen(QPen(Qt::black, w * 0.025, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawPath(path);
}

QRectF exitZoomBounds(const QSizeF & view_bounds)
{
    double scale = std::min(view_bounds.width(), view_bounds.height()) * 0.1;
    return QRectF(scale * 0.4, scale * 0.4, scale, scale);
}

void drawExitZoom(QPainter * painter, const QRectF & bounds)
{
    double w = bounds.width();
    double h = bounds.height();

    QPainterPath path;
    path.moveTo(0.1 * w, 0.0 * h);
    path.lineTo(0.5 * w, 0.4 * h);
    path.lineTo(0.9 * w, 0.0 * h);
    path.lineTo(1.0 * w, 0.1 * h);
    path.lineTo(0.6 * w, 0.5 * h);
    path.lineTo(1.0 * w, 0.9 * h);
    path.lineTo(0.9 * w, 1.0 * h);
    path.lineTo(0.5 * w, 0.6 * h);
    path.lineTo(0.1 * w, 1.0 * h);
    path.lineTo(0.0 * w, 0.9 * h);
    path.lineTo(0.4 * w, 0.5 * h);
    path.lineTo(0.0 * w, 0.1 * h);
    path.closeSubpath();
    path.translate(bounds.left(), bounds.top());
    painter->fillPath(path, Qt::white);
    painter->setPen(QPen(Qt::black, w * 0.025, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawPath(path);
}

QRectF cameraTriggerBounds(const QSizeF & view_bounds)
{
    double scale = std::min(view_bounds.width(), view_bounds.height()) * 0.1;
    return QRectF(view_bounds.width() - scale * 1.4, view_bounds.height() - scale * 1.4, scale, scale);
}

void drawCameraTrigger(QPainter * painter, const QRectF & bounds)
{
    double w = bounds.width();
    double h = bounds.height();

    QPainterPath path;
    path.moveTo(0.00 * w, 0.20 * h);
    path.lineTo(0.40 * w, 0.20 * h);
    path.lineTo(0.45 * w, 0.15 * h);
    path.lineTo(0.55 * w, 0.15 * h);
    path.lineTo(0.60 * w, 0.20 * h);
    path.lineTo(1.00 * w, 0.20 * h);
    path.lineTo(1.00 * w, 0.80 * h);
    path.lineTo(0.00 * w, 0.80 * h);
    path.closeSubpath();
    path.addEllipse(QRectF(0.4 * w, 0.4 * h, 0.2 * w, 0.2 * h));
    path.translate(bounds.left(), bounds.top());
    painter->fillPath(path, Qt::white);
    painter->setPen(QPen(Qt::black, w * 0.025, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawPath(path);
}

}  // namespace

class PhotoGalleryView::InteractionState {
public:
    virtual ~InteractionState() {}

    class Pan;
    class Zoom;
};

class PhotoGalleryView::InteractionState::Pan final : public PhotoGalleryView::InteractionState {
public:
    virtual ~Pan() {}

    /// Start location
    ///
    /// Start of pan in screen coordinates (i.e. the point on screen where mouse
    /// started dragging / finger started panning).
    QPointF start;

    /// Start time
    std::chrono::system_clock::time_point start_time;

    /// Current location
    ///
    /// Current pan position (i.e. where mouse/finger moved to) in screen
    /// coordinates.
    QPointF current;

    /// Cumulative distance travelled.
    double distance_travelled = 0.0;
};

class PhotoGalleryView::InteractionState::Zoom final : public PhotoGalleryView::InteractionState {
public:
    virtual ~Zoom() {}

    /// Zoom center point
    ///
    /// The center point of the zoom (i.e. the center point of the finger pinch
    /// or the center point of where mouse scroll started).
    QPointF center;

    /// Change in zoom
    double delta = 0.0;
};

PhotoGalleryView::PhotoGalleryView(QQuickItem * parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    _zoom_timeout.setSingleShot(true);
    connect(&_zoom_timeout, &QTimer::timeout, this, &PhotoGalleryView::handleZoomTimeout);
    _long_click_timeout.setSingleShot(true);
    connect(&_long_click_timeout, &QTimer::timeout, this, &PhotoGalleryView::handleLongClickTimeout);

    _view_state.mode = ViewMode::Gallery;
    _view_state.gallery.offset = 0.0;
    _view_state.gallery.columns = 4;
}

PhotoGalleryView::~PhotoGalleryView()
{
}

void PhotoGalleryView::paintGallery(QPainter * painter, const QSizeF & bounds, const GalleryViewState & view) const
{
    int num_photos = _model->numPhotos();

    QSizeF cell_dims = computeCellSize(bounds, view.columns);

    int start_row = std::floor(view.offset);
    int start_index = start_row * view.columns;
    int end_index = start_index + std::ceil(bounds.height() / cell_dims.height() + 1) * view.columns;

    bool any_selected = !_selected_images.empty();

    int n = start_index;
    while (n < num_photos && n < end_index) {
        const auto & item = _model->data(PhotoGalleryModelIndex(n));
        if (item.image) {
            const auto & image = *item.image;
            QRectF dst = imageIndexToScreen(view, bounds, n);
            paintImageThumbnail(painter, image, dst,
                any_selected ? (_selected_images.find(item.id) != _selected_images.end() ? SelectionState::Selected : SelectionState::Deselected) : SelectionState::None
            );
        }
        ++n;
    }
}

void PhotoGalleryView::paintSingle(QPainter * painter, const QSizeF & bounds, const SingleViewState & view) const
{
    const auto & item = _model->data(PhotoGalleryModelIndex(view.index));
    if (!item.image) {
        return;
    }
    const auto & image = *item.image;
    QRect img_bounds = image.rect();
    double img_aspect = static_cast<double>(img_bounds.width()) / img_bounds.height();
    double view_aspect = bounds.width() / bounds.height();
    double base_scale = (img_aspect > view_aspect) ? (img_bounds.width() / bounds.width()) : (img_bounds.height() / bounds.height());

    QRectF src(0, 0, image.width(), image.height());
    QRectF dst(bounds.width() / 2 - image.width() / base_scale / 2,  bounds.height() / 2 - image.height() / base_scale / 2, image.width() / base_scale, image.height() / base_scale);
    painter->drawImage(dst, image, src);
}

void PhotoGalleryView::paint(QPainter * painter, const QSizeF & bounds, const ViewState & view) const
{
    switch (view.mode) {
        case ViewMode::Gallery: {
            paintGallery(painter, bounds, view.gallery);
            break;
        }
        case ViewMode::Single: {
            paintSingle(painter, bounds, view.single);
            break;
        }
    }
}

void PhotoGalleryView::paintIcons(QPainter * painter, const QSizeF & bounds, const ViewState & view) const
{
    switch (view.mode) {
        case ViewMode::Gallery: {
            if (!_selected_images.empty()) {
                drawTrashCan(painter, trashCanBounds(bounds));
            }
            drawCameraTrigger(painter, cameraTriggerBounds(bounds));
            break;
        }
        case ViewMode::Single: {
            drawExitZoom(painter, exitZoomBounds(bounds));
            break;
        }
    }
}

void PhotoGalleryView::paintImageThumbnail(
    QPainter * painter, const QImage & image, const QRectF & dst_bounds, SelectionState selection_state) const
{
    QRect img_bounds = image.rect();
    // Figure out how to scale image into the view bounds.
    double img_aspect = static_cast<double>(img_bounds.width()) / img_bounds.height();
    double dst_aspect = dst_bounds.width() / dst_bounds.height();

    double scale = (img_aspect > dst_aspect) ? (dst_bounds.height() / img_bounds.height()) : (dst_bounds.width() / img_bounds.width());

    double crop_w = img_bounds.width() - dst_bounds.width() / scale;
    double crop_h = img_bounds.height() - dst_bounds.height() / scale;
    QRectF src(crop_w * 0.5, crop_h * 0.5, img_bounds.width() - crop_w, img_bounds.height() - crop_h);
    painter->drawImage(dst_bounds, image, src);

    if (selection_state == SelectionState::None) {
        return;
    }

    auto box_scale = std::min(dst_bounds.width(), dst_bounds.height()) * 0.08;

    QRectF select_box(dst_bounds.right() - 4 * box_scale,
                     dst_bounds.top() + 1 * box_scale,
                     3 * box_scale,
                     3 * box_scale);
    if (selection_state == SelectionState::Selected) {
        painter->fillRect(select_box, Qt::green);
    } else {
        painter->fillRect(select_box, Qt::black);
    }
    painter->setPen(QPen(Qt::white, box_scale * 0.4, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawRect(select_box);
}

void PhotoGalleryView::paint(QPainter * painter)
{
    QRectF rect(0, 0, width(), height());
    painter->fillRect(rect, QColor("black"));
    if (!_model) {
        return;
    }

    if (!_interaction_state) {
        paint(painter, rect.size(), _view_state);
        paintIcons(painter, rect.size(), _view_state);
    } else {
        ViewState current_view_state = _view_state;
        ViewState floating_view_state = _view_state;
        applyInteraction(_interaction_state.get(), current_view_state, InteractionApplyMode::FloatingBase);
        applyInteraction(_interaction_state.get(), floating_view_state, InteractionApplyMode::FloatingOverlay);

        if (!needInterpolation(current_view_state, floating_view_state)) {
            paint(painter, rect.size(), current_view_state);
            paintIcons(painter, rect.size(), current_view_state);
        } else {
            double z = computeInterpolationPosition(_interaction_state.get());
            QTransform t1;
            QTransform t2;
            std::tie(t1, t2) = computeInterpolationTransforms(rect.size(), current_view_state, floating_view_state, _interaction_state.get());

            interpolateTransform(t1, z);
            interpolateTransform(t2, 1 - z);

            QImage tmp(width(), height(), QImage::Format_ARGB32);
            QPainter tmp_p(&tmp);

            tmp_p.setCompositionMode(QPainter::CompositionMode_Source);
            tmp_p.fillRect(0, 0, width(), height(), QColor(Qt::transparent));
            paint(&tmp_p, rect.size(), current_view_state);
            painter->setOpacity(1 - z);
            painter->setTransform(t1);
            painter->drawImage(0, 0, tmp);

            tmp_p.fillRect(0, 0, width(), height(), QColor(Qt::transparent));
            paint(&tmp_p, rect.size(), floating_view_state);
            painter->setOpacity(z);
            painter->setTransform(t2);
            painter->drawImage(0, 0, tmp);
        }
    }
}

void PhotoGalleryView::setModel(PhotoGalleryModel * model)
{
    _model = model;
    connect(model, &PhotoGalleryModel::added, this, &PhotoGalleryView::modelAdded);
    connect(model, &PhotoGalleryModel::removed, this, &PhotoGalleryView::modelRemoved);
}

PhotoGalleryModel * PhotoGalleryView::model() const
{
    return _model;
}

AbstractPhotoTrigger * PhotoGalleryView::trigger() const
{
    return _trigger;
}

void PhotoGalleryView::setTrigger(AbstractPhotoTrigger * trigger)
{
    _trigger = trigger;
}

void PhotoGalleryView::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();

        commitInteraction();

        std::unique_ptr<InteractionState::Pan> pan(new InteractionState::Pan());
        pan->start = QPointF(event->x(), event->y());
        pan->start_time = std::chrono::system_clock::now();
        pan->current = pan->start;
        _interaction_state = std::move(pan);
        _long_click_timeout.start(_parameters.long_click_timeout / std::chrono::milliseconds(1));
    }
}

void PhotoGalleryView::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();
        _long_click_timeout.stop();

        auto * pan = dynamic_cast<InteractionState::Pan *>(_interaction_state.get());
        if (pan) {
            double ds = pan->distance_travelled;
            commitInteraction();
            if (ds < _parameters.click_slip) {
                handleShortClick(QPointF(event->x(), event->y()));
            }
        }
    }
}

void PhotoGalleryView::mouseMoveEvent(QMouseEvent * event)
{
    auto * pan = dynamic_cast<InteractionState::Pan *>(_interaction_state.get());
    if (pan) {
        event->accept();

        pan->current = QPointF(event->x(), event->y());
        double dx = (pan->current.x() - pan->start.x());
        double dy = (pan->current.y() - pan->start.y());
        double ds = sqrt(dx * dx + dy * dy) / std::max(width(), height());
        pan->distance_travelled += ds;
        update();
    }
}

void PhotoGalleryView::wheelEvent(QWheelEvent *event)
{
    event->accept();

    InteractionState::Zoom * zoom = dynamic_cast<InteractionState::Zoom *>(_interaction_state.get());
    if (_interaction_state && !zoom) {
        commitInteraction();
        _interaction_state.reset();
    }
    if (!zoom) {
        zoom = new InteractionState::Zoom();
        _interaction_state.reset(zoom);
        zoom->center = QPointF(event->x(), event->y());
    }

    zoom->delta -= event->angleDelta().y() / (90. * 8.);
    update();
    _zoom_timeout.start(_parameters.zoom_timeout / std::chrono::milliseconds(1));
}

QSizeF PhotoGalleryView::computeCellSize(const QSizeF & bounds, int num_columns) const
{
    double w = bounds.width() / num_columns;
    return {w, w / _parameters.grid_aspect};
}

void PhotoGalleryView::handleZoomTimeout()
{
    if (dynamic_cast<InteractionState::Zoom *>(_interaction_state.get())) {
        commitInteraction();
    }
}

void PhotoGalleryView::handleLongClickTimeout()
{
    if (auto * pan = dynamic_cast<InteractionState::Pan *>(_interaction_state.get())) {
        double ds = pan->distance_travelled;
        QPointF where(pan->current);
        if (ds < _parameters.click_slip) {
            commitInteraction();
            handleLongClick(where);
        }
    }
}

void PhotoGalleryView::modelAdded(const std::set<PhotoGalleryModelIndex> & /*indices*/)
{
    update();
}

void PhotoGalleryView::modelRemoved(const std::set<PhotoGalleryModelIndex> & /*indices*/)
{
    update();
}


namespace {

/// Find stop in vector
///
/// Find place in vector where our current column configuration lives (or the
/// one closest to it, at least. Unless the input vector is empty (don't do
/// that!) this will always yield a valid iterator into the vector.
std::vector<int>::const_iterator findCurrentColumnStop(const std::vector<int> & stops, int ncolumns)
{
    auto i = stops.begin();
    for (auto j = stops.begin(); j != stops.end(); ++j) {
        if (*j <= ncolumns) {
            i = j;
        }
    }
    return i;
}

/// Find prev/next stop
///
/// Find upper/lower column configuration, from our current configuration. Unless
/// stops is empty (don't do that!) this either yields an iterator to the next
/// column configuration to use, or iterator to stops.end() to indicate that we
/// should switch to single image view mode.
std::vector<int>::const_iterator findNextColumnStop(const std::vector<int> & stops, int columns, int direction)
{
    auto i = findCurrentColumnStop(stops, columns);
    if (i == stops.end()) {
        return i;
    }

    if (direction == -1) {
        if (i != stops.begin()) {
            return std::prev(i);
        } else {
            return stops.end();
        }
    }
    if (direction == +1 && std::next(i) != stops.end()) {
        return std::next(i);
    }
    return i;
}

}

void PhotoGalleryView::applyInteraction(const InteractionState * interaction, ViewState & view, InteractionApplyMode mode)
{
    if (auto pan = dynamic_cast<const InteractionState::Pan *>(interaction)) {
        QSizeF bounds(width(), height());
        switch (view.mode) {
            case ViewMode::Gallery: {
                QSizeF cell_dims = computeCellSize(bounds, view.gallery.columns);
                double delta = (pan->start.y() - pan->current.y()) / cell_dims.height();
                view.gallery.offset = std::max(0.0, view.gallery.offset + delta);
                boundGalleryViewOffset(bounds, view.gallery);
                break;
            }
            case ViewMode::Single: {
                break;
            }
        }
    } else if (auto zoom = dynamic_cast<const InteractionState::Zoom *>(interaction)) {
        QSizeF bounds(width(), height());
        bool threshold =
            (mode == InteractionApplyMode::Normal && std::fabs(zoom->delta) > 0.5) ||
            (mode == InteractionApplyMode::FloatingOverlay && zoom->delta != 0.0);
        switch (view.mode) {
            case ViewMode::Gallery: {
                if (threshold) {
                    int index = screenToImageIndex(view.gallery, bounds, zoom->center);
                    auto i = findNextColumnStop(_parameters.column_stops, view.gallery.columns, zoom->delta > 0 ? +1 : -1);
                    if (i == (_parameters.column_stops.end())) {
                        if (index >= 0 && index < imageIndexLimit()) {
                            view.mode = ViewMode::Single;
                            view.single.index = index;
                        }
                    } else {
                        QRectF original = imageIndexToScreen(view.gallery, bounds, index);
                        view.gallery.columns = *i;
                        QRectF zoomed = imageIndexToScreen(view.gallery, bounds, index);
                        view.gallery.offset += (zoomed.y() - original.y()) / computeCellSize(bounds, view.gallery.columns).height();
                        boundGalleryViewOffset(bounds, view.gallery);
                    }
                }
                break;
            }
            case ViewMode::Single: {
                if (threshold && zoom->delta > 0) {
                    view.mode = ViewMode::Gallery;
                }
                break;
            }
        }
    }
}

void PhotoGalleryView::commitInteraction()
{
    applyInteraction(_interaction_state.get(), _view_state);
    update();
    _interaction_state.reset();
}

void PhotoGalleryView::handleLongClick(const QPointF & where)
{
    if (_view_state.mode == ViewMode::Gallery) {
        if (!_selected_images.empty() && trashCanBounds(size()).contains(where)) {
            if (_model) {
                _model->remove(_selected_images);
            }
            _selected_images.clear();
            return;
        }
        if (cameraTriggerBounds(size()).contains(where)) {
            if (_trigger) {
                _trigger->takePhoto();
            }
            return;
        }
        toggleSelectImageAt(_view_state.gallery, where);
    }
}

void PhotoGalleryView::handleShortClick(const QPointF & where)
{
    if (_view_state.mode == ViewMode::Gallery) {
        if (!_selected_images.empty() && trashCanBounds(size()).contains(where)) {
            return;
        }
        if (cameraTriggerBounds(size()).contains(where)) {
            if (_trigger) {
                _trigger->takePhoto();
            }
            return;
        }
        if (!_selected_images.empty()) {
            toggleSelectImageAt(_view_state.gallery, where);
        } else {
            magnifyImageAt(_view_state.gallery, where);
        }
    } else {
        if (exitZoomBounds(size()).contains(where)) {
            _view_state.mode = ViewMode::Gallery;
            update();
            return;
        }
    }
}

QRectF PhotoGalleryView::imageIndexToScreen(const GalleryViewState & view, const QSizeF & bounds, int index) const
{
    QSizeF cell_size = computeCellSize(bounds, view.columns);
    int x = index % view.columns;
    int y = index / view.columns;
    return QRectF(QPointF(x * cell_size.width(), (y - view.offset) * cell_size.height()), cell_size);
}

int PhotoGalleryView::screenToImageIndex(const GalleryViewState & view, const QSizeF & bounds, const QPointF & point) const
{
    QSizeF cell_size = computeCellSize(bounds, view.columns);
    int x = floor(point.x() / cell_size.width());
    int y = floor(point.y() / cell_size.height() + view.offset);
    return x + y * view.columns;
}

int PhotoGalleryView::imageIndexLimit() const
{
    return _model ? _model->numPhotos() : 0;
}

void PhotoGalleryView::boundGalleryViewOffset(const QSizeF & bounds, GalleryViewState & view) const
{
    QSizeF cell_size = computeCellSize(bounds, view.columns);
    int last_row = (imageIndexLimit() - 1) / view.columns + 1;
    double upper_limit = last_row - bounds.height() / cell_size.height();
    view.offset = std::max(0.0, std::min(upper_limit, view.offset));
}

bool PhotoGalleryView::needInterpolation(const ViewState & view_1, const ViewState & view_2)
{
    if (view_1.mode != view_2.mode) {
        return true;
    }
    if (view_1.mode == ViewMode::Gallery) {
        return view_1.gallery.columns != view_2.gallery.columns;
    }
    return false;
}

double PhotoGalleryView::computeInterpolationPosition(const InteractionState * interaction)
{
    if (auto zoom = dynamic_cast<const InteractionState::Zoom *>(interaction)) {
        return std::min(1.0, std::fabs(zoom->delta));
    }
    return 0.0;
}

std::pair<QTransform, QTransform>
PhotoGalleryView::computeInterpolationTransforms(const QSizeF & bounds, const ViewState & view_1, const ViewState & view_2, const InteractionState * interaction)
{
    QTransform t1;
    QTransform t2;
    QPointF center(0, 0);
    if (auto zoom = dynamic_cast<const InteractionState::Zoom *>(interaction)) {
        center = zoom->center;
    }
    if (view_1.mode == ViewMode::Gallery && view_2.mode == ViewMode::Gallery) {
        double scale = static_cast<double>(view_1.gallery.columns) / view_2.gallery.columns;
        int index = screenToImageIndex(view_1.gallery, bounds, center);
        double y1 = imageIndexToScreen(view_1.gallery, bounds, index).y();
        double y2 = imageIndexToScreen(view_2.gallery, bounds, index).y();
        t1.translate(0, y1);
        t1.scale(scale, scale);
        t1.translate(0, -y2);
        t2.translate(0, y2);
        t2.scale(1 / scale, 1 / scale);
        t2.translate(0, -y1);
    } else if (view_1.mode == ViewMode::Gallery && view_2.mode == ViewMode::Single) {
        double image_aspect_ratio = getImageAspectRatio(view_2.single.index);
        QRectF gallery_outline = thumbnailVirtualOutline(view_1.gallery, bounds, view_2.single.index, image_aspect_ratio);
        QRectF single_outline = singleOutline(bounds, image_aspect_ratio);

        t2.translate(gallery_outline.x(), gallery_outline.y());
        t2.scale(gallery_outline.width() / single_outline.width(), gallery_outline.height() / single_outline.height());
        t2.translate(-single_outline.x(), -single_outline.y());
    } else if (view_1.mode == ViewMode::Single && view_2.mode == ViewMode::Gallery) {
        double image_aspect_ratio = getImageAspectRatio(view_1.single.index);
        QRectF gallery_outline = thumbnailVirtualOutline(view_2.gallery, bounds, view_1.single.index, image_aspect_ratio);
        QRectF single_outline = singleOutline(bounds, image_aspect_ratio);

        t1.translate(gallery_outline.x(), gallery_outline.y());
        t1.scale(gallery_outline.width() / single_outline.width(), gallery_outline.height() / single_outline.height());
        t1.translate(-single_outline.x(), -single_outline.y());
    }

    return {t1, t2};
}

QRectF PhotoGalleryView::thumbnailVirtualOutline(const GalleryViewState & view, const QSizeF & bounds, int index, double image_aspect_ratio) const
{
    QRectF box = imageIndexToScreen(view, bounds, index);
    double thumbnail_aspect_ratio = box.width() / box.height();

    if (image_aspect_ratio > thumbnail_aspect_ratio) {
        double w = box.height() * image_aspect_ratio;
        box = QRectF(box.x() - 0.5 * (w - box.width()), box.y(), w, box.height());
    } else {
        double h = box.width() / image_aspect_ratio;
        box = QRectF(box.x(), box.y() - 0.5 * (h - box.height()), box.width(), h);
    }

    return box;
}

QRectF PhotoGalleryView::singleOutline(const QSizeF & bounds, double image_aspect_ratio) const
{
    double view_aspect_ratio = bounds.width() / bounds.height();
    if (view_aspect_ratio > image_aspect_ratio) {
        double w = bounds.height() * image_aspect_ratio;
        return QRectF(0.5 * (bounds.width() - w), 0, w, bounds.height());
    } else {
        double h = bounds.width() / image_aspect_ratio;
        return QRectF(0, 0.5 * (bounds.height() - h), bounds.width(), h);
    }
}

double PhotoGalleryView::getImageAspectRatio(int index) const
{
    const auto & item = _model->data(PhotoGalleryModelIndex(index));
    if (item.image) {
        const auto & image = *item.image;
        return static_cast<double>(image.rect().width()) / image.rect().height();
    } else {
        return 1.0;
    }
}

void PhotoGalleryView::toggleSelectImageAt(const GalleryViewState & view, const QPointF & point)
{
    int index = screenToImageIndex(view, size(), point);
    const auto & item = _model->data(PhotoGalleryModelIndex(index));
    if (!item.id.isEmpty()) {
        auto i = _selected_images.find(item.id);
        if (i == _selected_images.end()) {
            _selected_images.insert(item.id);
        } else {
            _selected_images.erase(item.id);
        }
        update();
    }
}

void PhotoGalleryView::magnifyImageAt(const GalleryViewState & view, const QPointF & point)
{
    int index = screenToImageIndex(view, size(), point);
    const auto & item = _model->data(PhotoGalleryModelIndex(index));
    if (!item.id.isEmpty()) {
        _view_state.mode = ViewMode::Single;
        _view_state.single.index = index;
        update();
    }
}

namespace {

void registerPhotoGalleryMetaType()
{
    qmlRegisterType<PhotoGalleryView>("QGroundControl.Controllers", 1, 0, "PhotoGalleryView");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoGalleryMetaType);
