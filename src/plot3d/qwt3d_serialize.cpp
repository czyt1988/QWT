#include "qwt3d_serialize.h"

#include "qwt3d_theme.h"

#include <QDataStream>
#include <vector>

// ============================================================
// Triple 序列化
// ============================================================

/**
 * @brief 序列化 Triple 到数据流
 * @param[out] out 数据流
 * @param[in] t Triple 值
 * @details 直接写入 x / y / z 三个 double 值，无魔数校验（简单值类型）。
 */
QDataStream& operator<<(QDataStream& out, const Triple& t)
{
    out << t.x << t.y << t.z;
    return out;
}

/**
 * @brief 从数据流反序列化 Triple
 * @param[in] in 数据流
 * @param[out] t Triple 值
 */
QDataStream& operator>>(QDataStream& in, Triple& t)
{
    in >> t.x >> t.y >> t.z;
    return in;
}

// ============================================================
// RGBA 序列化
// ============================================================

/**
 * @brief 序列化 RGBA 到数据流
 * @param[out] out 数据流
 * @param[in] c RGBA 值
 * @details 直接写入 r / g / b / a 四个 double 值。
 */
QDataStream& operator<<(QDataStream& out, const RGBA& c)
{
    out << c.r << c.g << c.b << c.a;
    return out;
}

/**
 * @brief 从数据流反序列化 RGBA
 * @param[in] in 数据流
 * @param[out] c RGBA 值
 */
QDataStream& operator>>(QDataStream& in, RGBA& c)
{
    in >> c.r >> c.g >> c.b >> c.a;
    return in;
}

// ============================================================
// ParallelEpiped 序列化
// ============================================================

/**
 * @brief 序列化 ParallelEpiped 到数据流
 * @param[out] out 数据流
 * @param[in] p ParallelEpiped 值
 * @details 通过 Triple 的 operator<< 序列化 minVertex 和 maxVertex。
 */
QDataStream& operator<<(QDataStream& out, const ParallelEpiped& p)
{
    out << p.minVertex << p.maxVertex;
    return out;
}

/**
 * @brief 从数据流反序列化 ParallelEpiped
 * @param[in] in 数据流
 * @param[out] p ParallelEpiped 值
 */
QDataStream& operator>>(QDataStream& in, ParallelEpiped& p)
{
    in >> p.minVertex >> p.maxVertex;
    return in;
}

// ============================================================
// Qwt3DFunctionData 序列化
// ============================================================

/**
 * @brief 序列化 Qwt3DFunctionData 到数据流
 * @param[out] out 数据流
 * @param[in] d 函数数据
 * @details 格式：version(quint32) + magic(quint32) + columns(uint) + rows(uint)
 *          + minx(double) + maxx(double) + miny(double) + maxy(double)
 *          + magic2(quint32) + z_vector_size(quint32) + z_data(double*) + magic3(quint32)
 *          z 矩阵以嵌套 vector 结构写入：先写外层大小（columns），
 *          再逐列写入行数（rowSize）+ 该列的 double 值。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DFunctionData& d)
{
    out << static_cast<quint32>(qwt3d_serialize_version) << qwt3d_magic_mark;
    out << static_cast<quint32>(d.columns) << static_cast<quint32>(d.rows);
    out << d.minx << d.maxx << d.miny << d.maxy;
    out << qwt3d_magic_mark2;

    // 写入 z 矩阵数据
    quint32 totalSize = static_cast<quint32>(d.z.size());
    out << totalSize;
    for (const auto& row : d.z) {
        quint32 rowSize = static_cast<quint32>(row.size());
        out << rowSize;
        for (double val : row) {
            out << val;
        }
    }
    out << qwt3d_magic_mark3;
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DFunctionData
 * @param[in] in 数据流
 * @param[out] d 函数数据
 * @details 读取格式见 operator<< 的注释。遇到魔数不匹配时设置 ReadCorruptData 状态。
 */
QDataStream& operator>>(QDataStream& in, Qwt3DFunctionData& d)
{
    quint32 version;
    quint32 magic;
    in >> version >> magic;
    if (qwt3d_magic_mark != magic || version != qwt3d_serialize_version) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    quint32 columns;
    quint32 rows;
    in >> columns >> rows;
    d.columns = columns;
    d.rows = rows;
    in >> d.minx >> d.maxx >> d.miny >> d.maxy;

    quint32 magic2;
    in >> magic2;
    if (qwt3d_magic_mark2 != magic2) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    quint32 totalSize;
    in >> totalSize;
    d.z.clear();
    d.z.resize(totalSize);
    for (auto& row : d.z) {
        quint32 rowSize;
        in >> rowSize;
        row.resize(rowSize);
        for (auto& val : row) {
            in >> val;
        }
    }

    quint32 magic3;
    in >> magic3;
    if (qwt3d_magic_mark3 != magic3) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }
    return in;
}

// ============================================================
// Qwt3DParametricData 序列化
// ============================================================

/**
 * @brief 序列化 Qwt3DParametricData 到数据流
 * @param[out] out 数据流
 * @param[in] d 参数曲面数据
 * @details 格式：version(quint32) + magic(quint32) + columns(uint) + rows(uint)
 *          + uperiodic(bool) + vperiodic(bool) + magic2(quint32)
 *          + vertices_vector_size(quint32) + vertices_data(Triple*) + magic3(quint32)
 *          vertices 矩阵以嵌套 vector 结构写入：先写外层大小，
 *          再逐列写入行数（rowSize）+ 该列的 Triple 值（通过 Triple operator<<）。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DParametricData& d)
{
    out << static_cast<quint32>(qwt3d_serialize_version) << qwt3d_magic_mark;
    out << static_cast<quint32>(d.columns) << static_cast<quint32>(d.rows);
    out << d.uperiodic << d.vperiodic;
    out << qwt3d_magic_mark2;

    // 写入 vertices 矩阵数据
    quint32 totalSize = static_cast<quint32>(d.vertices.size());
    out << totalSize;
    for (const auto& row : d.vertices) {
        quint32 rowSize = static_cast<quint32>(row.size());
        out << rowSize;
        for (const Triple& t : row) {
            out << t;
        }
    }
    out << qwt3d_magic_mark3;
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DParametricData
 * @param[in] in 数据流
 * @param[out] d 参数曲面数据
 */
QDataStream& operator>>(QDataStream& in, Qwt3DParametricData& d)
{
    quint32 version;
    quint32 magic;
    in >> version >> magic;
    if (qwt3d_magic_mark != magic || version != qwt3d_serialize_version) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    quint32 columns;
    quint32 rows;
    in >> columns >> rows;
    d.columns = columns;
    d.rows = rows;
    in >> d.uperiodic >> d.vperiodic;

    quint32 magic2;
    in >> magic2;
    if (qwt3d_magic_mark2 != magic2) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    quint32 totalSize;
    in >> totalSize;
    d.vertices.clear();
    d.vertices.resize(totalSize);
    for (auto& row : d.vertices) {
        quint32 rowSize;
        in >> rowSize;
        row.resize(rowSize);
        for (auto& t : row) {
            in >> t;
        }
    }

    quint32 magic3;
    in >> magic3;
    if (qwt3d_magic_mark3 != magic3) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }
    return in;
}

// ============================================================
// Qwt3DTheme 序列化
// ============================================================

/**
 * @brief 序列化 Qwt3DTheme 到数据流
 * @param[out] out 数据流
 * @param[in] theme 主题对象
 * @details 序列化全部 21 个属性。格式：
 *          version(quint32) + magic(quint32)
 *          + backgroundColor(RGBA)
 *          + meshColor(RGBA) + meshLineWidth(double) + smoothMesh(bool)
 *          + dataColorPreset(QString)
 *          + axesColor(RGBA) + numberColor(RGBA) + labelColor(RGBA)
 *          + gridLinesColor(RGBA) + interiorGridLinesColor(RGBA)
 *          + interiorGridMajorWidth(double) + interiorGridMinorWidth(double)
 *          + titleColor(RGBA) + titleFontFamily(QString) + titleFontSize(int) + titleFontBold(bool)
 *          + lightingPreset(int) + shading(int) + plotStyle(int)
 *          + shininess(double) + specularIntensity(double)
 *          + magic2(quint32)
 *          枚举类型（LightingPreset / SHADINGSTYLE / PLOTSTYLE）以 int 序列化。
 */
QDataStream& operator<<(QDataStream& out, const Qwt3DTheme& theme)
{
    out << static_cast<quint32>(qwt3d_serialize_version) << qwt3d_magic_mark;

    // Background
    out << theme.backgroundColor();

    // Mesh
    out << theme.meshColor() << theme.meshLineWidth() << theme.smoothMesh();

    // Data Color
    out << theme.dataColorPreset();

    // Coordinate System
    out << theme.axesColor() << theme.numberColor() << theme.labelColor()
        << theme.gridLinesColor() << theme.interiorGridLinesColor()
        << theme.interiorGridMajorWidth() << theme.interiorGridMinorWidth();

    // Title
    out << theme.titleColor() << theme.titleFontFamily()
        << static_cast<qint32>(theme.titleFontSize()) << theme.titleFontBold();

    // Lighting
    out << static_cast<qint32>(theme.lightingPreset());

    // Shading
    out << static_cast<qint32>(theme.shading());

    // Plot Style
    out << static_cast<qint32>(theme.plotStyle());

    // Material
    out << theme.shininess() << theme.specularIntensity();

    out << qwt3d_magic_mark2;
    return out;
}

/**
 * @brief 从数据流反序列化 Qwt3DTheme
 * @param[in] in 数据流
 * @param[out] theme 主题对象
 * @details 读取格式见 operator<< 的注释。枚举值通过临时 qint32 读取后 static_cast 转换。
 *          魔数不匹配时设置 QDataStream::ReadCorruptData 状态。
 */
QDataStream& operator>>(QDataStream& in, Qwt3DTheme& theme)
{
    quint32 version;
    quint32 magic;
    in >> version >> magic;
    if (qwt3d_magic_mark != magic || version != qwt3d_serialize_version) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    RGBA rgba;
    double dval;
    bool bval;
    QString sval;
    qint32 ival;

    // Background
    in >> rgba;
    theme.setBackgroundColor(rgba);

    // Mesh
    in >> rgba;
    theme.setMeshColor(rgba);
    in >> dval;
    theme.setMeshLineWidth(dval);
    in >> bval;
    theme.setSmoothMesh(bval);

    // Data Color
    in >> sval;
    theme.setDataColorPreset(sval);

    // Coordinate System
    in >> rgba;
    theme.setAxesColor(rgba);
    in >> rgba;
    theme.setNumberColor(rgba);
    in >> rgba;
    theme.setLabelColor(rgba);
    in >> rgba;
    theme.setGridLinesColor(rgba);
    in >> rgba;
    theme.setInteriorGridLinesColor(rgba);
    in >> dval;
    theme.setInteriorGridMajorWidth(dval);
    in >> dval;
    theme.setInteriorGridMinorWidth(dval);

    // Title
    in >> rgba;
    theme.setTitleColor(rgba);
    in >> sval;
    theme.setTitleFontFamily(sval);
    in >> ival;
    theme.setTitleFontSize(ival);
    in >> bval;
    theme.setTitleFontBold(bval);

    // Lighting
    in >> ival;
    theme.setLightingPreset(static_cast<Qwt3DTheme::LightingPreset>(ival));

    // Shading
    in >> ival;
    theme.setShading(static_cast<SHADINGSTYLE>(ival));

    // Plot Style
    in >> ival;
    theme.setPlotStyle(static_cast<PLOTSTYLE>(ival));

    // Material
    in >> dval;
    theme.setShininess(dval);
    in >> dval;
    theme.setSpecularIntensity(dval);

    quint32 magic2;
    in >> magic2;
    if (qwt3d_magic_mark2 != magic2) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }
    return in;
}
