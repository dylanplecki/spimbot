#include <QPalette>
#include "../CPU/spim.h"
#include "../CPU/string-stream.h"
#include "../CPU/inst.h"
#include "../CPU/reg.h"
#include "../CPU/mem.h"
#include "spimbotwidget.h"
#include "ui_spimbotwidget.h"
#include "robot.h"

SpimbotWidget::SpimbotWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpimbotWidget)
{
    ui->setupUi(this);

    QPalette pallete(Qt::white);
    setPalette(pallete);
}

SpimbotWidget::~SpimbotWidget()
{
    delete ui;
}

void SpimbotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    redraw_map_window(&painter);
}

void SpimbotWidget::mousePressEvent(QMouseEvent *)
{
    spimbot_map_click();
}
