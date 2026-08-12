/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Cinematic Startup Controller
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

namespace Volador {

class VoladorStartupController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isReady READ isReady WRITE setIsReady NOTIFY isReadyChanged)
    Q_PROPERTY(bool isCompleted READ isCompleted WRITE setIsCompleted NOTIFY isCompletedChanged)
    Q_PROPERTY(double progress READ progress WRITE setProgress NOTIFY progressChanged)

public:
    explicit VoladorStartupController(QObject *parent = nullptr);
    ~VoladorStartupController() override = default;

    bool isReady() const { return _isReady; }
    void setIsReady(bool ready);

    bool isCompleted() const { return _isCompleted; }
    void setIsCompleted(bool completed);

    double progress() const { return _progress; }
    void setProgress(double progress);

public slots:
    void startSequence();
    void completeStartup();
    void simulateInitialization();

signals:
    void isReadyChanged(bool ready);
    void isCompletedChanged(bool completed);
    void progressChanged(double progress);
    void sequenceStarted();
    void sequenceFinished();

private:
    bool _isReady = false;
    bool _isCompleted = false;
    double _progress = 0.0;
    QTimer *_initTimer = nullptr;
};

} // namespace Volador
