#include "FramesBase.h"

#include "Frames/Frames.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

FramesBase::FramesBase(QObject *parent, Vehicle* vehicle)
    :_frames_root(parent, nullptr), _vehicle(vehicle)
{
    // Note: `_frames_root` must have a nullptr as the parent node!
}

void FramesBase::load(const QString &json_file)
{
    QFile file;
    file.setFileName(json_file);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json_data = file.readAll();
    file.close();

    // store the metadata to be loaded later after all params are available
    _jsonMetadata = QJsonDocument::fromJson(json_data.toUtf8());

    // Parse immediately
    parseJson(_jsonMetadata);
}

bool FramesBase::parseJson(const QJsonDocument &json)
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
        // Set the `FramesBase` as parent of subgroup Frames
        Frames *new_frame = new Frames(this, &_frames_root);
        QJsonObject frameObject = frameJson.toObject();
        new_frame->parseJson(frameObject);
        _frames_root._subgroups.append(new_frame);
    }

    // for now, set the selected frames as frames itself!
    setSelectedFrames(_frames_root._subgroups);

    return true;
}

void FramesBase::print_info() const
{
    QString str = "";

    // Required values
    str.append(QString("Schema ver: %1 | Frames ID Param name: %2 | ").arg(QString::number(_schema_version), _frames_id_param_name));
    str.append(QString("Framegroups size: %1").arg(QString::number(_frames_root._subgroups.length())));

    qDebug() << str;

    _frames_root.print_info("L--");
}

void FramesBase::setSelectedFrames(QList<Frames*> frames)
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

bool FramesBase::selectFrame(Frames *frame)
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

bool FramesBase::gotoParentFrame()
{
    // Do we have a valid set of selected Frames?
    if (_selectedFrames->count() >= 0) {
        Frames *frame = static_cast<Frames*>(_selectedFrames->get(0));
        qDebug() << frame;

        if (frame != nullptr) {
            Frames *parentframe = frame->_parentFrame;
            qDebug() << parentframe;

            if (parentframe != nullptr) {
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
