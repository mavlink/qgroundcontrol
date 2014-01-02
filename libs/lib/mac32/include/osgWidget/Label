/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2008 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

// Code by: Jeremy Moles (cubicool) 2007-2008

#ifndef OSGWIDGET_LABEL
#define OSGWIDGET_LABEL

#include <osgText/Text>
#include <osgWidget/Widget>
#include <osgWidget/Window>

namespace osgWidget {

class OSGWIDGET_EXPORT Label: public Widget
{
    public:

        META_Object   (osgWidget, Label);

        Label (const std::string& = "", const std::string& = "");
        Label (const Label&, const osg::CopyOp&);

        virtual void parented   (Window*);
        virtual void unparented (Window*);
        virtual void positioned ();

        void setLabel     (const std::string&);
        void setLabel     (const osgText::String&);
        void setFont      (const std::string&);
        void setFontSize  (unsigned int);
        void setFontColor (const Color&);
        void setShadow    (point_type);

        XYCoord getTextSize() const;

        std::string getLabel() const { return _text->getText().createUTF8EncodedString(); }

        void setFontColor(point_type r, point_type g, point_type b, point_type a) { setFontColor(Color(r, g, b, a)); }

        osgText::Text* getText() { return _text.get(); }

        const osgText::Text* getText() const { return _text.get(); }

    protected:

        osg::ref_ptr<osgText::Text> _text;
        unsigned int _textIndex;

        virtual void _calculateSize(const XYCoord&);

};

}

#endif
