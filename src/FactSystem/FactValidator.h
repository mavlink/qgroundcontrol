/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FactValidator_H
#define FactValidator_H

#include <QValidator>

class Fact;

/// QML Validator for Facts (Work In Progress)
///
/// The validator uses the FactMetaData to impose restrictions on the input. It is used as follows:
/// @code{.unparsed}
///     TextInput {
///         validator: FactValidator { fact: parameters["RC_MAP_THROTTLE"]; }
///     }
/// @endcode
class FactValidator : public QValidator
{
    Q_OBJECT
    
    Q_PROPERTY(Fact* fact READ fact WRITE setFact)
    
public:
    FactValidator(QObject* parent = NULL);
    
    // Property system methods
    
    /// Read accessor for fact property
    Fact* fact(void) { return _fact; }
    
    /// Write accessor for fact property
    void setFact(Fact* fact) { _fact = fact; }
    
    /// Override from QValidator
    virtual void fixup(QString& input) const;
    
    /// Override from QValidator
    virtual State validate(QString& input, int& pos) const;
    
private:
    Fact* _fact;    ///< Fact that the validator is working on
};

#endif