#include "initwidget.h"
#include "ui_initwidget.h"



InitWidget::InitWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InitWidget)
{
    ui->setupUi(this);

    this->move((parent->width()  - this->width())  / 2,
               (parent->height() - this->height()) / 2);
}

InitWidget::~InitWidget()
{
    delete ui;
}

