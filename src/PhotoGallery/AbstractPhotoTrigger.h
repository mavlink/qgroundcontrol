/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>

/// A "photo" operation in progress
///
/// Represents a photo being taken. This object exists while this is in
/// progress and can be used by callers to track operation.
class AbstractPhotoTriggerOperation : public QObject {
    Q_OBJECT
public:
    ~AbstractPhotoTriggerOperation() override;

    virtual bool finished() const = 0;
    /// Only meaningful in finished.
    virtual bool success() const = 0;
    /// Id of the photo, if operation succeeded.
    virtual QString id() const = 0;

signals:
    /// Signal emitted when the operation has finished.
    ///
    /// This object is alive until after emitting this signal has finished, it
    /// must not be referenced after delivery of this signal has completed.
    void finish();
};

/// Abstract interface to trigger taking a photo.
class AbstractPhotoTrigger : public QObject {
    Q_OBJECT
public:
    ~AbstractPhotoTrigger() override;

    explicit AbstractPhotoTrigger(QObject * parent = nullptr);

    /// Take a photo.
    ///
    /// \returns Operation in progress or nullptr.
    ///
    /// Starts taking a photo. If no photo can presently be taken at all
    /// (e.g. no camera) then this may return nullptr to indicate failure.
    /// Otherwise, returns a continuation object to represent the operation in
    /// progress.
    ///
    /// The continuation object is alive for as long as the operation is in
    /// progress, up and until its "finish" signal has been emitted.
    ///
    /// This method should also ensure that the operation cannot delay
    /// "indefinitely" (but e.g. instead cancel after some timeout).
    virtual AbstractPhotoTriggerOperation * takePhoto() = 0;
};
