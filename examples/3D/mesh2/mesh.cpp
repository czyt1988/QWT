#include "functions.h"

using namespace std;

void createCone(TripleField &conepos, CellField &conecell)
{
    conepos.clear();
    conecell.clear();
    Cell cell;

    conepos.push_back(Triple(0, 0, 0));

    const unsigned int count = 17;
    double r1 = 0.7;
    double r2 = 0.9 * r1;

    double h1 = 2;
    double h2 = 1.03 * h1;

    unsigned i;

    Cell c1;
    // outer top border
    for (i = 0; i < count; ++i) {
        conepos.push_back(Triple(r1 * cos(2 * i * Qwt3D_PI / count),
                                 r1 * sin(2 * i * Qwt3D_PI / count), h1));
    }
    // inner top border
    for (i = 0; i < count; ++i) {
        conepos.push_back(Triple(r2 * cos(2 * i * Qwt3D_PI / count),
                                 r2 * sin(2 * i * Qwt3D_PI / count), h2));
        c1.push_back(count + i + 1);
    }
    // top polygon
    conecell.push_back(c1);

    // triangles
    Cell c2(3);
    for (i = 1; i < count + 1; ++i) {
        c2[0] = 0;
        c2[1] = i % count + 1;
        c2[2] = i;
        conecell.push_back(c2);
    }
    // quadrangles
    c2 = Cell(4);
    for (i = 1; i < count + 1; ++i) {
        c2[0] = i;
        c2[1] = i % count + 1;
        c2[2] = count + i % count + 1;
        c2[3] = count + i;
        conecell.push_back(c2);
    }
}
