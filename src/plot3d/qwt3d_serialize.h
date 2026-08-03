#ifndef QWT3D_SERIALIZE_H
#define QWT3D_SERIALIZE_H

#include <QDataStream>

#include "qwt3d_global.h"
#include "qwt3d_types.h"

class Qwt3DTheme;

// 序列化版本号
constexpr int qwt3d_serialize_version = 1;

// 序列化魔数（用于块级校验）
constexpr quint32 qwt3d_magic_mark  = 0x3D5A4BF1;  // '3D' + magic
constexpr quint32 qwt3d_magic_mark2 = 0xAA3D1234;
constexpr quint32 qwt3d_magic_mark3 = 0x3D5678AB;

// QDataStream 版本（与 2D DAChartSerialize 保持一致）
constexpr QDataStream::Version qwt3d_datastream_version = QDataStream::Qt_5_12;

// ============================================================
// 值类型序列化（本计划唯一声明和实现）
// 08 计划的 DAChart3DSerialize.h 通过 #include "qwt3d_serialize.h"
// 直接使用这些 operator，不再重复声明或实现。
// ============================================================

// Triple 序列化
QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const Triple& t);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, Triple& t);

// RGBA 序列化
QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const RGBA& c);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, RGBA& c);

// ParallelEpiped 序列化
QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const ParallelEpiped& p);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, ParallelEpiped& p);

// Qwt3DFunctionData 序列化
QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const Qwt3DFunctionData& d);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, Qwt3DFunctionData& d);

// Qwt3DParametricData 序列化
QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const Qwt3DParametricData& d);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, Qwt3DParametricData& d);

// ============================================================
// Qwt3DTheme 序列化（本计划唯一声明和实现）
// ============================================================

QWT3D_EXPORT QDataStream& operator<<(QDataStream& out, const Qwt3DTheme& theme);
QWT3D_EXPORT QDataStream& operator>>(QDataStream& in, Qwt3DTheme& theme);

// ============================================================
// Item/Plot 级 operator（Qwt3DPlotItem*, Qwt3DSurface*, Qwt3DBar*,
// Qwt3DLine*, Qwt3DPlot*）和 Qwt3DPlot::saveState/restoreState
// 不在本计划声明。由 08 计划在 DAFigure 模块（DAFIGURE_API）
// 负责声明和实现。
// ============================================================

#endif  // QWT3D_SERIALIZE_H
