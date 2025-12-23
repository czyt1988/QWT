#ifndef QWTFIGURESERIESDATAPICKER_H
#define QWTFIGURESERIESDATAPICKER_H
#include "qwt_figure_picker.h"

class QwtFigureSeriesDataPicker : public QwtFigurePicker
{
    Q_OBJECT
public:
   explicit QwtFigureSeriesDataPicker(QwtFigure* fig);
~QwtFigureSeriesDataPicker();
};

#endif // QWTFIGURESERIESDATAPICKER_H
