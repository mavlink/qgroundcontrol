/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
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

#ifndef OSG_GRAPHICSCOSTESTIMATOR
#define OSG_GRAPHICSCOSTESTIMATOR

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <utility>

namespace osg
{

class Geometry;
class Texture;
class Program;
class Node;
class RenderInfo;

struct ClampedLinearCostFunction1D
{
    ClampedLinearCostFunction1D(double cost0=0.0, double dcost_di=0.0, unsigned int min_input=0):
        _cost0(cost0),
        _dcost_di(dcost_di),
        _min_input(min_input) {}

    void set(double cost0, double dcost_di, unsigned int min_input)
    {
        _cost0 = cost0;
        _dcost_di = dcost_di;
        _min_input = min_input;
    }

    double operator() (unsigned int input) const
    {
        return _cost0 + _dcost_di * double(input<=_min_input ? 0u : input-_min_input);
    }
    double _cost0;
    double _dcost_di;
    unsigned int _min_input;
};

/** Pair of double representing CPU and GPU times in seconds as first and second elements in std::pair. */
typedef std::pair<double, double> CostPair;


class OSG_EXPORT GeometryCostEstimator : public osg::Referenced
{
public:
    GeometryCostEstimator();
    void setDefaults();
    void calibrate(osg::RenderInfo& renderInfo);
    CostPair estimateCompileCost(const osg::Geometry* geometry) const;
    CostPair estimateDrawCost(const osg::Geometry* geometry) const;

protected:
    ClampedLinearCostFunction1D _arrayCompileCost;
    ClampedLinearCostFunction1D _primtiveSetCompileCost;

    ClampedLinearCostFunction1D _arrayDrawCost;
    ClampedLinearCostFunction1D _primtiveSetDrawCost;

    double _displayListCompileConstant;
    double _displayListCompileFactor;
};

class OSG_EXPORT TextureCostEstimator : public osg::Referenced
{
public:
    TextureCostEstimator();
    void setDefaults();
    void calibrate(osg::RenderInfo& renderInfo);
    CostPair estimateCompileCost(const osg::Texture* texture) const;
    CostPair estimateDrawCost(const osg::Texture* texture) const;

protected:
    ClampedLinearCostFunction1D _compileCost;
    ClampedLinearCostFunction1D _drawCost;
};


class OSG_EXPORT ProgramCostEstimator : public osg::Referenced
{
public:
    ProgramCostEstimator();
    void setDefaults();
    void calibrate(osg::RenderInfo& renderInfo);
    CostPair estimateCompileCost(const osg::Program* program) const;
    CostPair estimateDrawCost(const osg::Program* program) const;

protected:
    ClampedLinearCostFunction1D _shaderCompileCost;
    ClampedLinearCostFunction1D _linkCost;
    ClampedLinearCostFunction1D _drawCost;
};

class OSG_EXPORT GraphicsCostEstimator : public osg::Referenced
{
public:
    GraphicsCostEstimator();

    /** set defaults for computing the costs.*/
    void setDefaults();

    /** calibrate the costs of various compile and draw operations */
    void calibrate(osg::RenderInfo& renderInfo);

    CostPair estimateCompileCost(const osg::Geometry* geometry) const { return _geometryEstimator->estimateCompileCost(geometry); }
    CostPair estimateDrawCost(const osg::Geometry* geometry) const { return _geometryEstimator->estimateDrawCost(geometry); }

    CostPair estimateCompileCost(const osg::Texture* texture) const { return _textureEstimator->estimateCompileCost(texture); }
    CostPair estimateDrawCost(const osg::Texture* texture) const { return _textureEstimator->estimateDrawCost(texture); }

    CostPair estimateCompileCost(const osg::Program* program) const { return _programEstimator->estimateCompileCost(program); }
    CostPair estimateDrawCost(const osg::Program* program) const { return _programEstimator->estimateDrawCost(program); }

    CostPair estimateCompileCost(const osg::Node* node) const;
    CostPair estimateDrawCost(const osg::Node* node) const;

protected:

    virtual ~GraphicsCostEstimator();

    osg::ref_ptr<GeometryCostEstimator> _geometryEstimator;
    osg::ref_ptr<TextureCostEstimator> _textureEstimator;
    osg::ref_ptr<ProgramCostEstimator> _programEstimator;

};

}

#endif
