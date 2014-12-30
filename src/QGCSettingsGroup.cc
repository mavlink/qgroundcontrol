#include "QGCSettingsGroup.h"
#include <QSettings>
#include <QDebug>

QGCSettingsGroup::QGCSettingsGroup(QGCSettingsGroup* pparentGroup, QString groupName)
{
    p_parent = pparentGroup;
    _groupName = groupName;
}


void QGCSettingsGroup::loadGroup(){
    QSettings settings;
    QString groupPath = getGroupPath();
    settings.beginGroup(groupPath);
    settings.sync();
    deserialize(&settings);
    settings.endGroup();
    loadChildren();
    qDebug() << "Loaded settings group: " << groupPath;
}

void QGCSettingsGroup::saveGroup(){
    QSettings settings;
    QString groupPath = getGroupPath();
    settings.remove(groupPath);
    settings.beginGroup(groupPath);
    serialize(&settings);
    settings.endGroup();
    settings.sync();
    saveChildren();
    qDebug() << "Saved settings group: " << groupPath;
}


void QGCSettingsGroup::setGroupName(QString groupName){
    _groupName = groupName;
    if(p_parent != NULL)
        p_parent->saveGroup();
    else
        this->saveGroup();
}

QString QGCSettingsGroup::getGroupPath(){
    if(p_parent != NULL)
    {
        return p_parent->getGroupPath() + "/" + getGroupName();
    }
    else
        return getGroupName();
}

void QGCSettingsGroup::serialize(QSettings* psettings){

}

void QGCSettingsGroup::deserialize(QSettings* psettings){

}

QString QGCSettingsGroup::getGroupName(void){
    return _groupName;
}

void QGCSettingsGroup::saveChildren(void) {};
void QGCSettingsGroup::loadChildren(void) {};
