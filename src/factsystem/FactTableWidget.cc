#include "FactTableWidget.h"
#include "FactTableModel.h"
#include "FactHandler.h"
#include "FactItemDelegate.h"
#include "UASInterface.h"

#include <QHeaderView>
#include <QDebug>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QMetaProperty>

const char*  FactTableWidget::_formulaChar = "=";
const char*  FactTableWidget::_formulaSeperator = ":";

FactTableWidget::FactTableWidget(QWidget* parent) :
    QTableWidget(parent),
    _tableView(parent),
    _factTableModel(NULL),
    _factItemDelegate(NULL),
    _uas(NULL)
{
    _tableView.verticalHeader()->setVisible(false);
}

void FactTableWidget::setup(UASInterface* uas)
{
    Q_ASSERT(uas);

    _uas = uas;
    
    // We can't handle live parameter adds yet, so we need to know when the full set of params is ready
    bool connected = connect(_uas->getParamManager(), SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
    Q_ASSERT_X(connected, "QGCUASParamManager::parameterListUpToDate", "connect failed");
}

void FactTableWidget::_parameterListUpToDate(void)
{
    disconnect(_uas->getParamManager(), SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
    
    _setup();
    
    QRect rectFrame = frameGeometry();
    _tableView.move(rectFrame.topLeft());
    _tableView.resize(rectFrame.size());
    
    // When the parameters are ready we set the model and item delegate onto our QTableView
    _factTableModel = new FactTableModel(_uas, this, this);
    Q_CHECK_PTR(_factTableModel);

    _factItemDelegate = new FactItemDelegate(_uas, this, this);
    Q_CHECK_PTR(_factItemDelegate);

    _tableView.setModel(_factTableModel);
    _tableView.setItemDelegate(_factItemDelegate);
    
    //_tableView.horizontalHeader()->setVisible(horizontalHeader()->isVisible());
    _tableView.horizontalHeader()->setStretchLastSection(true);
    
    _resizeToRowsAndColumns();
    
    _uas->getFactRuleHandler().evaluateRules();
}

void FactTableWidget::_setup(void)
{
    QString groupFilter;
    
    // Parse the column labels
    
    // Column headers must either be all formulas, or not all formulas
    bool nonFormulaFound = false;
    bool formulaFound = false;
    
    int cColumn = columnCount();
    Q_ASSERT(cColumn > 0);
    
    for (int i=0; i<cColumn; i++) {
        QTableWidgetItem* columnItem = horizontalHeaderItem(i);
        QString formula = columnItem->text();
        
        // Look for a formula
        
        Fact::Provider_t    provider;
        QString             property;
        QString             parsedGroupFilter;
        
        bool isValidFormula = _parseColumnFormula(formula, provider, property, parsedGroupFilter);
        
        if (isValidFormula) {
            Q_ASSERT_X(!nonFormulaFound,
                       "FactTableWidget::_setup",
                       QString("Mix of formulas and non-formuals found. col(%1) formula(%2)").arg(i).arg(formula).toAscii().constData());
            formulaFound = true;

            // Put the visible part of the label back. Uppercase the first letter of the property to make it look better
            columnItem->setText(property.leftRef(1).toString().toUpper() + property.rightRef(property.length() - 1).toString());

            _provider = provider;
            _columnProperties += property;

            // Save the group filter
            if (!parsedGroupFilter.isEmpty()) {
                groupFilter = parsedGroupFilter;
            }
        } else {
            Q_ASSERT_X(!formulaFound,
                       "FactTableWidget::_setup",
                       QString("Mix of formulas and non-formulas found. col(%1) formula(%2)").arg(i).arg(formula).toAscii().constData());
            nonFormulaFound = true;
        }
    }
    
    if (!groupFilter.isEmpty()) {
        Q_ASSERT(!nonFormulaFound);
        Q_ASSERT(_uas);
        _filteredFactIds = _uas->getFactHandler().getFactIdsForGroup(_provider, groupFilter);
        setRowCount(_filteredFactIds.count());
    } else {
        if (formulaFound) {
            // Otherwise we are showing all parameter facts. Just need to set the correct number of rows
            Q_ASSERT(_uas);
            setRowCount(_uas->getFactHandler().getFactCount(_provider));
        }
    }
}

/// Parses the specified text looking for a cell formula. Column formulas take the form:
///     '=property:P.' or '=property:P.groupfilter'
/// where you can replace P. with any subtype prefix.
///     @param[in] formula string to parse
///     @param[out] provider parsed provider
///     @param[out] property parsed property
///     @param[out] groupFilter group filter (isEmpty() for none)
/// @return true if valid formula was found, false it not valid formula
bool FactTableWidget::_parseColumnFormula(
    const QString&      formula,
    Fact::Provider_t&   provider,
    QString&            property,
    QString&            groupFilter)
{
    bool formulaFound = formula.startsWith(_formulaChar);
    
    if (formulaFound) {
        
        // Parse the formula into parts
        
        QString strippedFormula = formula.right(formula.count() - 1); // strip off formula char
        QStringList formulaParts = strippedFormula.split(_formulaSeperator); // split the two parts of the formula
        Q_ASSERT_X(formulaParts.count() > 0 && formulaParts.count() <= 2,
                   "FactTableWidget::_parseColumnFormula",
                   QString("Badly formed formula:'%1'").arg(formula).toAscii().constData());
        if (formulaParts.count() == 0 || formulaParts.count() > 2) {
            return false;
        }

        property = formulaParts[0];
        if (!Fact::isValidProperty(property)) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseColumnFormula",
                       QString("Unknown property(%1) in formula(%2)").arg(property).arg(formula).toAscii().constData());
            return false;
        }
        
        // Parse provider and group filter
        bool correctFormat = Fact::parseProviderFactIdReference(formulaParts[1], provider, groupFilter);
        if (!correctFormat) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseColumnFormula",
                       QString("Incorrect fact id format in formula(%1)").arg(formula).toAscii().constData());
            return false;
        }
        
        if (!groupFilter.isEmpty() && !Fact::isValueProperty(property)) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseColumnFormula",
                       QString("Only value property supports group filter in column header. formula(%1)").arg(formula).toAscii().constData());
            return false;
        }

    }
    
    return formulaFound;
}

/// Parses the specified text looking for a cell formula. Cell formulas take the form:
///     '=P.factid:property'
/// where you can replace P. with any subtype prefix.
///     @param[in] formula string to parse
///     @param[out] provider parsed provider
///     @param[out] factId parsed fact id
///     @param[out] property parsed property
/// @return true if valid formula was found, false it not valid formula r no formula
bool FactTableWidget::_parseCellFormula(
    const QString&      formula,
    Fact::Provider_t&   provider,
    QString&            factId,
    QString&            property)
{
    bool formulaFound = formula.startsWith(_formulaChar);
    
    if (formulaFound) {
        
        // Parse the formula into parts
        
        QString strippedFormula = formula.right(formula.count() - 1); // strip off formula char
        QStringList formulaParts = strippedFormula.split(_formulaSeperator); // split the two parts of the formula
        Q_ASSERT_X(formulaParts.count() == 2,
                   "FactTableWidget::_parseCellFormula",
                   QString("Cell formula must be in format '%1P.factid%2dimension' formula(%3)").arg(_formulaChar).arg(_formulaSeperator).arg(formula).toAscii().constData());
        if (formulaParts.count() != 2) {
            return false;
        }
        
        // Parse provider and fact id
        bool correctFormat = Fact::parseProviderFactIdReference(formulaParts[1], provider, factId);
        if (!correctFormat) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseCellFormula",
                       QString("Incorrect fact id format in formula(%1)").arg(formula).toAscii().constData());
            return false;
        }
        
        // Validate fact id
        Q_ASSERT(_uas);
        if (!_uas->getFactHandler().containsFact(provider, factId)) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseCellFormula",
                       QString("Unknown fact in formula(%1)").arg(formula).toAscii().constData());
            return false;
        }
        
        // Validate property
        property = formulaParts[0];
        if (!Fact::isValidProperty(property)) {
            Q_ASSERT_X(false,
                       "FactTableWidget::_parseCellFormula",
                       QString("Unknown property in formula(%1) property(%2)").arg(formula).arg(property).toAscii().constData());
            return false;
        }
    }
    
    return formulaFound;
}

/// @return Returns the column heading label for the specified column. This is the filtered human
/// readable version of the column header label after parsing.
QString FactTableWidget::getColumnLabel(int column) const
{
    Q_ASSERT(column >= 0 && column < columnCount());
    
    QTableWidgetItem* item = horizontalHeaderItem(column);
    Q_ASSERT(item);
    return item->text();
}

/// Returns the fact information associated with the specified cell.
///     @param[in] row row to return information on
///     @param[in] col column to return information on
///     @param[out] property fact property name for cell
///     @param[out] cellValue contents of cell
///     @param[out] true: fact returned, false: plain text in cell, only cellValue is returned
/// @return Returns fact associated with cell
Fact& FactTableWidget::getCellFact(int row, int col, QString& property, QString& cellValue, bool& factFound)
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(col >= 0 && col < columnCount());
    Q_ASSERT(_columnProperties.count() == 0 || col < _columnProperties.count());
    
    QString             factId;
    Fact::Provider_t    provider;
    
    Q_ASSERT(_uas);
    FactHandler& factHandler = _uas->getFactHandler();
    
    factFound = false;
    property.clear();
    cellValue.clear();
    
    if (_columnProperties.count() == 0) {
        // No column properties means formulas in each individual cell.
        
        QTableWidgetItem* cellItem = item(row, col);
        
        if (cellItem) {
            // We got an item, so there is either a formula in here or just display text
            
            cellValue = cellItem->text();
            
            bool formulaFound = _parseCellFormula(cellValue, provider, factId, property);
            if (!formulaFound) {
                goto NoFact;
            }

            Q_ASSERT(!factId.isEmpty());
            Q_ASSERT(!property.isEmpty());
        } else {
            // If we don't get an item back it means it's an empty cell
            goto NoFact;
        }
    } else {
        // We have column properties which means the entire column is set to the specified property
        property = _columnProperties[col];
        
        if (_filteredFactIds.count()) {
            Q_ASSERT(row < _filteredFactIds.count());
            
            factId = _filteredFactIds[row];
            provider = _provider;
        } else {
            factId = factHandler.getFactIdByIndex(_provider, row);
            provider = _provider;
        }
    }
    
    factFound = true;
    return factHandler.getFact(provider, factId);

NoFact:
    factFound = false;
    return _nilFact;
}

/// Returns the index which contains the value dimension of the specified fact
///     @param[in] factId fact id to look for
///     @param[in] provider fact provider to look for
///     @param[in] row row where cell was found
///     @param[in] col column where cell was found
/// @return true: formula found in cell and parsed, false: plain text in cell, only cellValue is returned
bool FactTableWidget::findValueCell(const QString&      factIdSearch,
                                    Fact::Provider_t    providerSearch,
                                    int& row, int& col)
{
    if (_columnProperties.count() == 0) {
        // No column properties means formulas in each individual cell. We have to loop over the whole table.
        
        for (int r=0; r<rowCount(); r++) {
            for (int c=0; c<columnCount(); c++) {
                QTableWidgetItem* cellItem = item(row, col);
                
                if (cellItem) {
                    // We got an item, so there is either a formula in here or just display text
                    
                    QString formula = cellItem->text();
                    
                    Fact::Provider_t    provider;
                    QString             factId;
                    QString             property;
                    
                    bool formulaFound = _parseCellFormula(formula, provider, factId, property);
                    
                    if (formulaFound) {
                        Q_ASSERT(!factId.isEmpty());

                        if (factId == factIdSearch && provider == providerSearch) {
                            row = r;
                            col = c;
                            return true;
                        }
                    }
                }
            }
        }
    } else {
        // We have column properties which means the entire column is set to the specified property.
        // Find the value column.

        const char* valuePropertyName = NULL;
        for (int i=0; i<Fact::staticMetaObject.propertyCount(); i++) {
            QMetaProperty metaProperty = Fact::staticMetaObject.property(i);
            if (metaProperty.isUser()) {
                valuePropertyName = metaProperty.name();
                break;
            }
        }
        Q_ASSERT(valuePropertyName);

        int c = _columnProperties.indexOf(valuePropertyName);
        if (c != -1) {
            if (_filteredFactIds.count()) {
                int r = _filteredFactIds.indexOf(factIdSearch);
                if (r != -1) {
                    row = r;
                    col = c;
                    return true;
                }
            } else {
                Q_ASSERT(_uas);
                int r = 0;
                QStringListIterator i = _uas->getFactHandler().factIdIterator(_provider);
                while (i.hasNext()) {
                    QString factId = i.next();
                    if (factId == factIdSearch) {
                        row = r;
                        col = c;
                        return true;
                    }
                    r++;
                }
            }
        }
    }
    
    return false;
}

void FactTableWidget::_resizeToRowsAndColumns(void)
{
#if 0
    QSize newSize;
    
    // We only shrink, not grow
    
    newSize = size();
    int newHeight = horizontalHeader()->height() + (rowHeight(0) * rowCount()) + (frameWidth() * 2);
    if (newHeight < newSize.height()) {
        newSize.setHeight(newHeight);
        resize(newSize);
        _tableView.resize(newSize);
    }
#endif
}

void FactTableWidget::moveEvent(QMoveEvent * event)
{
    _tableView.move(event->pos());
}

void FactTableWidget::resizeEvent(QResizeEvent * event)
{
    _tableView.resize(frameGeometry().size());
}
