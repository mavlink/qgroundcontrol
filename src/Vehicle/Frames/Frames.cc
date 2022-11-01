#include "Frames.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDirIterator>

// Frames - Node
Frames::Frames(QObject* parent, Frames* parent_frame)
    : QObject(parent), _parentFrame(parent_frame)
{

}

Frames::~Frames()
{
    qDeleteAll(_subgroups);
}

QString Frames::getImageUrlFromName(const QString imageName, const QString fallback)
{
    QDirIterator it(FRAME_IMEAGES_PATH);

    while (it.hasNext())
    {
        QString filename = it.next();
        QFileInfo file(filename);

        if (file.isDir()) { // Check if it's a dir
            continue;
        }

        // Note: Allow files to be insensitive to case, as we may have occasional
        // case differences (e.g. Multirotor vs MultiRotor), and we should allow that :P
        if (file.baseName().compare(imageName, Qt::CaseInsensitive) == 0) {
            // FIXME: Sort of hacky way to get the Qt Resources directory
            // mapped properly. Since we will have filePath() return ":/images/*.svg" style,
            // this doesn't get recognized in `Frames.qml` by Image item: `qrc:/:/images/*.svg`
            // So we need to already prepand `qrc` to get it mapped properly.
            // This already exists in QGC: https://github.com/mavlink/qgroundcontrol/blob/7a357de47132c5f80aa6d703a392a9c7323f0c1d/src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc#L53
            return file.filePath().prepend("qrc");
        }
    }

    if (!fallback.isNull()) {
//        qDebug() << "Fallback is: " << fallback << ", with size: " << fallback.size();
        return fallback;

    } else {
        // No image found, *try to return undefined UFO image, and fallback if not found
        return getImageUrlFromName(FRAME_UNKNOWN_NAME, "");
    }
}

QString Frames::findClosestParentImageUrl() const
{
    Frames* parent = _parentFrame;

    while (parent != nullptr) {
        if (!parent->_imageUrl.isEmpty()) {
            return parent->_imageUrl;
        }

        parent = parent->_parentFrame;
    }

    // No valid url found even in the root-most parent node
    return "";
}

bool Frames::parseJson(const QJsonObject &json)
{
    // Parse rquired properties

    // Name
    QJsonValue name = json.value("name");
    if (!name.isNull() && name.isString()) {
        _name = name.toString();
    } else {
        qWarning() << "Frames name is not set!" << json;
    }

    // Type
    QString type = json.value("type").toString();
    if (type.isNull()) {
        qWarning() << "Type is undefined for airframe: " << _name;

    } else if (type == "group") {
        _type = FrameType::FrameGroup;

    } else if (type == "frame") {
        _type = FrameType::FrameEndNode;

    } else {
        _type = FrameType::FrameUndefined;

    }

    // Frame ID: required only for end nodes
    if (_type == FrameType::FrameEndNode) {
        QJsonValue frameID = json.value("frame_id");
        if (!frameID.isNull()) {
            _frame_id = frameID.toInt(FRAME_ID_UNDEFINED);
        } else {
            qWarning() << "Frame ID not set!" << json;
        }
    }

    // Non-required properties
    QString description = json.value("description").toString();
    if (!description.isEmpty()) {
        _description = description;
    }

    QString imageEnum = json.value("image").toString();
    if (!imageEnum.isEmpty()) {
        _imageEnum = imageEnum;
        _imageUrl = getImageUrlFromName(_imageEnum, "");
    }

    QString imageCustom = json.value("image-custom").toString();
    if (!imageCustom.isEmpty()) {
        // Override url set by imageEnum with a custom, if it exists
        _imageUrl = getImageUrlFromName(imageCustom, "");
//        qDebug() << "imageCustomUrl fetched: " << _imageUrl << ", name: " << _name << ", customUrl was: " << imageCustom;
    }

    if (_imageUrl.isEmpty()) {
        // If we still don't have any valid image URL, fallback to nearest valid parent's image URL
        _imageUrl = findClosestParentImageUrl();
    }

    QString url = json.value("url").toString();
    if (!url.isNull()) {
        _productUrl = url;
    }

    QString manufacturer = json.value("manufacturer").toString();
    if (!manufacturer.isNull()) {
        _manufacturer = manufacturer;
    }

    // If it has a sub-group, parse them and append it to the list recursively
    QJsonArray subgroups = json.value("subgroups").toArray();
    for (const auto &&frameJson : subgroups) {
        Frames *new_frame = new Frames(this, this);
        QJsonObject frameObject = frameJson.toObject();
        new_frame->parseJson(frameObject);
        _subgroups.append(new_frame);
    }

    return true;
}

void Frames::print_info(QString prefix) const
{
    QString str = "";

    // Required values
    str.append(QString("name: %1, type: %2 | ").arg(_name, QString::number((int)_type)));

    if (_type == FrameType::FrameEndNode) {
        // Frame ID is required only for end nodes
        str.append(QString("frame ID: %1 | ").arg(QString::number(_frame_id)));
    }

    // Debug
//    qulonglong p = (qulonglong)_parentFrame;
//    str.append(QString("parentFrame address: %1 | ").arg(p));

    // Non-required values
    if (!_description.isEmpty()) {
        str.append(QString("description: %1 | ").arg(_description));
    }

    if (!_imageEnum.isEmpty()) {
        str.append(QString("image Enum: %1 | ").arg(_imageEnum));
    }

    if (!_imageUrl.isEmpty()) {
        str.append(QString("image URL: %1 | ").arg(_imageUrl));
    }

    if (!_manufacturer.isEmpty()) {
        str.append(QString("manufacturer: %1 | ").arg(_manufacturer));
    }

    if (!_productUrl.isEmpty()) {
        str.append(QString("Product URL: %1").arg(_productUrl));
    }

    qDebug() << prefix + str;

    // Print out subgroups info
    if (!_subgroups.isEmpty()) {
        qDebug() << prefix + QString("Subgroups size: %1").arg(QString::number(_subgroups.length()));

        // Recursively call subgroup frame's print_info
        // Enlongate the prefix with "--"
        prefix.append("--");

        for (const auto &frame : _subgroups) {
            frame->print_info(prefix);
        }
    }
}
