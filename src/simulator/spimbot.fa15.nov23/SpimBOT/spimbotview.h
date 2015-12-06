#ifndef SPIMBOTVIEW_H
#define SPIMBOTVIEW_H

#include <QMainWindow>
#include <QWidget>

namespace Ui {
class SpimbotView;
}

class SpimbotView : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit SpimbotView(QWidget *parent = 0);
    ~SpimbotView();

    virtual void closeEvent(QCloseEvent *);
    
private:
    Ui::SpimbotView *ui;
};

#endif // SPIMBOTVIEW_H
