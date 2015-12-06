#ifndef SPIMBOTWIDGET_H
#define SPIMBOTWIDGET_H

#include <QWidget>
#include <QPainter>

namespace Ui {
class SpimbotWidget;
}

class SpimbotWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit SpimbotWidget(QWidget *parent = 0);
    ~SpimbotWidget();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);

private:
    Ui::SpimbotWidget *ui;
};

#endif // SPIMBOTWIDGET_H
