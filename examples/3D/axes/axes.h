#ifndef axes_h
#define axes_h

#include "qwt3d_plot.h"
#include "qwt3d_scale.h"

/*****************************
 *
 *  Examples for user defined
 *  tic labels
 *
 ******************************/

class Letter : public Qwt3DLinearScale
{
public:
    explicit Letter(bool uppercase = true) : uc_(uppercase) { }
    Qwt3DScale *clone() const override { return new Letter(uc_); }
    QString ticLabel(unsigned int idx) const override
    {
        if (idx < majorTicks().size() && idx < 26)
            return (uc_) ? QString(QChar('A' + idx)) : QString(QChar('a' + idx));
        return QString("-");
    }

private:
    bool uc_;
};

class Imaginary : public Qwt3DLinearScale
{
public:
    Qwt3DScale *clone() const override { return new Imaginary; }
    QString ticLabel(unsigned int idx) const override
    {
        const auto& majors = majorTicks();
        if (idx < majors.size()) {
            double val = majors[idx];
            if (val)
                return QString::number(val) + "*i";
            return QString::number(val);
        }
        return QString("");
    }
};

class TimeItems : public Qwt3DLinearScale
{
public:
    Qwt3DScale *clone() const override { return new TimeItems; }
    QString ticLabel(unsigned int idx) const override
    {
        if (idx < majorTicks().size()) {
            QTime t = QTime::currentTime();
            int h = t.hour();
            int m = t.minute();
            if (m + idx > 59) {
                if (h == 23)
                    h = 0;
                else
                    h += 1;
                m = (m + idx) % 60;
            } else
                m += idx;

            return QTime(h, m).toString("hh:mm") + "h";
        }
        return QString("");
    }
};

#endif /* include guarded */
