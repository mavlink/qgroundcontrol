#ifndef QGCPARAMID_H
#define QGCPARAMID_H

#include <QString>
#include <QObject>
namespace OpalRT
{
    class QGCParamID : public QString
    {
        Q_OBJECT
    public:
        QGCParamID();
    };
}
#endif // QGCPARAMID_H
