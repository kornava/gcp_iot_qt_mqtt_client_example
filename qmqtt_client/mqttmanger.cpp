#include "mqttmanger.h"
#include <QThread>
#include <QSslSocket>


#define TRACE 1 /* Set to 1 to enable tracing */


static void GetIatExp(char* iat, char* exp, int time_size) {
    // TODO: Use time.google.com for iat
    time_t now_seconds = time(NULL);
    snprintf(iat, time_size, "%lu", now_seconds);
    snprintf(exp, time_size, "%lu", now_seconds + 3600);
    if (TRACE) {
        printf("IAT: %s\n", iat);
        printf("EXP: %s\n", exp);
    }
}

static char* CreateJwt(const char* ec_private_path, const char* project_id) {
    char iat_time[sizeof(time_t) * 3 + 2];
    char exp_time[sizeof(time_t) * 3 + 2];
    uint8_t* key = NULL; // Stores the Base64 encoded certificate
    size_t key_len = 0;
    jwt_t *jwt = NULL;
    int ret = 0;
    char *out = NULL;

    // Read private key from file
    FILE *fp = fopen(ec_private_path, "r");
    if (fp == (void*) NULL) {
        printf("Could not open file: %s\n", ec_private_path);
        return "";
    }
    fseek(fp, 0L, SEEK_END);
    key_len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    key = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * (key_len + 1))); // certificate length + \0

    fread(key, 1, key_len, fp);
    key[key_len] = '\0';
    fclose(fp);

    // Get JWT parts
    GetIatExp(iat_time, exp_time, sizeof(iat_time));

    jwt_new(&jwt);

    // Write JWT
    ret = jwt_add_grant(jwt, "iat", iat_time);
    if (ret) {
        printf("Error setting issue timestamp: %d", ret);
    }
    ret = jwt_add_grant(jwt, "exp", exp_time);
    if (ret) {
        printf("Error setting expiration: %d", ret);
    }
    ret = jwt_add_grant(jwt, "aud", project_id);
    if (ret) {
        printf("Error adding audience: %d", ret);
    }
    ret = jwt_set_alg(jwt, JWT_ALG_ES256, key, key_len);
    if (ret) {
        printf("Error during set alg: %d", ret);
    }
    out = jwt_encode_str(jwt);

    // Print JWT
    if (TRACE) {
        printf("JWT: [%s]", out);
    }

    jwt_free(jwt);
    free(key);
    return out;
}


mqttManger::mqttManger(QObject *parent) : QObject(parent)
{
    QSslSocket *socket = new QSslSocket(this);
//    connect(socket, SIGNAL(encrypted()), this, SLOT(ready()));
    socket->connectToHostEncrypted("mqtt.googleapis.com", 8883);



    m_client = new QMqttClient(this);
    m_client->setTransport(socket,QMqttClient::SecureSocket);
    m_client->setHostname(hostname);
    m_client->setPort(port);
    m_client->setClientId(clientid);
    m_client->setKeepAlive(20);
    m_client->setCleanSession(1);
    m_client->setUsername("unused");
    m_client->setPassword(CreateJwt(ecpath, projectid));

    //TODO SSL

    qDebug()<<m_client->state();
    m_client->connectToHostEncrypted()
    qDebug()<<m_client->state();
    connect(m_client, &QMqttClient::stateChanged, this, &mqttManger::stateChanged);
}

void mqttManger::stateChanged()
{
    qDebug()<<m_client->state();
    //Check if we are connected (state 2)
    if(m_client->state() == 2)
    {
            qDebug()<<"Request ping.. "<<m_client->requestPing();

            auto subscription = m_client->subscribe(topic);

            if (!subscription) {
                qDebug()<<"Failed to subscribe to the topic"<<topic;
            }
            qDebug()<<m_client->publish(topic,payload.toUtf8());
    }

}
