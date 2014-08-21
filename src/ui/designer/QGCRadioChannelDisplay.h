#ifndef QGCRADIOCHANNELDISPLAY_H
#define QGCRADIOCHANNELDISPLAY_H

#include <QWidget>

class QGCRadioChannelDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit QGCRadioChannelDisplay(QWidget *parent = 0);
    void setOrientation(Qt::Orientation orient);
    void setValue(int value);
    void showMinMax(bool show = true);
    void hideMinMax();
    bool isMinMaxShown() { return m_showMinMax; }
    void setValueAndRange(int val, int min, int max);
    void setMinMax(int min, int max);
    void setMin(int value);
    void setMax(int value);
    void setName(QString name);
    int value() { return m_value; }
    int min() { return m_min; }
    int max() { return m_max; }

protected:
    void paintEvent(QPaintEvent *event);
private:
    Qt::Orientation m_orientation;
    int m_value;
    int m_min;
    int m_max;
    bool m_showMinMax;
    QString m_name;
    QBrush m_fillBrush;

signals:
    
public slots:
    
};

#endif // QGCRADIOCHANNELDISPLAY_H
