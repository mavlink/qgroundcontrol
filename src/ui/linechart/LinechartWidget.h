/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Definition of Line chart plot widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Thomas Gubler <thomasgubler@student.ethz.ch>
 */
#ifndef LINECHARTWIDGET_H
#define LINECHARTWIDGET_H

#include <QGridLayout>
#include <QWidget>
#include <QFrame>
#include <QComboBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QScrollBar>
#include <QSpinBox>
#include <QMap>
#include <QString>
#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QReadWriteLock>
#include <QToolButton>
#include <QTimer>
#include <qwt_plot_curve.h>

#include "LinechartPlot.h"
#include "UASInterface.h"
#include "ui_Linechart.h"

#include "LogCompressor.h"

/**
 * @brief The linechart widget allows to visualize different timeseries as lineplot.
 * The display interval, the timeseries and the scaling can be changed interactively
 **/
class LinechartWidget : public QWidget
{
    Q_OBJECT

public:
    LinechartWidget(int systemid, QWidget *parent = 0);
    ~LinechartWidget();

    static const int MIN_TIME_SCROLLBAR_VALUE = 0; ///< The minimum scrollbar value
    static const int MAX_TIME_SCROLLBAR_VALUE = 16383; ///< The maximum scrollbar value

public slots:
    void addCurve(const QString& curve, const QString& unit);
    void removeCurve(QString curve);
    /** @brief Recolor all curves */
    void recolor();
    /** @brief Set short names for curves */
    void setShortNames(bool enable);
    /** @brief Append data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, const QVariant& value, quint64 usec);
    /** @brief Hide curves which do not match the filter pattern */
    void filterCurves(const QString &filter);

    void toggleLogarithmicScaling(bool toggled);
    void takeButtonClick(bool checked);
    void setPlotWindowPosition(int scrollBarValue);
    void setPlotWindowPosition(quint64 position);
    void setPlotInterval(quint64 interval);
    /** @brief Start automatic updates once visible */
    void showEvent(QShowEvent* event);
    /** @brief Stop automatic updates once hidden */
    void hideEvent(QHideEvent* event);
    void setActive(bool active);
    /** @brief Select one MAV for curve display */
    void selectActiveSystem(int mav);
    /** @brief Set the number of values to average over */
    void setAverageWindow(int windowSize);
    /** @brief Start logging to file */
    void startLogging();
    /** @brief Stop logging to file */
    void stopLogging();
    /** @brief Refresh the view */
    void refresh();
    /** @brief Write the current configuration to disk */
    void writeSettings();
    /** @brief Read the current configuration from disk */
    void readSettings();
    /** @brief Select all curves */
    void selectAllCurves(bool all);
    /** @brief Sets the focus to the LineEdit for plot-filtering */
    void setPlotFilterLineEditFocus();

private slots:
    /** Called when the user changes the time scale combobox. */
    void timeScaleChanged(int index);
    /** @brief Toggles visibility of curve based on bool match if corresponding checkbox is not checked */
    void filterCurve(const QString &key, bool match);

protected:
    void addCurveToList(QString curve);
    void removeCurveFromList(QString curve);
    QToolButton* createButton(QWidget* parent);
    void createCurveItem(QString curve);
    void createLayout();
    /** @brief Get the name for a curve key */
    QString getCurveName(const QString& key, bool shortEnabled);

    int sysid;                            ///< ID of the unmanned system this plot belongs to
    LinechartPlot* activePlot;            ///< Plot for this system
    QReadWriteLock* curvesLock;           ///< A lock (mutex) for the concurrent access on the curves
    QReadWriteLock plotWindowLock;        ///< A lock (mutex) for the concurrent access on the window position

    int curveListIndex;
    int curveListCounter;                 ///< Counter of curves in curve list
    QMap<QString, QLabel*>* curveLabels;  ///< References to the curve labels
    QMap<QString, QLabel*> curveNameLabels;  ///< References to the curve labels
    QMap<QString, QString> curveNames;    ///< Full curve names
    QMap<QString, QLabel*>* curveMeans;   ///< References to the curve means
    QMap<QString, QLabel*>* curveMedians; ///< References to the curve medians
    QMap<QString, QWidget*> curveUnits;    ///< References to the curve units
    QMap<QString, QLabel*>* curveVariances; ///< References to the curve variances
    QMap<QString, int> intData;           ///< Current values for integer-valued curves
    QMap<QString, QWidget*> colorIcons;    ///< Reference to color icons
    QMap<QString, QCheckBox*> checkBoxes;    ///< Reference to checkboxes

    QWidget* curvesWidget;                ///< The QWidget containing the curve selection button
    QGridLayout* curvesWidgetLayout;      ///< The layout for the curvesWidget QWidget
    QScrollBar* scrollbar;                ///< The plot window scroll bar
    QSpinBox* averageSpinBox;             ///< Spin box to setup average window filter size

    QAction* addNewCurve;                 ///< Add curve candidate to the active curves

    QComboBox *timeScaleCmb;

    QToolButton* scalingLogButton;
    QToolButton* logButton;
    QPointer<QCheckBox> timeButton;

    QFile* logFile;
    unsigned int logindex;
    bool logging;
    quint64 logStartTime;
    QTimer* updateTimer;
    LogCompressor* compressor;
    QCheckBox* selectAllCheckBox;
    int selectedMAV; ///< The MAV for which plot items are accepted, -1 for all systems
    quint64 lastTimestamp;
    bool userGroundTimeSet;
    bool autoGroundTimeSet;
    static const int updateInterval = 1000; ///< Time between number updates, in milliseconds

    static const int MAX_CURVE_MENUITEM_NUMBER = 8;
    static const int PAGESTEP_TIME_SCROLLBAR_VALUE = (MAX_TIME_SCROLLBAR_VALUE - MIN_TIME_SCROLLBAR_VALUE) / 10;

private:
    Ui::linechart ui;
    void createActions();

signals:
    /**
         * @brief This signal is emitted if a curve is removed from the list
         *
         * @param curve The removed plot curve
         **/
    void curveRemoved(QString curve);

    /**
         * @brief This signal is emitted if a curve has been moved or added
         *
         * @param curve The moved or added curve
         * @param position The x-position of the curve (The centerline)
         **/
    void curveSet(QString curve, int position);

    /**
         * @brief This signal is emitted to change the visibility of a curve
         *
         * @param curve The changed curve
         * @pram visible The visibility
         **/
    void curveVisible(QString curve, bool visible);

    void plotWindowPositionUpdated(quint64 position);
    void plotWindowPositionUpdated(int position);

    /** @brief This signal is emitted once a logfile has been finished writing */
    void logfileWritten(QString fileName);

};

#endif // LINECHARTWIDGET_H
