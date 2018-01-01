#ifndef MQTTMANGER_H
#define MQTTMANGER_H

#include <QObject>
#include <QtMqtt/QtMqtt>
#include "jwt.h"


class mqttManger : public QObject
{
    Q_OBJECT
public:
    explicit mqttManger(QObject *parent = nullptr);

private:
    QMqttClient *m_client;


//    QString hostname = "broker.hivemq.com";
//    qint16 port = 1883;
    QString hostname = "mqtt.googleapis.com"; // address
    qint16 port = 8883; //port //443
    QString clientid = "projects/driverwatch-189321/locations/europe-west1/registries/general/devices/my_device_2"; // clientid
    QString deviceid = "my_device_2"; // deviceid
    char*  ecpath = "/home/saif/MEGAsync/MY_DEV/MY_QT/qmqtt_client/keys/ec_private.pem"; // ecpath
    char*  projectid = "driverwatch-189321"; // projectid
    QString region = "europe-west1"; // region
    QString registryid = "general"; // registryid
    QString rootpath = "/home/saif/MEGAsync/MY_DEV/MY_QT/qmqtt_client/keys/roots.pem"; // rootpath
    QString topic = "/devices/my_device_2/events"; // topic
    QString payload = "msg";

    //static char* CreateJwt(const char* ec_private_path, const char* project_id);
signals:

public slots:
    void stateChanged();
};

#endif // MQTTMANGER_H
