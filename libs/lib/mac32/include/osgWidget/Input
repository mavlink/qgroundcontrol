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

#ifndef OSGWIDGET_INPUT
#define OSGWIDGET_INPUT

#include <osgWidget/Label>

namespace osgWidget {

// This is a string of values we use to try and determine the best Y
// descent value (yoffset); you're welcome to use what works best for
// your font.
const std::string DESCENT_STRING("qpl");

class OSGWIDGET_EXPORT Input: public Label
{
    public:

        Input(const std::string& = "", const std::string& = "", unsigned int = 20);

        virtual void parented   (Window*);
        virtual void positioned ();

        virtual bool focus   (const WindowManager*);
        virtual bool unfocus (const WindowManager*);
        virtual bool keyUp   (int, int, const WindowManager*);
        virtual bool keyDown (int, int, const WindowManager*);
        virtual bool mouseDrag (double, double, const WindowManager*);
        virtual bool mousePush (double x, double y, const WindowManager*);
        virtual bool mouseRelease (double, double, const WindowManager*);

        void         setCursor            (Widget*);
        unsigned int calculateBestYOffset (const std::string& = "qgl");
        void         clear();

        void setXOffset(point_type xo) {
            _xoff = xo;
        }

        void setYOffset(point_type yo) {
            _yoff = yo;
        }

        void setXYOffset(point_type xo, point_type yo) {
            _xoff = xo;
            _yoff = yo;
        }

        osg::Drawable* getCursor() {
            return _cursor.get();
        }

        const osg::Drawable* getCursor() const {
            return _cursor.get();
        }

        point_type getXOffset() const {
            return _xoff;
        }

        point_type getYOffset() const {
            return _yoff;
        }

        XYCoord getXYOffset() const {
            return XYCoord(_xoff, _yoff);
        }

    protected:
        virtual void _calculateSize(const XYCoord&);

        void _calculateCursorOffsets();

        point_type               _xoff;
        point_type               _yoff;

        unsigned int             _index;
        unsigned int             _size;
        unsigned int             _cursorIndex;
        unsigned int             _maxSize;

        std::vector<point_type>  _offsets;
        std::vector<unsigned int> _wordsOffsets;
        std::vector<point_type>  _widths;
        osg::ref_ptr<Widget>     _cursor;

        bool                     _insertMode;        // Insert was pressed --> true --> typing will overwrite existing text

        osg::ref_ptr<Widget>     _selection;
        unsigned int             _selectionStartIndex;
        unsigned int             _selectionEndIndex;
        unsigned int             _selectionIndex;

        point_type               _mouseClickX;
};

}

#endif
