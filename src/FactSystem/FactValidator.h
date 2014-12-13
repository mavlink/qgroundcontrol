/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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