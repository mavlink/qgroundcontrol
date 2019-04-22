#ifndef FREQCALIBRATIONMODEL_H
#define FREQCALIBRATIONMODEL_H

#include <QObject>

class FreqCalibrationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit FreqCalibrationModel(QObject *parent = 0);
    FreqCalibrationModel(const QString &color,QObject *parent=0);

    QString color() const;
    void setColor(const QString &value);

signals:
    void colorChanged();

public slots:

private:
    QString c_color;
};


// for CliTestModel

class CliTestModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY valueChanged)
public:
    explicit CliTestModel(QObject *parent = 0);
    CliTestModel(const float &value, QObject *parent=0);

    float value() const;
    void setValue(const float &value);
signals:
    void valueChanged();
public slots:

private:
    float x_value;
};

#endif // FREQCALIBRATIONMODEL_H
