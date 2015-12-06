#include "spimbotwidget.h"
#include "spimbotview.h"
#include "ui_spimbotview.h"

SpimbotView::SpimbotView(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SpimbotView)
{
    ui->setupUi(this);

    SpimbotWidget* widget = new SpimbotWidget();
    this->setCentralWidget(widget);
}

SpimbotView::~SpimbotView()
{
    delete ui;
}

void SpimbotView::closeEvent(QCloseEvent *)
{
    qApp->exit(0);
    exit(0);
}
