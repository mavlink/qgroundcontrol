#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSize>
#include <QtCore/QVariantList>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QMutex>

struct YoloBox {
    int classId;
    float confidence;
    int x;
    int y;
    int width;
    int height;
};

class YoloInference : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(float confidenceThreshold READ confidenceThreshold WRITE setConfidenceThreshold NOTIFY confidenceThresholdChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE loadModel NOTIFY modelPathChanged)
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY showLabelsChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QVariantList boundingBoxes READ boundingBoxes NOTIFY boundingBoxesChanged)

public:
    explicit YoloInference(QObject *parent = nullptr);
    ~YoloInference();

    bool loadModel(const QString& modelPath);
    
    // Call this from the Gstreamer worker thread when a new frame is available.
    // It returns immediately if another frame is currently being processed.
    void processFrame(const cv::Mat& frame);

    float confidenceThreshold() const { return _confidenceThreshold; }
    void setConfidenceThreshold(float threshold) {
        if (_confidenceThreshold != threshold) {
            _confidenceThreshold = threshold;
            emit confidenceThresholdChanged(threshold);
        }
    }

    bool enabled() const { return _enabled; }
    void setEnabled(bool enabled) {
        if (_enabled != enabled) {
            _enabled = enabled;
            emit enabledChanged(enabled);
        }
    }

    QString modelPath() const { return _modelPath; }

    bool showLabels() const { return _showLabels; }
    void setShowLabels(bool show) {
        if (_showLabels != show) {
            _showLabels = show;
            emit showLabelsChanged(show);
        }
    }

    int fps() const { return _fps; }
    
    QVariantList boundingBoxes() const { return _boundingBoxes; }

    float nmsThreshold() const { return _nmsThreshold; }
    void setNmsThreshold(float threshold) { _nmsThreshold = threshold; }

    const std::vector<std::string>& classNames() const { return _classNames; }
    void setClassNames(const std::vector<std::string>& names) { _classNames = names; }

signals:
    void enabledChanged(bool enabled);
    void confidenceThresholdChanged(float threshold);
    void modelPathChanged(QString modelPath);
    void showLabelsChanged(bool showLabels);
    void fpsChanged(int fps);
    void boundingBoxesChanged(QVariantList boundingBoxes);
    void inferenceCompleted(QVariantList boundingBoxes, int lastInferenceTimeMs);

private slots:
    void _onInferenceCompleted(QVariantList boxes, int timeMs);

private:
    void _runInferenceBackground(cv::Mat frame);
    QString _modelPath;
    bool _showLabels = true;
    int _fps = 0;
    QVariantList _boundingBoxes;
    cv::dnn::Net _net;
    bool _isModelLoaded = false;
    bool _enabled = false;
    float _confidenceThreshold = 0.45f;
    float _nmsThreshold = 0.45f;
    std::vector<std::string> _classNames;
    
    QSize _inputSize = QSize(640, 640);

    QMutex _inferenceMutex;
    bool _inferenceRunning = false;
};
