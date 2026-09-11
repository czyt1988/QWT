#ifndef QWT3D_AUTOSCALER_H
#define QWT3D_AUTOSCALER_H

#include <vector>
#include "qwt3d_global.h"
#include "qwt3d_autoptr.h"



/**
 * @brief Abstract base class for autoscaler
 */
class QWT3D_EXPORT Qwt3DAutoScaler
{
    friend class Qwt3DClonePtr< Qwt3DAutoScaler >;

protected:
    virtual Qwt3DAutoScaler* clone() const                                        = 0;
    virtual int execute(double& a, double& b, double start, double stop, int ivals) = 0;
    virtual ~Qwt3DAutoScaler()
    {
    }

private:
    void destroy() const
    {
        delete this;
    }
};

/**
 * @brief Automatic beautifying of linear scales
 */
class QWT3D_EXPORT Qwt3DLinearAutoScaler : public Qwt3DAutoScaler
{
    friend class Qwt3DLinearScale;

protected:
    QWT_DECLARE_PRIVATE(Qwt3DLinearAutoScaler)

    Qwt3DLinearAutoScaler();
    explicit Qwt3DLinearAutoScaler(std::vector< double >& mantisses);
    ~Qwt3DLinearAutoScaler() override;
    Qwt3DAutoScaler* clone() const override;
    int execute(double& a, double& b, double start, double stop, int ivals) override;

    // Copies state from another Qwt3DLinearAutoScaler (used by Qwt3DLinearScale::clone)
    void copyStateFrom(const Qwt3DLinearAutoScaler& other);

private:
    void init(double start, double stop, int ivals);
    double anchorvalue(double start, double mantisse, int exponent);
    int segments(int& l_intervals, int& r_intervals, double start, double stop, double anchor, double mantissa, int exponent);
};


#endif
