#ifndef QWT3D_SCALE_H
#define QWT3D_SCALE_H

#include <vector>
#include <qstring.h>
#include "qwt3d_types.h"
#include "qwt3d_autoptr.h"



/**
 * @brief Non-visual scale class encapsulating tic generation
 * @details The class encapsulates non-visual scales. It is utilized by Axis and also
 *          collaborates closely with AutoScaler. A Scale allows control over all aspects
 *          of tic generation including arbitrary transformations of tic values into corresponding
 *          strings. The strings contain what eventually will be shown as tic labels.
 *          Standard linear and logarithmic scales have been integrated into the Axis interface.
 *          User-defined axes can be derived from Scale, LinearScale et al.
 */
class QWT3D_EXPORT Scale
{
    friend class Qwt3DAxis;
    friend class ClonePtr< Scale >;

protected:
    QWT_DECLARE_PRIVATE(Scale)

    Scale();
    virtual ~Scale();
    virtual QString ticLabel(unsigned int idx) const;

    virtual void setLimits(double start, double stop);
    // Sets number of major intervals
    virtual void setMajors(int val);
    // Sets number of minor intervals per major interval
    virtual void setMinors(int val);
    virtual void setMajorLimits(double start, double stop);

    // Returns major intervals
    int majors() const;
    // Returns minor intervals
    int minors() const;

    // Derived classes should return a new heap based object here
    virtual Scale* clone() const = 0;
    // This function should setup the 2 vectors for major and minor positions
    virtual void calculate() = 0;
    virtual int autoscale(double& a, double& b, double start, double stop, int ivals);

    // Returns const reference to major tic positions
    const std::vector< double >& majorTicks() const;
    // Returns const reference to minor tic positions
    const std::vector< double >& minorTicks() const;

    // Copies Scale base state from another Scale (used by derived clone())
    void copyFrom(const Scale& other);

private:
    void destroy() const;
};

/**
 * @brief The standard (1:1) mapping class for axis numbering
 */
class QWT3D_EXPORT LinearScale : public Scale
{
    friend class Qwt3DAxis;
    friend class ClonePtr< Scale >;

protected:
    QWT_DECLARE_PRIVATE(LinearScale)

    LinearScale();
    ~LinearScale() override;
    int autoscale(double& a, double& b, double start, double stop, int ivals) override;
    // Returns a new heap based object utilized from ClonePtr
    Scale* clone() const override;
    void calculate() override;
};

/**
 * @brief Log10 scale
 */
class QWT3D_EXPORT LogScale : public Scale
{
    friend class Qwt3DAxis;
    friend class ClonePtr< Scale >;

protected:
    QString ticLabel(unsigned int idx) const override;
    void setMinors(int val) override;
    // Standard ctor
    LogScale();
    ~LogScale() override;
    // Returns a new heap based object utilized from ClonePtr
    Scale* clone() const override;
    void calculate() override;

private:
    void setupCounter(double& k, int& step);
};


#endif  // QWT3D_SCALE_H