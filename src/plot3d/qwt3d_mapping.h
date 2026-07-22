#ifndef QWT3D_MAPPING_H
#define QWT3D_MAPPING_H

#include <qstring.h>
#include "qwt3d_global.h"
#include "qwt3d_types.h"



/**
 * @brief Abstract base class for general mappings
 */
class QWT3D_EXPORT Mapping
{

public:
    // Destructor
    virtual ~Mapping()
    {
    }
    // Descriptive String
    virtual QString name() const
    {
        return QString("");
    }
};


#endif  // QWT3D_MAPPING_H