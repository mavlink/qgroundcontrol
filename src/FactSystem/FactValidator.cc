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

#include "FactValidator.h"

FactValidator::FactValidator(QObject* parent) :
    QValidator(parent)
{

}
    
void FactValidator::fixup(QString& input) const
{
    Q_UNUSED(input);
}

FactValidator::State FactValidator::validate(QString& input, int& pos) const
{
    Q_UNUSED(input);
    Q_UNUSED(pos);
    
    return Acceptable;
}
