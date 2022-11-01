#ifndef COMPINFOFRAMES_H
#define COMPINFOFRAMES_H

#include <QObject>
#include <QModelIndex>
#include <QmlObjectListModel.h>

#include "CompInfo.h"

#include "Frames/Frames.h"

/**
 * @brief The CompInfoFrames class is objectified version of the frames JSON file
 *
 * This includes the version information and other highest-level information contained in the
 * Frames.json file.
 */
class CompInfoFrames : public Frames
{
    Q_OBJECT

    Q_PROPERTY(QmlObjectListModel* selectedFrames READ selectedFrames NOTIFY selectedFramesChanged)
    Q_PROPERTY(QString frames_id_param_name READ frames_id_param_name NOTIFY frames_id_param_name_changed)
    Q_PROPERTY(int finalSelectionFrameID READ finalSelectionFrameID NOTIFY finalSelectionFrameIDChanged)

public:
    CompInfoFrames(QObject *parent = nullptr);

    /**
     * @brief Parse provided JsonDocument data struct
     *
     * Schema is defined in `frames.schema.json`
     */
    bool parseJson(const QJsonDocument &json);

    void print_info() const;

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
//    /**
//     * @brief List of `Frames`, which can contain other Frames in it's subgroup
//     */
    // QList<Frames*> _frames;

    /**
     * @brief QmlObjectListModel containing the frame list that user selected to view
     */
    QmlObjectListModel* _selectedFrames = new QmlObjectListModel(this);

    // Data from JSON
    int _schema_version{0};
    QString _frames_id_param_name;

    // Variables for run time processing
    int _finalSelectionFrameID{FRAME_ID_INVALID}; // Holds Frame ID of the last selected EndNode frame from the user
};
#endif // COMPINFOFRAMES_H
