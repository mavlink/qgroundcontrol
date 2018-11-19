#pragma once

#include <QWidget>

namespace Ui
{
class QGCUnconnectedInfoWidget;
}

/**
 * @brief Widget providing initial instructions
 *
 * This widget provides initial instructions to new users how
 * to connect a simulation or a live link to a system to
 * QGroundControl. The widget should be automatically be
 * unloaded and destroyed once the first MAV/UAS is connected.
 *
 * @see UASListWidget
 */
class QGCUnconnectedInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUnconnectedInfoWidget(QWidget *parent = 0);
    ~QGCUnconnectedInfoWidget();

    Ui::QGCUnconnectedInfoWidget *ui;

public slots:
    /** @brief Start simulation */
    void simulate();
    /** @brief Add a link */
    void addLink();
};

