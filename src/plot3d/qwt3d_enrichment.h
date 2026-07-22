#ifndef QWT3D_ENRICHMENT_H
#define QWT3D_ENRICHMENT_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"



class Qwt3DPlot;

/**
 * @brief Abstract base class for data dependent visible user objects
 * @details Enrichments provide a framework for user defined OpenGL objects. The base class has a pure virtual
 *          function clone(). 2 additional functions are per default empty and could also get a new
 *          implementation in derived classes. They can be used for initialization issues or actions not
 *          depending on the related primitive.
 */
class QWT3D_EXPORT Qwt3DEnrichment
{
public:
    // Type of the Enrichment - only VERTEXENRICHMENT's are defined at this moment
    enum TYPE
    {
        VERTEXENRICHMENT,
        EDGEENRICHMENT,
        FACEENRICHMENT,
        VOXELENRICHMENT
    };

    Qwt3DEnrichment() : plot(nullptr)
    {
    }
    virtual ~Qwt3DEnrichment()
    {
    }
    // The derived class should give back a new Derived(something) here
    virtual Qwt3DEnrichment* clone() const = 0;
    // Empty per default. Can be overwritten
    virtual void drawBegin() {};
    // Empty per default. Can be overwritten
    virtual void drawEnd() {};
    // Assign to existent plot
    virtual void assign(Qwt3DPlot const& pl)
    {
        plot = &pl;
    }
    // Overwrite
    virtual TYPE type() const = 0;

protected:
    const Qwt3DPlot* plot;
};

/**
 * @brief Abstract base class for vertex dependent visible user objects
 * @details VertexEnrichments introduce a specialized draw routine for vertex dependent data.
 *          draw() is called, when the Plot realizes its internal OpenGL data representation
 *          for every Vertex associated to his argument.
 */
class QWT3D_EXPORT Qwt3DVertexEnrichment : public Qwt3DEnrichment
{
public:
    Qwt3DVertexEnrichment() : Qwt3DEnrichment()
    {
    }
    // The derived class should give back a new Derived(something) here
    virtual Qwt3DEnrichment* clone() const = 0;
    // Overwrite this
    virtual void draw(Triple const&) = 0;
    // This gives VERTEXENRICHMENT
    TYPE type() const override
    {
        return Qwt3DEnrichment::VERTEXENRICHMENT;
    }
};


#endif  // QWT3D_ENRICHMENT_H
