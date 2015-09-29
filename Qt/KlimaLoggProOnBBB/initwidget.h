#ifndef INITWIDGET_H
#define INITWIDGET_H

#include <QWidget>

namespace Ui {
class InitWidget;
}

class InitWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InitWidget(QWidget *parent = 0);
    ~InitWidget();

private:
    Ui::InitWidget *ui;

};

#endif // INITWIDGET_H
