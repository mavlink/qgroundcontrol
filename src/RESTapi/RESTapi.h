#pragma once


#include <QObject>
#include <QString>
#include <thread>
#include <memory>
#include <QVariantList>

#define LANDBUTTONICON_NORMAL   "/qmlimages/LandMode.svg"
#define LANDBUTTONICON_WAIT     "/res/wait_clock.svg"

class RESTapi : public QObject{
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY newMissionPlan)
    Q_PROPERTY(QString landButtonIcon READ landButtonIcon NOTIFY landButtonIconChanged)
signals:
    void messageChanged();
    void ctrUpdate();
    void newMissionPlan();
    void landButtonIconChanged();
    void newMarkers(QVariantList markers);


public:
    void run();

    QString message() const;
    QString currentFilePath() const;
    QString landButtonIcon() const;

public slots:
    void landButtonPressed();

private:
    void worker();
    QString _msg = "hallo";
    QString _currentFilePath = "";
    QString _landButtonIcon = LANDBUTTONICON_NORMAL;
    std::shared_ptr<std::thread> p_t;

    bool _landingPlanRequested = false;
};


