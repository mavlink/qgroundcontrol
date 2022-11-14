#ifndef FRAMES_H
#define FRAMES_H

#include <QList>
#include <QJsonDocument>
#include <QObject>
#include <QVariantList>

/**
 * @brief The Frames class defining a single frame (could be a group)
 *
 * Class properties follow the Frame schema defined in the `frames.schema.json`.
 */

enum class FrameType {
    FrameUndefined = 0,     // Undefined
    FrameGroup = 1,         // Group: has valid `_subgroup` QList
    FrameEndNode = 2        // End-mode
};

static constexpr int FRAME_ID_INVALID = -2; // Used for storing intial value of selected final Frame ID in FrameComponent
static constexpr int FRAME_ID_UNDEFINED = -1; // Used for a Frame Group's ID

static const QString FRAME_IMEAGES_PATH = ":/images/";
static const QString FRAME_UNKNOWN_NAME = "AirframeUnknown";

/**
 * @brief Base Frame class defining a group of frames / or an end node frame
 *
 * This defines the recursive Frames metadata structure defined in the JSON schema.
 */
class Frames : public QObject
{
    Q_OBJECT

    // Basic properties
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QList<int> frame_param_values READ getFrameParamValues NOTIFY framParamValuesChanged)

    // Optional properties
    Q_PROPERTY(QString image_url READ getImageurl NOTIFY imageurlChanged)
    Q_PROPERTY(QString description READ getDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString manufacturer READ getManufacturer NOTIFY manufacturerChanged)
    Q_PROPERTY(QString product_url READ getProducturl NOTIFY producturlChanged)

    // Properties for helping QML rendering
    Q_PROPERTY(bool isEndNode READ getIsEndNode NOTIFY isEndNodeChanged)

public:
    /**
     * @brief Constructor
     *
     * Note: This had to be added, since the `Frames` class needs to be compatible
     * to `QObject` in a sense of data handling at the `DataObjectModel` level.
     *
     * @param parent Generic QObject parent item needed to store this in `DataObjectModel`
     * @param parent_frame Parent Frame Group that this frame belongs to
     */
    Frames(QObject* parent, Frames* parent_frame);
    ~Frames();

    /**
     * @brief Parse a single airframe object in the JsonDocument
     *
     * Schema is defined in `frames.schema.json/#/$defs/frame`
     */
    bool parseJson(const QJsonObject &json);

    void print_info(QString prefix = "") const;

    QString getName() { return _name; }
    QList<int> getFrameParamValues() { return _frame_param_values; }
    QString getImageurl() { return _imageUrl; }
    QString getDescription() { return _description; }
    QString getManufacturer() { return _manufacturer; }
    QString getProducturl() { return _productUrl; }

    /**
     * @brief If the Frme is the end node (has no subgroups), return true
     */
    bool getIsEndNode() { return (_type == FrameType::FrameEndNode); }

signals:
    void nameChanged();
    void framParamValuesChanged();
    void imageurlChanged();
    void descriptionChanged();
    void manufacturerChanged();
    void producturlChanged();
    void isEndNodeChanged();

private:
    /**
     * @brief Finds the vehicle image from the resources with the matching name
     * @param String of the image
     * @param fallback Used for finding Unknown Frame, if we can't even find the unknown frame file, we fallback to this url
     */
    QString getImageUrlFromName(const QString imageName, const QString fallback = QString());

    /**
     * @brief Goes up the parent tree structure to find latest viable image URL to display
     */
    QString findClosestParentImageUrl() const;

    // Parent Frames node
    Frames *_parentFrame{nullptr};

    // Required properties
    FrameType _type{FrameType::FrameUndefined};
    QString _name;
    QList<int> _frame_param_values;

    // Subgroup that holds many different sub-frames
    QList<Frames*> _subgroups;

    // Non-required properties
    QString _description;
    QString _imageEnum;
    QString _imageUrl; // this gets set by default via imageEnum, but can be overwritten by image-url in JSON
    QString _manufacturer;
    QString _productUrl;

    // Allow access to private variables to the superceeding FramesBase class
    friend class FramesBase;
};

#endif // FRAMES_H
