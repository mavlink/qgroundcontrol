#ifndef FRAMESBASE_H
#define FRAMESBASE_H

#include "Frames/Frames.h"

#include <QmlObjectListModel.h>
#include <QModelIndex>

class Vehicle;

/**
 * @brief The FramesBase class that serves as an instantiation of the Frames metadata
 *
 * This includes the version information and other highest-level information contained in the
 * Frames.json file.
 *
 * Note: Since the Frames metadata has a recursive structure (Frame can include list of frames), and the
 * root frames (the Frames component metadata istelf) has some special data (e.g. Parameter corresponding to the
 * frames_id)
 */
class FramesBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool frameJSONLoaded READ isFrameJSONLoaded NOTIFY frameJSONLoadedChanged)
    Q_PROPERTY(Frames* rootFrame READ getRootFrame NOTIFY rootFrameChanged)
    Q_PROPERTY(QmlObjectListModel* selectedFrames READ selectedFrames NOTIFY selectedFramesChanged)
    Q_PROPERTY(QList<QString> frame_parameters READ frame_parameters NOTIFY frameParametersChanged)
//    Q_PROPERTY(QList<int> finalSelectionFrameParamValues READ finalSelectionFrameParamValues NOTIFY finalSelectionFrameParamValuesChanged)
    Q_PROPERTY(Frames* finalSelectedFrame READ getFinalSelectedFrame NOTIFY finalSelectedFrameChanged)

public:
    FramesBase(QObject *parent, Vehicle* vehicle);
    ~FramesBase() = default;

    /**
     * @brief Parse provided JsonDocument data struct
     *
     * Schema is defined in `frames.schema.json`
     */
    bool parseJson(const QJsonDocument &json);

    /**
     * @brief Cache the frames metadata JSON file's content
     *
     * Question: Actuators metadata caches the file & parses it later.
     * Should Frames do the same?
     */
    void load(const QString &json_file);

    // Returns whether the Frames JSON metadata was loaded & parsed correctly
    bool isFrameJSONLoaded() { return _framesJSONLoaded; }

    void print_info() const;

    /**
     * @brief Return the super node of the frames hierarchy
     *
     * Needed for Vehicle class' access to the Frames structure
     */
    Frames *getRootFrame() { return &_frame_root; }

    /**
     * @brief Getter for the List of Frames to display
     */
    QmlObjectListModel* selectedFrames() { return _selectedFrames; }

    /**
     * @brief Setter for the list of frames to display
     */
    void setSelectedFrames(QList<Frames*> frames);

    /**
     * @brief Getter for the last user selected Frame Endnode's Frame ID
     */
//    QList<int> finalSelectionFrameParamValues() { return _finalSelectionFrameParamValues; } const

    /**
     * @brief Getter for final selected frames ID
     * @return
     */
    Frames* getFinalSelectedFrame() {return _finalSelectedFrame; }

    /**
     * @brief Getter for the parameters needed for frame selection to come into effect
     */
    QList<QString> frame_parameters() const { return _frame_parameter_names; }

    /**
     * @brief QML invokable function to process when user selects a frame
     *
     * This should update the selectedFrames property to the underlaying layer of frames
     */
    Q_INVOKABLE bool selectFrame(Frames *frame);

    /**
     * @brief Changes `_selectedFrames` to track parent frame group (if it exists)
     */
    Q_INVOKABLE bool gotoParentFrame();

signals:
     void frameJSONLoadedChanged();
     void rootFrameChanged();
     void selectedFramesChanged();
     void frameParametersChanged();
//     void finalSelectionFrameParamValuesChanged();
     void finalSelectedFrameChanged();

private:
     // Root frames object without a parent, that serves as a container including all the frames defined in the "frames_v1" element of the schema
     Frames _frame_root;

     // Flag to indicate if the Frames JSON file was successfully read / loaded without errors
     bool _framesJSONLoaded{false};

     // Pointer to vehicle object this metadata is tied to
     Vehicle *_vehicle{nullptr};

     // Cached Json Metadata file content
     QJsonDocument _jsonMetadata;

    // QmlObjectListModel containing the frame list that user selected to view
    QmlObjectListModel* _selectedFrames = new QmlObjectListModel(this);

    // Data from JSON
    int _schema_version{0};

    // Parameters to set for applying frame selection
    QList<QString> _frame_parameter_names;

    // Variables for run time processing
    Frames *_finalSelectedFrame{nullptr};       // Holds pointer for final selected frame for displaying pop-up
    QList<int> _finalSelectionFrameParamValues; // Holds Frame ID of the last selected EndNode frame from the user
};

#endif // FRAMESBASE_H
