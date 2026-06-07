// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHEADERVIEW_P_H
#define QHEADERVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qheaderview.h"
#include "private/qabstractitemview_p.h"

#include "QtCore/qbitarray.h"
#include "QtWidgets/qapplication.h"
#if QT_CONFIG(label)
#include "QtWidgets/qlabel.h"
#endif

#include <array>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

// Currently we support huge models with no memory per section if people are not resizing sections and not ordering sections.
// This enum is unlikely to be public later on. (Either we go for a huge model bool or another ("subset") enum not containing the initial mode).

enum HeaderMode
{
    InitialNoSectionMemoryUsage,         // Initial state - we don't use any memory per section until needed (needed on resize, swap, move, hide etc)
    FlexibleWithSectionMemoryUsage,      // user can hide, resize and reorder sections at the cost of memory usage.
};

class QHeaderViewPrivate: public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QHeaderView)

public:
    enum StateVersion { VersionMarker = 0xff };

    QHeaderViewPrivate()
        : state(NoState),
          headerOffset(0),
          sortIndicatorOrder(Qt::DescendingOrder),
          sortIndicatorSection(0),
          sortIndicatorShown(false),
          sortIndicatorClearable(false),
          lastPos(-1),
          firstPos(-1),
          originalSize(-1),
          section(-1),
          target(-1),
          firstPressed(-1),
          pressed(-1),
          hover(-1),
          length(0),
          preventCursorChangeInSetOffset(false),
          movableSections(false),
          clickableSections(false),
          highlightSelected(false),
          stretchLastSection(false),
          cascadingResizing(false),
          resizeRecursionBlock(false),
          allowUserMoveOfSection0(true), // will be false for QTreeView and true for QTableView
          customDefaultSectionSize(false),
          stretchSections(0),
          contentsSections(0),
          minimumSectionSize(-1),
          maximumSectionSize(-1),
          lastSectionSize(0),
          lastSectionLogicalIdx(-1), // Only trust when we stretch last section
          sectionIndicatorOffset(0),
#if QT_CONFIG(label)
          sectionIndicator(nullptr),
#endif
          globalResizeMode(QHeaderView::Interactive),
          sectionStartposRecalc(true),
          resizeContentsPrecision(1000)
    {}


    int lastVisibleVisualIndex() const;
    void restoreSizeOnPrevLastSection();
    void setNewLastSection(int visualIndexForLastSection);
    void maybeRestorePrevLastSectionAndStretchLast();
    int sectionHandleAt(int position);
    void setupSectionIndicator(int section, int position);
    void updateSectionIndicator(int section, int position);
    void updateHiddenSections(int logicalFirst, int logicalLast);
    void resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode = false);
    void sectionsRemoved(const QModelIndex &,int,int);
    void sectionsAboutToBeMoved(const QModelIndex &sourceParent, int logicalStart,
                                int logicalEnd, const QModelIndex &destinationParent,
                                int logicalDestination);
    void sectionsMoved(const QModelIndex &sourceParent, int logicalStart,
                       int logicalEnd, const QModelIndex &destinationParent,
                       int logicalDestination);
    void sectionsAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                                  QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void sectionsChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(),
                         QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

    bool isSectionSelected(int section) const;
    bool isFirstVisibleSection(int section) const;
    bool isLastVisibleSection(int section) const;

    inline bool rowIntersectsSelection(int row) const {
        return (selectionModel ? selectionModel->rowIntersectsSelection(row, root) : false);
    }

    inline bool columnIntersectsSelection(int column) const {
        return (selectionModel ? selectionModel->columnIntersectsSelection(column, root) : false);
    }

    inline bool sectionIntersectsSelection(int logical) const {
        return (orientation == Qt::Horizontal ? columnIntersectsSelection(logical) : rowIntersectsSelection(logical));
    }

    inline bool isRowSelected(int row) const {
        return (selectionModel ? selectionModel->isRowSelected(row, root) : false);
    }

    inline bool isColumnSelected(int column) const {
        return (selectionModel ? selectionModel->isColumnSelected(column, root) : false);
    }

    inline void prepareSectionSelected() {
        if (!selectionModel || !selectionModel->hasSelection())
            sectionSelected.clear();
        else if (sectionSelected.size() != sectionCount() * 2)
            sectionSelected.fill(false, sectionCount() * 2);
        else sectionSelected.fill(false);
    }

    inline int sectionCount() const {
        return noSectionMemoryUsage() ? countInNoSectionItemsMode : sectionItems.size();
    }

    inline bool reverse() const {
        return orientation == Qt::Horizontal && q_func()->isRightToLeft();
    }

    inline int logicalIndex(int visualIndex) const {
        return logicalIndices.isEmpty() ? visualIndex : logicalIndices.at(visualIndex);
    }

    inline int visualIndex(int logicalIndex) const {
        return visualIndices.isEmpty() ? logicalIndex : visualIndices.at(logicalIndex);
    }

    inline void setDefaultValues(Qt::Orientation o) {
        orientation = o;
        updateDefaultSectionSizeFromStyle();
        defaultAlignment = (o == Qt::Horizontal
                            ? Qt::Alignment(Qt::AlignCenter)
                            : Qt::AlignLeft|Qt::AlignVCenter);
    }

    inline bool isVisualIndexHidden(int visual) const {
        return !noSectionMemoryUsage() && sectionItems.at(visual).isHidden;
    }

    inline void setVisualIndexHidden(int visual, bool hidden) {
        sectionItems[visual].isHidden = hidden;
    }

    inline bool hasAutoResizeSections() const {
        return stretchSections || stretchLastSection || contentsSections;
    }

    QStyleOptionHeader getStyleOption() const;

    inline void invalidateCachedSizeHint() const {
        cachedSizeHint = QSize();
    }

    inline void initializeIndexMapping() const {
        if (visualIndices.size() != sectionCount()
            || logicalIndices.size() != sectionCount()) {
            visualIndices.resize(sectionCount());
            logicalIndices.resize(sectionCount());
            for (int s = 0; s < sectionCount(); ++s) {
                visualIndices[s] = s;
                logicalIndices[s] = s;
            }
        }
    }

    inline void clearCascadingSections() {
        firstCascadingSection = sectionItems.size();
        lastCascadingSection = 0;
        cascadingSectionSize.clear();
    }

    inline void saveCascadingSectionSize(int visual, int size) {
        if (!cascadingSectionSize.contains(visual)) {
            cascadingSectionSize.insert(visual, size);
            firstCascadingSection = qMin(firstCascadingSection, visual);
            lastCascadingSection = qMax(lastCascadingSection, visual);
        }
    }

    inline bool sectionIsCascadable(int visual) const {
        return headerSectionResizeMode(visual) == QHeaderView::Interactive;
    }

    inline int modelSectionCount() const {
        return (orientation == Qt::Horizontal
                ? model->columnCount(root)
                : model->rowCount(root));
    }

    inline void doDelayedResizeSections() {
        if (!delayedResize.isActive())
            delayedResize.start(0, q_func());
    }

    inline void executePostedResize() const {
        if (delayedResize.isActive() && state == NoState) {
            const_cast<QHeaderView*>(q_func())->resizeSections();
        }
    }

    inline void disconnectModel()
    {
        for (const QMetaObject::Connection &connection : modelConnections)
            QObject::disconnect(connection);
    }

    void clear();
    void flipSortIndicator(int section);
    Qt::SortOrder defaultSortOrderForSection(int section) const;
    void cascadingResize(int visual, int newSize);

    enum State { NoState, ResizeSection, MoveSection, SelectSections, NoClear } state;

    int headerOffset;
    Qt::Orientation orientation;
    Qt::SortOrder sortIndicatorOrder;
    int sortIndicatorSection;
    bool sortIndicatorShown;
    bool sortIndicatorClearable;

    mutable QList<int> visualIndices; // visualIndex = visualIndices.at(logicalIndex)
    mutable QList<int> logicalIndices; // logicalIndex = row or column in the model
    mutable QBitArray sectionSelected; // from logical index to bit
    mutable QHash<int, int> hiddenSectionSize; // from logical index to section size
    mutable QHash<int, int> cascadingSectionSize; // from visual index to section size
    mutable QSize cachedSizeHint;
    mutable QBasicTimer delayedResize;

    int firstCascadingSection;
    int lastCascadingSection;

    int lastPos;
    int firstPos;
    int originalSize;
    int section; // used for resizing and moving sections
    int target;
    int firstPressed;
    int pressed;
    int hover;

    int length;
    bool preventCursorChangeInSetOffset;
    bool movableSections;
    bool clickableSections;
    bool highlightSelected;
    bool stretchLastSection;
    bool cascadingResizing;
    bool resizeRecursionBlock;
    bool allowUserMoveOfSection0;
    bool customDefaultSectionSize;
    int stretchSections;
    int contentsSections;
    int defaultSectionSize;
    int oldDefaultSectionSize = -1;
    int minimumSectionSize;
    int maximumSectionSize;
    int lastSectionSize;
    int lastSectionLogicalIdx; // Only trust if we stretch LastSection
    int sectionIndicatorOffset;
    Qt::Alignment defaultAlignment;
#if QT_CONFIG(label)
    QLabel *sectionIndicator;
#endif
    QHeaderView::ResizeMode globalResizeMode;
    mutable bool sectionStartposRecalc;
    int resizeContentsPrecision;
    // header sections

    struct SectionItem {
        uint size : 20;
        uint isHidden : 1;
        uint resizeMode : 5;  // (holding QHeaderView::ResizeMode)
        uint currentlyUnusedPadding : 6;

        union { // This union is made in order to save space and ensure good vector performance (on remove)
            mutable int calculated_startpos; // <- this is the primary used member.
            mutable int tmpLogIdx;         // When one of these 'tmp'-members has been used we call
            int tmpDataStreamSectionCount; // recalcSectionStartPos() or set sectionStartposRecalc to true
        };                                 // to ensure that calculated_startpos will be calculated afterwards.

        inline SectionItem() : size(0), isHidden(0), resizeMode(QHeaderView::Interactive) {}
        inline SectionItem(int length, QHeaderView::ResizeMode mode)
            : size(length), isHidden(0), resizeMode(mode), calculated_startpos(-1) {}
        inline int sectionSize() const { return size; }
        inline int calculatedEndPos() const { return calculated_startpos + size; }
#ifndef QT_NO_DATASTREAM
        inline void write(QDataStream &out) const
        { out << static_cast<int>(size); out << 1; out << (int)resizeMode; }
        inline void read(QDataStream &in)
        { int m; in >> m; size = m; in >> tmpDataStreamSectionCount; in >> m; resizeMode = m; }
#endif
    };

    QList<SectionItem> sectionItems;

    HeaderMode headerMode = HeaderMode::InitialNoSectionMemoryUsage;
    qsizetype countInNoSectionItemsMode = 0;
    inline bool noSectionMemoryUsage() const
    {
        return (headerMode == HeaderMode::InitialNoSectionMemoryUsage);
    }

    inline void switchToFlexibleModeWithSectionMemoryUsage()
    {
        setHeaderMode(HeaderMode::FlexibleWithSectionMemoryUsage);
    }

    void updateCountInNoSectionItemsMode(int newCount);
    void setHeaderMode(HeaderMode mode);

    struct LayoutChangeItem {
        QPersistentModelIndex index;
        SectionItem section;
    };
    QList<LayoutChangeItem> layoutChangePersistentSections;
    std::array<QMetaObject::Connection, 8> modelConnections;

    void createSectionItems(int start, int end, int sectionSize, QHeaderView::ResizeMode mode);
    void removeSectionsFromSectionItems(int start, int end);
    void resizeSectionItem(int visualIndex, int oldSize, int newSize);
    void setDefaultSectionSize(int size);
    void updateDefaultSectionSizeFromStyle();
    void recalcSectionStartPos() const; // not really const

    inline int headerLength() const { // for debugging
        int len = 0;
        for (const auto &section : sectionItems)
            len += section.size;
        return len;
    }

    QBitArray sectionsHiddenToBitVector() const
    {
        QBitArray sectionHidden;
        if (!hiddenSectionSize.isEmpty()) {
            sectionHidden.resize(sectionItems.size());
            for (int u = 0; u < sectionItems.size(); ++u)
                sectionHidden[u] = sectionItems.at(u).isHidden;
        }
        return sectionHidden;
    }

    void setHiddenSectionsFromBitVector(const QBitArray &sectionHidden) {
        SectionItem *sectionData = sectionItems.data();
        for (int i = 0; i < sectionHidden.size(); ++i)
            sectionData[i].isHidden = sectionHidden.at(i);
    }

    int headerSectionSize(int visual) const;
    int headerSectionPosition(int visual) const;
    int headerVisualIndexAt(int position) const;

    // resize mode
    void setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode);
    QHeaderView::ResizeMode headerSectionResizeMode(int visual) const;
    void setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode);

    // other
    int viewSectionSizeHint(int logical) const;
    int adjustedVisualIndex(int visualIndex) const;
    void setScrollOffset(const QScrollBar *scrollBar, QAbstractItemView::ScrollMode scrollMode);
    void updateSectionsBeforeAfter(int logical);

#ifndef QT_NO_DATASTREAM
    void write(QDataStream &out) const;
    bool read(QDataStream &in);
#endif

};
Q_DECLARE_TYPEINFO(QHeaderViewPrivate::SectionItem, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QHeaderViewPrivate::LayoutChangeItem, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QHEADERVIEW_P_H
