#ifndef QGCSETTINGSGROUP_H
#define QGCSETTINGSGROUP_H

#include <QObject>
#include <QList>
#include <QSettings>


class QGCSettingsGroup
{
public:
    explicit QGCSettingsGroup(QGCSettingsGroup* pparentGroup, QString groupName = "default");

    void setGroupName(QString groupName);
    void loadGroup();
    void saveGroup();

protected:
    virtual void serialize(QSettings* psettings);
    virtual void deserialize(QSettings* psettings);
    virtual QString getGroupName(void);
    virtual void saveChildren(void);
    virtual void loadChildren(void);

    QString getGroupPath();

private:
    QGCSettingsGroup* p_parent;
    QString         _groupName;


signals:

public slots:

};

#endif // QGCSETTINGSGROUP_H
