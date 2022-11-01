#include "CompInfoFrames.h"

#include <QJsonObject>
#include <QJsonArray>

CompInfoFrames::CompInfoFrames(QObject *parent)
    :Frames(parent, nullptr)
{
    // Since we are the root frame group, we set parent_frame pointer to `nullpter`.
    qDebug() << "CompInfoFrames constructor: " << parent;
    // Do nothing
}

bool CompInfoFrames::parseJson(const QJsonDocument &json)
{
    QJsonObject obj = json.object();

    // Get Frame ID Parameter
    QJsonValue frameIdJson = obj.value("settings").toObject().value("frame_id_parameter");
    if (!frameIdJson.isNull() && frameIdJson.isString()) {
        _frames_id_param_name = frameIdJson.toString();
        emit frames_id_param_name_changed();
    } else {
        qWarning() << "Frames ID Parameter is not set!";
    }

    // Get Version
    QJsonValue versionJson = obj.value("version");
    if (!versionJson.isNull()) {
        _schema_version = versionJson.toInt();
    } else {
        qWarning() << "Frames ID Parameter is not set!";
    }

    // Get Frames data struct
    QJsonValue framesJson = obj.value("frames_v1");
    QJsonArray framesArray = framesJson.toArray();

    for (const auto &&frameJson : framesArray) {
        // Set the `CompInfoFrames` as parent of subgroup Frames
        Frames *new_frame = new Frames(this, this);
        QJsonObject frameObject = frameJson.toObject();
        new_frame->parseJson(frameObject);
        _subgroups.append(new_frame);
    }

    // for now, set the selected frames as frames itself!
    setSelectedFrames(_subgroups);

    return true;
}

void CompInfoFrames::print_info() const
{
    QString str = "";

    // Required values
    str.append(QString("Schema ver: %1 | Frames ID Param name: %2 | ").arg(QString::number(_schema_version), _frames_id_param_name));
    str.append(QString("Framegroups size: %1").arg(QString::number(_subgroups.length())));

    qDebug() << str;

    // Print out frames info
    if (!_subgroups.isEmpty()) {
        for (const auto &frame : _subgroups) {
            // Indent for each different frame categories
            frame->print_info("L-");
        }
    }
}

void CompInfoFrames::setSelectedFrames(QList<Frames*> frames)
{
    qDebug() << "setSelectedFrames() called: " << frames;

    // This doesn't work currently due to error: `Can't convert QList<Frames*> to QList<QObject*>`
    //_selectedFrames->setDataDirect(*frames);

    // First clear the data
    _selectedFrames->clear();

    // Workaround: go through the list and append the frames
    for (auto frame : frames) {
        _selectedFrames->append(frame);
    }

    emit selectedFramesChanged();
}

bool CompInfoFrames::selectFrame(Frames *frame)
{
    qDebug() << "selectFrame() called: " << frame;

    // Show available options in the selected frame group
    if (!frame->_subgroups.isEmpty()) {
        setSelectedFrames(frame->_subgroups);
    }

    // User selected the end-node, select this item as final selection
    if (frame->_type == FrameType::FrameEndNode) {
        _finalSelectionFrameID = frame->_frame_id;
        qDebug() << "Setting final Selection Frame ID to: " << _finalSelectionFrameID;
        emit finalSelectionFrameIDChanged();
    }

    return true;
}

bool CompInfoFrames::gotoParentFrame()
{
    // Do we have a valid set of selected Frames?
    if (_selectedFrames->count() >= 0) {
        Frames *frame = static_cast<Frames*>(_selectedFrames->get(0));
        qDebug() << frame;

        if (frame != nullptr) {
            Frames *parentframe = frame->_parentFrame;
            qDebug() << parentframe;

            if (parentframe != nullptr) {
                // Debug
                //parentframe->print_info("**");

                Frames *parentparentframe = parentframe->_parentFrame;
                qDebug() << parentparentframe;

                if (parentparentframe != nullptr) {
                    selectFrame(parentparentframe);
                }
            }
        }
    }

    return true;
}
