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

    Q_PROPERTY(QmlObjectListModel* selectedFrames READ selectedFrames NOTIFY selectedFramesChanged)
    Q_PROPERTY(QString frames_id_param_name READ frames_id_param_name NOTIFY frames_id_param_name_changed)
    Q_PROPERTY(int finalSelectionFrameID READ finalSelectionFrameID NOTIFY finalSelectionFrameIDChanged)

public:
    FramesBase(QObject *parent, Vehicle* vehicle);

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

    void print_info() const;

    /**
     * @brief Return the super node of the frames hierarchy
     *
     * Needed for Vehicle class' access to the Frames structure
     */
    Frames *getRootFrames() { return &_frames_root; }

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
    int finalSelectionFrameID() { return _finalSelectionFrameID; } const

    /**
     * @brief Getter for the parameter name corresponding to the Frame ID
     */
    QString frames_id_param_name() const { return _frames_id_param_name; }

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
     void selectedFramesChanged();
     void frames_id_param_name_changed();
     void finalSelectionFrameIDChanged();

private:
     // Frames root object without a parent, that serves as a container including all the frames defined in the "frames_v1" element of the schema
     Frames _frames_root;

     // Pointer to vehicle object this metadata is tied to
     Vehicle *_vehicle{nullptr};

     // Cached Json Metadata file content
     QJsonDocument _jsonMetadata;

    // QmlObjectListModel containing the frame list that user selected to view
    QmlObjectListModel* _selectedFrames = new QmlObjectListModel(this);

    // Data from JSON
    int _schema_version{0};
    QString _frames_id_param_name;

    // Variables for run time processing
    int _finalSelectionFrameID{FRAME_ID_INVALID}; // Holds Frame ID of the last selected EndNode frame from the user
};

#endif // FRAMESBASE_H
