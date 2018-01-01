#include <QCoreApplication>
#include <QDebug>
#include "mqttmanger.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug()<<"besmELLAH";
    mqttManger mm;
    return a.exec();
}
