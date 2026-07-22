#include "qbitmap.h"

#include "qwt3d_color.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "enrichments.h"

/////////////////////////////////////////////////////////////////
//
//   Bar
//
/////////////////////////////////////////////////////////////////

Bar::Bar()
{
    configure(0, 1);
}

Bar::Bar(double rad, double level)
{
    configure(rad, level);
}

void Bar::configure(double rad, double level)
{
    plot = nullptr;
    radius_ = rad;
    level_ = level;
}

// Stubs — full VBO+shader implementation deferred.
// The Bar enrichment draws 3D bars at surface vertices.
// When implemented: accumulate bar geometry in a vertex array,
// upload to VBO in drawEnd(), render with polygon/line shaders.
void Bar::drawBegin()
{
    diag_ = 0;
}

void Bar::drawEnd() { }

void Bar::draw(Triple const &)
{
}

void Label3D::draw(Triple const &, double, double)
{
}
