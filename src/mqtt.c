/*
 * mqtt.c
 *
 * Aufgabe:
 * Aufbau und Verwaltung der MQTT-Verbindung, Empfang abonnierter
 * Temperaturwerte sowie Veröffentlichung des eigenen Betriebszustands.
 */


/* ==========================================================================
 * Includes
 * ========================================================================== */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto.h>

#include "log.h"
#include "mqtt.h"
#include "rrd.h"

/* ==========================================================================
 * Constants
 * ========================================================================== */

#define MQTT_SUBSCRIPTION_QOS          0
#define MQTT_STATUS_QOS                1
#define MQTT_STATUS_RETAIN             true

#define MQTT_STATUS_ONLINE             "online"
#define MQTT_STATUS_OFFLINE            "offline"

#define MQTT_RECONNECT_DELAY_MIN       2U
#define MQTT_RECONNECT_DELAY_MAX       30U

#define MQTT_LOOP_TIMEOUT_MS           100
#define MQTT_LOOP_MAX_PACKETS          1


/* ==========================================================================
 * Local variables
 * ========================================================================== */

static struct mosquitto *mqtt_client = NULL;

static const struct app_config *mqtt_config = NULL;

static char *mqtt_subscription_topic = NULL;
static char *mqtt_status_topic = NULL;

static bool mqtt_library_initialized = false;
static bool mqtt_connected = false;
static bool mqtt_subscribed = false;

/* ==========================================================================
 * Local function declarations
 * ========================================================================== */

static char *mqtt_create_subscription_topic(const char *topic_base);

static const struct sensor_config *mqtt_find_sensor(const char *topic);

static int mqtt_publish_status(struct mosquitto *mosq,
                               const char *status);

static void mqtt_on_connect(struct mosquitto *mosq,
                            void *userdata,
                            int result);

static void mqtt_on_disconnect(struct mosquitto *mosq,
                               void *userdata,
                               int result);

static void mqtt_on_message(struct mosquitto *mosq,
                            void *userdata,
                            const struct mosquitto_message *message);

static int mqtt_parse_temperature(const struct mosquitto_message *message,
                                  double *temperature);

/* ==========================================================================
 * Local functions
 * ========================================================================== */

static char *mqtt_create_subscription_topic(const char *topic_base)
{
    char *topic;
    size_t base_length;
    size_t topic_length;
    bool has_trailing_slash;

    if ((topic_base == NULL) || (topic_base[0] == '\0'))
    {
        return NULL;
    }

    base_length = strlen(topic_base);
    has_trailing_slash = (topic_base[base_length - 1U] == '/');

    /*
     * Ohne abschließenden Slash werden "/#" und das Nullzeichen benötigt.
     * Mit abschließendem Slash werden nur "#" und das Nullzeichen benötigt.
     */
    topic_length = base_length + (has_trailing_slash ? 2U : 3U);

    topic = malloc(topic_length);

    if (topic == NULL)
    {
        return NULL;
    }

    if (has_trailing_slash)
    {
        (void)snprintf(topic,
                       topic_length,
                       "%s#",
                       topic_base);
    }
    else
    {
        (void)snprintf(topic,
                       topic_length,
                       "%s/#",
                       topic_base);
    }

    return topic;
}

static const struct sensor_config *
mqtt_find_sensor(const char *topic)
{
    size_t index;

    if ((mqtt_config == NULL) ||
        (topic == NULL) ||
        (topic[0] == '\0')) {
        return NULL;
    }

    for (index = 0U;
         index < mqtt_config->sensor_count;
         index++) {
        const struct sensor_config *sensor;

        sensor = &mqtt_config->sensors[index];

        if ((sensor->topic != NULL) &&
            (strcmp(sensor->topic, topic) == 0)) {
            return sensor;
        }
    }

    return NULL;
}

static int mqtt_publish_status(struct mosquitto *mosq,
                               const char *status)
{
    int rc;

    if ((mosq == NULL) ||
        (mqtt_status_topic == NULL) ||
        (status == NULL))
    {
        return -1;
    }

    rc = mosquitto_publish(mosq,
                           NULL,
                           mqtt_status_topic,
                           (int)strlen(status),
                           status,
                           MQTT_STATUS_QOS,
                           MQTT_STATUS_RETAIN);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot publish MQTT status '%s' on topic '%s': %s",
                  status,
                  mqtt_status_topic,
                  mosquitto_strerror(rc));

        return -1;
    }

    log_info("MQTT status published: topic='%s', status='%s'",
             mqtt_status_topic,
             status);

    return 0;
}


static void mqtt_on_connect(struct mosquitto *mosq,
                            void *userdata,
                            int result)
{
    int rc;

    (void)userdata;

    mqtt_subscribed = false;

    if (result != 0)
    {
        mqtt_connected = false;

        log_error("MQTT connection failed: %s",
                  mosquitto_connack_string(result));

        return;
    }

    mqtt_connected = true;

    log_notice("MQTT connection established");

    rc = mosquitto_subscribe(mosq,
                             NULL,
                             mqtt_subscription_topic,
                             MQTT_SUBSCRIPTION_QOS);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot subscribe to MQTT topic '%s': %s",
                  mqtt_subscription_topic,
                  mosquitto_strerror(rc));
    }
    else
    {
        mqtt_subscribed = true;
        log_info("MQTT subscription requested: %s",
                 mqtt_subscription_topic);
    }

    if (mqtt_publish_status(mosq, MQTT_STATUS_ONLINE) != 0)
    {
        log_warning("Cannot publish MQTT online status");
    }
}


static void mqtt_on_disconnect(struct mosquitto *mosq,
                               void *userdata,
                               int result)
{
    (void)mosq;
    (void)userdata;

    mqtt_connected = false;
    mqtt_subscribed = false;

    if (result == 0)
    {
        log_notice("MQTT connection closed");
    }
    else
    {
        log_warning("MQTT connection lost: %s",
                    mosquitto_strerror(result));
    }
}


static void mqtt_on_message(struct mosquitto *mosq,
                            void *userdata,
                            const struct mosquitto_message *message)
{
    const struct sensor_config *sensor;
    (void)mosq;
    (void)userdata;

    if ((message == NULL) || (message->topic == NULL))
    {
        log_warning("Invalid MQTT message received");
        return;
    }

    if ((message->payload == NULL) || (message->payloadlen <= 0))
    {
        log_warning("Empty MQTT payload received on topic '%s'",
                    message->topic);
        return;
    }

    sensor = mqtt_find_sensor(message->topic);

    if (sensor == NULL) {
        log_warning("MQTT message received for unknown sensor topic '%s'",
                    message->topic);
        return;
    }

    log_debug("MQTT sensor message received: "
             "name='%s', label='%s', topic='%s', payload='%.*s'",
             sensor->name,
             sensor->label,
             message->topic,
             message->payloadlen,
             (const char *)message->payload);


   sensor = mqtt_find_sensor(message->topic);

    if (sensor == NULL) {
        log_warning("MQTT message received for unknown sensor topic '%s'",
                    message->topic);
        return;
    }
    
    double temperature;

    if (mqtt_parse_temperature(message, &temperature) != 0) {
        log_warning("Invalid temperature payload on topic '%s'",
                    message->topic);
        return;
    }

    log_debug("Temperature received: sensor='%s', value=%.3f",
            sensor->name,
            temperature);
    
    if (rrd_store_value(sensor, temperature) != 0) {
            log_error("Cannot store temperature for sensor '%s'",
                    sensor->name);
    }
    
}


/* ==========================================================================
 * Public functions
 * ========================================================================== */

int mqtt_init(const struct app_config *config)
{
    int rc;

    if (config == NULL)
    {
        log_error("MQTT initialization failed: invalid configuration");
        return -1;
    }

    if ((config->mqtt_host == NULL) ||
        (config->mqtt_host[0] == '\0') ||
        (config->mqtt_client_id == NULL) ||
        (config->mqtt_client_id[0] == '\0') ||
        (config->mqtt_topic_base == NULL) ||
        (config->mqtt_topic_base[0] == '\0') ||
        (config->mqtt_status_topic == NULL) ||
        (config->mqtt_status_topic[0] == '\0'))
    {
        log_error("MQTT initialization failed: incomplete configuration");
        return -1;
    }

    if (mqtt_client != NULL)
    {
        log_warning("MQTT client is already initialized");
        return 0;
    }

    mqtt_config = config;

    rc = mosquitto_lib_init();

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot initialize Mosquitto library: %s",
                  mosquitto_strerror(rc));
        return -1;
    }

    mqtt_library_initialized = true;

    mqtt_subscription_topic =
        mqtt_create_subscription_topic(config->mqtt_topic_base);

    if (mqtt_subscription_topic == NULL)
    {
        log_error("Cannot create MQTT subscription topic");
        mqtt_cleanup();
        return -1;
    }

    mqtt_status_topic = strdup(config->mqtt_status_topic);

    if (mqtt_status_topic == NULL)
    {
        log_error("Cannot allocate MQTT status topic");
        mqtt_cleanup();
        return -1;
    }

    mqtt_client = mosquitto_new(config->mqtt_client_id,
                                true,
                                NULL);

    if (mqtt_client == NULL)
    {
        log_error("Cannot create MQTT client");
        mqtt_cleanup();
        return -1;
    }

    mosquitto_connect_callback_set(mqtt_client,
                                   mqtt_on_connect);

    mosquitto_disconnect_callback_set(mqtt_client,
                                      mqtt_on_disconnect);

    mosquitto_message_callback_set(mqtt_client,
                                   mqtt_on_message);

    rc = mosquitto_reconnect_delay_set(mqtt_client,
                                       MQTT_RECONNECT_DELAY_MIN,
                                       MQTT_RECONNECT_DELAY_MAX,
                                       true);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot configure MQTT reconnect delay: %s",
                  mosquitto_strerror(rc));

        mqtt_cleanup();
        return -1;
    }

    rc = mosquitto_will_set(mqtt_client,
                            mqtt_status_topic,
                            (int)strlen(MQTT_STATUS_OFFLINE),
                            MQTT_STATUS_OFFLINE,
                            MQTT_STATUS_QOS,
                            MQTT_STATUS_RETAIN);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot configure MQTT Last Will: %s",
                  mosquitto_strerror(rc));

        mqtt_cleanup();
        return -1;
    }

    rc = mosquitto_connect(mqtt_client,
                           config->mqtt_host,
                           (int)config->mqtt_port,
                           (int)config->mqtt_keepalive);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_error("Cannot connect to MQTT broker %s:%u: %s",
                  config->mqtt_host,
                  (unsigned int)config->mqtt_port,
                  mosquitto_strerror(rc));

        mqtt_cleanup();
        return -1;
    }

    log_info("MQTT client initialized: broker=%s:%u, subscription=%s",
             config->mqtt_host,
             (unsigned int)config->mqtt_port,
             mqtt_subscription_topic);

    return 0;
}


int mqtt_process(void)
{
    int rc;

    if (mqtt_client == NULL)
    {
        log_error("MQTT client is not initialized");
        return -1;
    }

    rc = mosquitto_loop(mqtt_client,
                        MQTT_LOOP_TIMEOUT_MS,
                        MQTT_LOOP_MAX_PACKETS);

    if (rc == MOSQ_ERR_SUCCESS)
    {
        return 0;
    }

    mqtt_connected = false;
    mqtt_subscribed =false;

    if ((rc != MOSQ_ERR_NO_CONN) &&
        (rc != MOSQ_ERR_CONN_LOST))
    {
        log_error("MQTT processing failed: %s",
                  mosquitto_strerror(rc));
    }

    rc = mosquitto_reconnect(mqtt_client);

    if (rc != MOSQ_ERR_SUCCESS)
    {
        log_warning("MQTT reconnection failed: %s",
                    mosquitto_strerror(rc));
        return -1;
    }

    log_notice("MQTT reconnection initiated");

    return 0;
}


void mqtt_cleanup(void)
{
    if (mqtt_client != NULL)
    {
        if (mqtt_connected)
        {
            /*
             * Bei einem kontrollierten Disconnect sendet der Broker
             * den Last Will nicht. Deshalb wird "offline" zuvor selbst
             * veröffentlicht.
             */
            if (mqtt_publish_status(mqtt_client,
                                    MQTT_STATUS_OFFLINE) == 0)
            {
                /*
                 * Die Publish-Nachricht wird noch an den Broker
                 * übertragen, bevor die Verbindung getrennt wird.
                 */
                (void)mosquitto_loop(mqtt_client,
                                     MQTT_LOOP_TIMEOUT_MS,
                                     MQTT_LOOP_MAX_PACKETS);
            }

            (void)mosquitto_disconnect(mqtt_client);

            /*
             * Den kontrollierten Disconnect noch verarbeiten.
             */
            (void)mosquitto_loop(mqtt_client,
                                 MQTT_LOOP_TIMEOUT_MS,
                                 MQTT_LOOP_MAX_PACKETS);
        }

        mosquitto_destroy(mqtt_client);
        mqtt_client = NULL;
    }

    free(mqtt_subscription_topic);
    mqtt_subscription_topic = NULL;

    free(mqtt_status_topic);
    mqtt_status_topic = NULL;

    mqtt_connected = false;
    mqtt_subscribed = false;
    mqtt_config = NULL;

    if (mqtt_library_initialized)
    {
        mosquitto_lib_cleanup();
        mqtt_library_initialized = false;
    }
}

static int mqtt_parse_temperature(const struct mosquitto_message *message,
                                  double *temperature)
{
    char buffer[32];
    char *end;
    double value;

    if ((message == NULL) ||
        (temperature == NULL) ||
        (message->payload == NULL) ||
        (message->payloadlen <= 0) ||
        (message->payloadlen >= (int)sizeof(buffer))) {
        return -1;
    }

    memcpy(buffer,
           message->payload,
           (size_t)message->payloadlen);

    buffer[message->payloadlen] = '\0';

    end = NULL;
    value = strtod(buffer, &end);

    if ((end == buffer) || (*end != '\0')) {
        return -1;
    }

    *temperature = value;

    return 0;
}
