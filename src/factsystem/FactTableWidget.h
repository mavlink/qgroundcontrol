#ifndef FACTTABLEWIDGET_H
#define FACTTABLEWIDGET_H

#include <QTableWidget>
#include "Fact.h"

class FactHandler;
class FactMetaDataHandler;
class FactTableModel;
class FactItemDelegate;
class UASInterface;

class FactTableWidget : public QTableWidget
{
    Q_OBJECT
    
public:
    FactTableWidget(QWidget* parent = NULL);
    void setup(UASInterface* uas);
    
    QString getColumnLabel(int column) const;
    Fact& getCellFact(int row, int col, QString& property, QString& cellValue, bool& factFound);
    bool findValueCell(const QString&      factIdSearch,
                       Fact::Provider_t    providerSearch,
                       int& row, int& col);
    
private slots:
    // Signals from UASParameterCommsMgr
    void _parameterListUpToDate(void);
    
private:
    void _setup(void);
    bool _parseColumnFormula(
                             const QString&     formula,
                             Fact::Provider_t&  provider,
                             QString&           property,
                             QString&           groupFilter);
    bool _parseCellFormula(
                           const QString&       formula,
                           Fact::Provider_t&    provider,
                           QString&             factId,
                           QString&             property);
    void _resizeToRowsAndColumns(void);
    
    // overrides from QWidget
    virtual void moveEvent(QMoveEvent * event);
    virtual void resizeEvent(QResizeEvent * event);

    QTableView          _tableView;         /// this is the actual table view which displays the data to the users
    FactTableModel*     _factTableModel;    /// MVC model for _tableView
    FactItemDelegate*   _factItemDelegate;  /// QItemDelegate for _tableView for custom editing behavior

    static const char*  _formulaChar;       /// character which identifies a formula versus just a string label
    static const char*  _formulaSeperator;  /// character which used to seperate formula parts
    
    QStringList         _columnProperties;  /// property for each column
    Fact::Provider_t    _provider;          /// fact provider for entire table
    QStringList         _filteredFactIds;   /// list of filtered fact ids
    
    UASInterface*       _uas;
    
    Fact                _nilFact;
};

#endif