/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <memory>

#include <QColor>
#include <QIcon>
#include <QQuickPaintedItem>
#include <QTimer>

#include "PhotoGalleryModel.h"

class AbstractPhotoTrigger;

/// Widget for image gallery.
class PhotoGalleryView : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(PhotoGalleryModel* model READ model WRITE setModel);
    Q_PROPERTY(AbstractPhotoTrigger* trigger READ trigger WRITE setTrigger);
public:
    /// Customizations of the widget
    struct Parameters {
        /// Grid cell aspect ration.
        ///
        /// Defines the aspect ratio of the cells shown in the image gallery
        /// grid. Android gallery uses 1.0 in minimum magnification, but maybe
        /// something close to "actual" image aspect makes more sense for better
        /// screen space utilization?
        double grid_aspect = 4.0/3.0;

        /// Columns shown in magnification.
        ///
        /// Defines the number of columns that we can show the gallery overview
        /// with.
        /// Requirements:
        /// - must be non-empty
        /// - first entry must be > 1
        /// - sorted in ascending order
        std::vector<int> column_stops = {2, 4, 6};

        /// Slip allowed to consider panning a "click" actually.
        ///
        /// Moving pointer by less than this amount (relative to widget size)
        /// is interpreted as being a "click".
        double click_slip = 0.02;

        /// Timeout to finish zoom operations.
        std::chrono::system_clock::duration zoom_timeout = std::chrono::milliseconds(250);

        /// Timeout to recognize long clicks.
        std::chrono::system_clock::duration long_click_timeout = std::chrono::milliseconds(500);
    };

    /// Current view mode (gallery or single image full screen).
    enum class ViewMode {
        /// Show gallery organized as grid
        Gallery = 0,
        /// Show single image full screen
        Single = 1
    };

    /// Detail state for "single" view mode.
    struct SingleViewState {
        /// Identify image currently shown
        int index;
    };

    /// Detail state for "gallery" view mode.
    struct GalleryViewState {
        /// Number of columns shown
        int columns;

        /// Vertical view offset
        ///
        /// The top edge of view as fractional row position in the image grid.
        /// If this is 0.0, then the top edge of the view is at the top edge
        /// of the first row (1.0 the second image etc.).
        /// If this is 0.5 then the top edge of the view as half-way in the
        /// first row etc.
        double offset;
    };

    /// Current visual state.
    ///
    /// This defines the complete visual state that is shown on screen, except
    /// for an "unstable" overlay view that is used to animate between different
    /// states.
    struct ViewState {
        /// View mode: gallery grid or single image
        ViewMode mode;

        /// Detail state for gallery view mode
        GalleryViewState gallery;

        /// Detail state for single image view mode
        SingleViewState single;
    };

    explicit PhotoGalleryView(QQuickItem *parent = nullptr);
    ~PhotoGalleryView() override;

    void setModel(PhotoGalleryModel * model);
    PhotoGalleryModel * model() const;

    AbstractPhotoTrigger * trigger() const;
    void setTrigger(AbstractPhotoTrigger * trigger);

    void paint(QPainter *painter) override;

    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    /// Handle timeout for zoom operation.
    void handleZoomTimeout();

    /// Handle timeout to detect very long clicks.
    void handleLongClickTimeout();

    void modelAdded(const std::set<PhotoGalleryModelIndex> & indices);
    void modelRemoved(const std::set<PhotoGalleryModelIndex> & indices);

private:
    /// Control how view changes should be applied
    enum class InteractionApplyMode {
        /// Change view based on user interaction state
        ///
        /// Apply (final) interaction state to view. This "snaps" to thresholds
        /// as needed.
        Normal = 0,
        /// Apply changes for base of floating view
        ///
        /// When making user interactions that show an intermediate state
        /// between present an (possible) target state, apply changes to form
        /// the "base" view.
        FloatingBase = 1,
        /// Apply changes for overlay of floating view
        ///
        /// When making user interactions that show an intermediate state
        /// between present an (possible) target state, apply changes to form
        /// the "overlay" view.
        FloatingOverlay = 2,
    };
    enum class SelectionState {
        Deselected = 0,
        Selected = 1,
        None = 2
    };
    class InteractionState;

    /// Paints gallery of images.
    void paintGallery(QPainter * painter, const QSizeF & bounds, const GalleryViewState & view) const;

    /// Paints single full-screen image.
    void paintSingle(QPainter * painter, const QSizeF & bounds, const SingleViewState & view) const;

    /// Paints view in given view mode (gallery or single).
    void paint(QPainter * painter, const QSizeF & bounds, const ViewState & view) const;

    /// Paints icons appropriate for current view mode.
    void paintIcons(QPainter * painter, const QSizeF & bounds, const ViewState & view) const;

    /// Paints "thumbnailed" version of image.
    ///
    /// Paint image thumbnail into the given target area. The image is cropped
    /// to fill the space in the target area (top/bottom or left/right depending
    /// on aspect ratio).
    void paintImageThumbnail(
        QPainter * painter, const QImage & image,
        const QRectF & dst_bounds, SelectionState selection_state) const;

    QSizeF computeCellSize(const QSizeF & bounds, int num_columns) const;

    void applyInteraction(
        const InteractionState * interaction,
        ViewState & view,
        InteractionApplyMode mode = InteractionApplyMode::Normal);

    /// Set new view state resulting from user interaction.
    ///
    /// Zoom/pan operations show an intermediate state of how the widget view
    /// is transformed. This snaps the actual view mode to the target view mode.
    void commitInteraction();

    void handleLongClick(const QPointF & where);

    void handleShortClick(const QPointF & where);

    /// Upper bound to valid image index
    int imageIndexLimit() const;
    void boundGalleryViewOffset(const QSizeF & bounds, GalleryViewState & view) const;

    /// Screen position of image
    ///
    /// Compute logical location on screen where picture of given index would
    /// appear.
    QRectF imageIndexToScreen(const GalleryViewState & view, const QSizeF & bounds, int index) const;
    int screenToImageIndex(const GalleryViewState & view, const QSizeF & bounds, const QPointF & point) const;

    static bool needInterpolation(const ViewState & view_1, const ViewState & view_2);
    static double computeInterpolationPosition(const InteractionState * interaction);

    std::pair<QTransform, QTransform>
    computeInterpolationTransforms(const QSizeF & bounds, const ViewState & view_1, const ViewState & view_2, const InteractionState * interaction);

    QRectF thumbnailVirtualOutline(const GalleryViewState & view, const QSizeF & bounds, int index, double image_aspect_ratio) const;

    QRectF singleOutline(const QSizeF & bounds, double image_aspect_ratio) const;

    double getImageAspectRatio(int index) const;

    void toggleSelectImageAt(const GalleryViewState & view, const QPointF & point);

    void magnifyImageAt(const GalleryViewState & view, const QPointF & point);

    Parameters _parameters;

    PhotoGalleryModel * _model = nullptr;

    AbstractPhotoTrigger * _trigger = nullptr;

    ViewState _view_state;

    /// On-going user interaction state
    ///
    /// This models the "unstable" state of a user interaction (e.g. dragging /
    /// panning / zooming in progress).
    std::unique_ptr<InteractionState> _interaction_state;

    /// Timeout for zoom interaction.
    ///
    /// This timer is used as a helper to terminate zoom interaction (we
    /// lack an event of "releasing scroll wheel").
    QTimer _zoom_timeout;

    /// Click timeout.
    ///
    /// This timer is used as a helper to distinguish "long click" and "pan"
    /// operations.
    QTimer _long_click_timeout;

    std::set<QString> _selected_images;
};
