/******************************************************************************
 * Copyright 2017 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include <QCoreApplication>
#include <QDebug>

// [START iot_mqtt_include]
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jwt.h"
#include "openssl/ec.h"
#include "openssl/evp.h"
#include "MQTTClient.h"
// [END iot_mqtt_include]


/*
 * TODO: QtMqtt based publish method
 *
 * /

#define TRACE 1 /* Set to 1 to enable tracing */
int clientid_maxlen = 256;
struct {
    char* address;
    enum { clientid_maxlen = 256, clientid_size };
    char clientid[clientid_size];
    char* deviceid;
    char* ecpath;
    char* projectid;
    char* region;
    char* registryid;
    char* rootpath;
    char* topic;
    char* payload;
//} opts = {
//    "ssl://mqtt.googleapis.com:8883", // address
//    "projects/driverwatch-189321/locations/europe-west1/registries/general/devices/my-device", // clientid
//    "my-device", // deviceid
//    "/home/pi/gcloud/ec_private.pem", // ecpath
//    "driverwatch-189321", // projectid
//    "europe-west1", // region
//    "general", // registryid
//    "roots.pem", // rootpath
//    "/devices/my-device/events", // topic
//    "test!" // payload
//};

} opts = {
    "ssl://mqtt.googleapis.com:8883", // address
    "projects/driverwatch-189321/locations/europe-west1/registries/general/devices/my-device", // clientid
    "my-device", // deviceid
    "/home/pi/gcloud/ec_private.pem", // ecpath
    "driverwatch-189321", // projectid
    "europe-west1", // region
    "general", // registryid
    "roots.pem", // rootpath
    "/devices/my-device/events", // topic
    "test!" // payload
};


// [START iot_mqtt_jwt]
/**
 * Calculates issued at / expiration times for JWT and places the time, as a
 * Unix timestamp, in the strings passed to the function. The time_size
 * parameter specifies the length of the string allocated for both iat and exp.
 */
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

/**
 * Calculates a Java Web Token (JWT) given the path to a EC private key and
 * Google Cloud project ID. Returns the JWT as a string that the caller must
 * free.
 */
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
// [END iot_mqtt_jwt]


static const int kQos = 1;
static const unsigned long kTimeout = 10000L;
static const char* kUsername = "unused";

/**
 * Publish a given message, passed in as payload, to Cloud IoT Core using the
 * values in the opts struct, stored in the global opts structure. Returns
 * the result code from the MQTT client.
 */
// [START iot_mqtt_publish]
int Publish(char* payload, int payload_size) {
    int rc = -1;
    MQTTClient client = {0};
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token = {0};

    MQTTClient_create(&client, opts.address, opts.clientid,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = kUsername;
    conn_opts.password = CreateJwt(opts.ecpath, opts.projectid);
    MQTTClient_SSLOptions sslopts = MQTTClient_SSLOptions_initializer;

    sslopts.trustStore = opts.rootpath;
    sslopts.privateKey = opts.ecpath;
    conn_opts.ssl = &sslopts;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    pubmsg.payload = payload;
    pubmsg.payloadlen = payload_size;
    pubmsg.qos = kQos;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, opts.topic, &pubmsg, &token);
    printf("Waiting for up to %lu seconds for publication of %s\n"
           "on topic %s for client with ClientID: %s\n",
           (kTimeout/1000), opts.payload, opts.topic, opts.clientid);
    rc = MQTTClient_waitForCompletion(client, token, kTimeout);
    printf("Message with delivery token %d delivered\n", token);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}
// [END iot_mqtt_publish]


/**
 * Connects MQTT client and transmits payload.
 */

// [START iot_mqtt_run]
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();
    OpenSSL_add_all_ciphers();

    Publish(opts.payload, strlen(opts.payload));

    EVP_cleanup();

    //return(a.exec());
    return(0);
}

// [END iot_mqtt_run]
