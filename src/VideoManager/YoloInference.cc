#include "YoloInference.h"
#include <QDebug>
#include <QVariantMap>
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>

YoloInference::YoloInference(QObject *parent)
    : QObject(parent), _enabled(false)
{
    // Register types for QML
    qRegisterMetaType<QVariantList>("QVariantList");
    
    // Default COCO classes (truncated for example, or you can supply generic labels)
    _classNames = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };

    connect(this, &YoloInference::inferenceCompleted, this, &YoloInference::_onInferenceCompleted);
}

void YoloInference::_onInferenceCompleted(QVariantList boxes, int timeMs)
{
    _boundingBoxes = boxes;
    emit boundingBoxesChanged(_boundingBoxes);

    if (timeMs > 0) {
        int newFps = 1000 / timeMs;
        if (newFps != _fps) {
            _fps = newFps;
            emit fpsChanged(_fps);
        }
    }
}

YoloInference::~YoloInference()
{
}

bool YoloInference::loadModel(const QString& modelPath)
{
    _isModelLoaded = false;
    _modelPath = modelPath;
    
    try {
        _net = cv::dnn::readNet(modelPath.toStdString());
        _net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        _net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU); // Or use CUDA if available
        _isModelLoaded = true;
        qDebug() << "YOLO Model loaded successfully:" << modelPath;
    } catch (const cv::Exception& e) {
        qWarning() << "Failed to load YOLO model:" << e.what();
    }
    
    return _isModelLoaded;
}

void YoloInference::processFrame(const cv::Mat& frame)
{
    if (!_enabled || !_isModelLoaded) {
        return; // Drop frame if YOLO is disabled or not loaded
    }
    
    if (_inferenceMutex.tryLock()) {
        if (_inferenceRunning) {
            _inferenceMutex.unlock();
            return; // Drop if already running
        }
        _inferenceRunning = true;
        _inferenceMutex.unlock();
        
        // Clone frame to avoid lifetime issues
        cv::Mat frameCopy = frame.clone();
        
        // Run inference async
        QThreadPool::globalInstance()->start([this, frameCopy]() {
            this->_runInferenceBackground(frameCopy);
        });
    }
}

void YoloInference::_runInferenceBackground(cv::Mat frame)
{
    QElapsedTimer timer;
    timer.start();
    
    cv::Mat blob;
    // Standard YOLOv5/v8 preprocessing
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(_inputSize.width(), _inputSize.height()), cv::Scalar(), true, false);
    
    _net.setInput(blob);
    std::vector<cv::Mat> outputs;
    
    try {
        _net.forward(outputs, _net.getUnconnectedOutLayersNames());
    } catch (const cv::Exception& e) {
        qWarning() << "YOLO Inference error:" << e.what();
        _inferenceMutex.lock();
        _inferenceRunning = false;
        _inferenceMutex.unlock();
        return;
    }
    
    // Post-processing
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    
    // Logic varies depending on whether it's YOLOv5 (outputs[0].dims == 3) or YOLOv8
    if (outputs.size() > 0 && outputs[0].dims == 3) {
        // Handle standard YOLO outputs (batch, rows, dims)
        int rows = outputs[0].size[1];
        int dimensions = outputs[0].size[2];
        
        // For YOLOv8, output is [1, dims, rows], meaning it needs transposition
        bool isYolov8 = false;
        if (dimensions > rows) {
            isYolov8 = true;
            cv::transposeND(outputs[0], {0, 2, 1}, outputs[0]);
            rows = outputs[0].size[1];
            dimensions = outputs[0].size[2];
        }

        float* data = (float*)outputs[0].data;

        float x_factor = frame.cols / (float)_inputSize.width();
        float y_factor = frame.rows / (float)_inputSize.height();

        for (int i = 0; i < rows; ++i) {
            if (isYolov8) {
                float* classesScores = data + 4;
                cv::Mat scores(1, _classNames.size(), CV_32FC1, classesScores);
                cv::Point classIdPoint;
                double maxClassScore;
                cv::minMaxLoc(scores, 0, &maxClassScore, 0, &classIdPoint);

                if (maxClassScore > _confidenceThreshold) {
                    confidences.push_back(maxClassScore);
                    classIds.push_back(classIdPoint.x);

                    float x = data[0];
                    float y = data[1];
                    float w = data[2];
                    float h = data[3];

                    int left = int((x - 0.5 * w) * x_factor);
                    int top = int((y - 0.5 * h) * y_factor);
                    int width = int(w * x_factor);
                    int height = int(h * y_factor);

                    boxes.push_back(cv::Rect(left, top, width, height));
                }
            } else {
                float confidence = data[4];
                if (confidence >= _confidenceThreshold) {
                    float* classesScores = data + 5;
                    cv::Mat scores(1, _classNames.size(), CV_32FC1, classesScores);
                    cv::Point classIdPoint;
                    double maxClassScore;
                    cv::minMaxLoc(scores, 0, &maxClassScore, 0, &classIdPoint);

                    if (maxClassScore > _confidenceThreshold) {
                        confidences.push_back(confidence * maxClassScore);
                        classIds.push_back(classIdPoint.x);

                        float x = data[0];
                        float y = data[1];
                        float w = data[2];
                        float h = data[3];

                        int left = int((x - 0.5 * w) * x_factor);
                        int top = int((y - 0.5 * h) * y_factor);
                        int width = int(w * x_factor);
                        int height = int(h * y_factor);

                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }
            }
            data += dimensions;
        }
    }
    
    // NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, _confidenceThreshold, _nmsThreshold, indices);
    
    QVariantList qmlBoxes;
    for (int idx : indices) {
        cv::Rect box = boxes[idx];
        QVariantMap qmlBox;
        qmlBox["classId"] = classIds[idx];
        qmlBox["className"] = QString::fromStdString(_classNames[classIds[idx]]);
        qmlBox["confidence"] = confidences[idx];
        qmlBox["x"] = static_cast<double>(box.x) / frame.cols; // normalize 0-1
        qmlBox["y"] = static_cast<double>(box.y) / frame.rows;
        qmlBox["w"] = static_cast<double>(box.width) / frame.cols;
        qmlBox["h"] = static_cast<double>(box.height) / frame.rows;
        qmlBoxes.append(qmlBox);
    }
    
    _inferenceMutex.lock();
    _inferenceRunning = false;
    _inferenceMutex.unlock();
    
    emit inferenceCompleted(qmlBoxes, timer.elapsed());
}
