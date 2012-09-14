/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of Line chart plot widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
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
    /** @brief Append int8 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, qint8 value, quint64 usec);
    /** @brief Append uint8 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, quint8 value, quint64 usec);
    /** @brief Append int16 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, qint16 value, quint64 usec);
    /** @brief Append uint16 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, quint16 value, quint64 usec);
    /** @brief Append int32 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, qint32 value, quint64 usec);
    /** @brief Append uint32 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, quint32 value, quint64 usec);
    /** @brief Append int64 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, qint64 value, quint64 usec);
    /** @brief Append uint64 data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, quint64 value, quint64 usec);
    /** @brief Append double data to the given curve. */
    void appendData(int uasId, const QString& curve, const QString& unit, double value, quint64 usec);
	
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
    QList<QString>* listedCurves;         ///< Curves listed
    QMap<QString, QLabel*>* curveLabels;  ///< References to the curve labels
    QMap<QString, QLabel*> curveNameLabels;  ///< References to the curve labels
    QMap<QString, QString> curveNames;    ///< Full curve names
    QMap<QString, QLabel*>* curveMeans;   ///< References to the curve means
    QMap<QString, QLabel*>* curveMedians; ///< References to the curve medians
    QMap<QString, QLabel*>* curveVariances; ///< References to the curve variances
    QMap<QString, int> intData;           ///< Current values for integer-valued curves
    QMap<QString, QWidget*> colorIcons;    ///< Reference to color icons

    QWidget* curvesWidget;                ///< The QWidget containing the curve selection button
    QGridLayout* curvesWidgetLayout;      ///< The layout for the curvesWidget QWidget
    QScrollBar* scrollbar;                ///< The plot window scroll bar
    QSpinBox* averageSpinBox;             ///< Spin box to setup average window filter size

    QAction* setScalingLogarithmic;       ///< Set logarithmic scaling
    QAction* setScalingLinear;            ///< Set linear scaling
    QAction* addNewCurve;                 ///< Add curve candidate to the active curves

    QMenu* curveMenu;
    QGridLayout* mainLayout;

    QToolButton* scalingLinearButton;
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
