#pragma once

#include "QGCState.h"

#include <functional>

/// State that reports progress on entry.
/// Useful for state machines that need to track progress through a sequence of states.
///
/// Progress can be reported as:
/// - A fixed value (0.0 to 1.0)
/// - Via a callback function for dynamic calculation
///
/// Example usage:
/// @code
/// // Fixed progress
/// auto* state1 = new ProgressState("Step1", &machine, 0.25);
/// connect(state1, &ProgressState::progressChanged, this, &MyClass::onProgress);
///
/// // Dynamic progress via callback
/// auto* state2 = new ProgressState("Step2", &machine, [this]() {
///     return calculateProgress();
/// });
/// @endcode
class ProgressState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(ProgressState)

public:
    using ProgressCallback = std::function<float()>;
    using Action = std::function<void()>;

    /// Create a progress state with a fixed progress value
    /// @param stateName Name for this state
    /// @param parent Parent state
    /// @param progress Fixed progress value (0.0 to 1.0)
    /// @param action Optional action to execute on entry
    ProgressState(const QString& stateName, QState* parent, float progress, Action action = nullptr);

    /// Create a progress state with a dynamic progress callback
    /// @param stateName Name for this state
    /// @param parent Parent state
    /// @param progressCallback Function that returns current progress (0.0 to 1.0)
    /// @param action Optional action to execute on entry
    ProgressState(const QString& stateName, QState* parent, ProgressCallback progressCallback, Action action = nullptr);

    /// Get the current progress value
    float progress() const;

    /// Set a fixed progress value
    void setProgress(float progress);

    /// Set a progress callback
    void setProgressCallback(ProgressCallback callback);

signals:
    /// Emitted when the state is entered with the current progress value
    void progressChanged(float progress);

protected:
    void onEnter() override;

private:
    float _fixedProgress = 0.0f;
    ProgressCallback _progressCallback;
    Action _action;
    bool _useCallback = false;
};
