#include "mainwindow.h"


#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QRect>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    //qDebug() << "main() - ThreadId: " << QThread::currentThreadId();

    QRect r = w.geometry();
    r.moveCenter(QApplication::desktop()->availableGeometry().center());
    w.setGeometry(r);

    w.show();

    return a.exec();
}


