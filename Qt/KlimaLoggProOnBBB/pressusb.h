#ifndef PRESSUSB_H
#define PRESSUSB_H

#include <QWidget>

namespace Ui {
class PressUsb;
}

class PressUsb : public QWidget
{
    Q_OBJECT

public:
    explicit PressUsb(QWidget *parent = 0);
    ~PressUsb();

private:
    Ui::PressUsb *ui;
};

#endif // PRESSUSB_H
