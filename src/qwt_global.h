/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_GLOBAL_H
#define QWT_GLOBAL_H

#include <qglobal.h>
#include <memory>
#include "qwt_version_info.h"

#if defined(_MSC_VER) /* MSVC Compiler */
/* template-class specialization 'identifier' is already instantiated */
#pragma warning(disable : 4660)
/* inherits via dominance */
#pragma warning(disable : 4250)
#endif  // _MSC_VER

#ifdef QWT_DLL

#if defined(QWT_MAKEDLL)  // create a Qwt DLL library
#define QWT_EXPORT Q_DECL_EXPORT
#else  // use a Qwt DLL library
#define QWT_EXPORT Q_DECL_IMPORT
#endif

#endif  // QWT_DLL

#ifndef QWT_EXPORT
#define QWT_EXPORT
#endif

#define QWT_CONSTEXPR Q_DECL_CONSTEXPR

#if QT_VERSION >= 0x050000
#define QWT_OVERRIDE Q_DECL_OVERRIDE
#define QWT_FINAL Q_DECL_FINAL
#endif

#ifndef QWT_CONSTEXPR
#define QWT_CONSTEXPR
#endif

#ifndef QWT_OVERRIDE
#define QWT_OVERRIDE
#endif

#ifndef QWT_FINAL
#define QWT_FINAL
#endif

#ifndef QWT_DEBUG_DRAW
#define QWT_DEBUG_DRAW 1
#endif

#ifndef QWT_DEBUG_PRINT
#define QWT_DEBUG_PRINT 1
#endif
/**
 * @def QWT_DECLARE_PRIVATE
 * @brief 模仿Q_DECLARE_PRIVATE，但不用前置声明而是作为一个内部类
 *
 * 例如:
 *
 * @code
 * //header
 * class A
 * {
 *  QWT_DECLARE_PRIVATE(A)
 * };
 * @endcode
 *
 * 其展开效果为：
 *
 * @code
 * class A{
 *  class PrivateData;
 *  friend class A::PrivateData;
 *  std::unique_ptr< PrivateData > m_data;
 * }
 * @endcode
 *
 * 这样前置声明了一个内部类PrivateData，在cpp文件中建立这个内部类的实现
 *
 * @code
 * //cpp
 * class A::PrivateData{
 *  QWT_DECLARE_PUBLIC(A)
 *  PrivateData(A* p):m_data(p){
 *  }
 * };
 *
 * A::A():m_data(new PrivateData(this)){
 * }
 * @endcode
 *
 */
#ifndef QWT_DECLARE_PRIVATE
#define QWT_DECLARE_PRIVATE(classname)                                                                                 \
	class PrivateData;                                                                                                 \
	friend class classname::PrivateData;                                                                               \
    std::unique_ptr< PrivateData > m_data;                                                                             \
	inline PrivateData* d_func()                                                                                       \
	{                                                                                                                  \
        return (m_data.get());                                                                                         \
	}                                                                                                                  \
	inline const PrivateData* d_func() const                                                                           \
	{                                                                                                                  \
        return (m_data.get());                                                                                         \
	}
#endif

/**
 * @def QWT_DECLARE_PUBLIC
 * @brief 模仿Q_DECLARE_PUBLIC
 *
 * 配套QWT_DECLARE_PRIVATE使用
 */
#ifndef QWT_DECLARE_PUBLIC
#define QWT_DECLARE_PUBLIC(classname)                                                                                  \
	friend class classname;                                                                                            \
	classname* q_ptr { nullptr };                                                                                      \
	inline classname* q_func()                                                                                         \
	{                                                                                                                  \
		return (static_cast< classname* >(q_ptr));                                                                     \
	}                                                                                                                  \
	inline const classname* q_func() const                                                                             \
	{                                                                                                                  \
		return (static_cast< const classname* >(q_ptr));                                                               \
	}
#endif

/**
 * @def  QWT_PIMPL_CONSTRUCT
 *
 * 配套QWT_DECLARE_PRIVATE使用,在构造函数中构建privatedata
 */
#ifndef QWT_PIMPL_CONSTRUCT
#define QWT_PIMPL_CONSTRUCT m_data(std::make_unique< PrivateData >(this))
#endif

#endif
