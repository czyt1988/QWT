#ifndef QWT3D_AUTOSCALER_H
#define QWT3D_AUTOSCALER_H

#include <vector>
#include "qwt3d_global.h"
#include "qwt3d_autoptr.h"



/**
 * @brief Abstract base class for autoscaler
 */
class QWT3D_EXPORT AutoScaler
{
    friend class ClonePtr< AutoScaler >;

protected:
    virtual AutoScaler* clone() const                                               = 0;
    virtual int execute(double& a, double& b, double start, double stop, int ivals) = 0;
    virtual ~AutoScaler()
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
class QWT3D_EXPORT LinearAutoScaler : public AutoScaler
{
    friend class LinearScale;

protected:
    QWT_DECLARE_PRIVATE(LinearAutoScaler)

    LinearAutoScaler();
    explicit LinearAutoScaler(std::vector< double >& mantisses);
    ~LinearAutoScaler() override;
    AutoScaler* clone() const override;
    int execute(double& a, double& b, double start, double stop, int ivals) override;

    // Copies state from another LinearAutoScaler (used by LinearScale::clone)
    void copyStateFrom(const LinearAutoScaler& other);

private:
    void init(double start, double stop, int ivals);
    double anchorvalue(double start, double mantisse, int exponent);
    int segments(int& l_intervals, int& r_intervals, double start, double stop, double anchor, double mantissa, int exponent);
};


#endif