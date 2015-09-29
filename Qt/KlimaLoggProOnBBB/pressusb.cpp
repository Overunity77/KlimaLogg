#include "pressusb.h"
#include "ui_pressusb.h"

PressUsb::PressUsb(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PressUsb)
{
    ui->setupUi(this);

    this->move((parent->width()  - this->width())  / 2,
               (parent->height() - this->height()) / 2);
}

PressUsb::~PressUsb()
{
    delete ui;
}
