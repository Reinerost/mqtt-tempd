/*
 * config.c
 *
 * Initialisierung, Einlesen, Prüfung und Freigabe der
 * Anwendungskonfiguration.
 */


/* ------------------------------------------------------------------------- */
/* Includes                                                                  */
/* ------------------------------------------------------------------------- */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <uci.h>

#include "config.h"
#include "log.h"


/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

#define CONFIG_PACKAGE_NAME         "mqtt-tempd"

#define CONFIG_SECTION_MQTT         "mqtt"
#define CONFIG_SECTION_SENSOR       "sensor"

#define CONFIG_OPTION_HOST          "host"
#define CONFIG_OPTION_PORT          "port"
#define CONFIG_OPTION_CLIENT_ID     "client_id"
#define CONFIG_OPTION_TOPIC_BASE    "topic_base"
#define CONFIG_OPTION_STATUS_TOPIC  "status_topic"
#define CONFIG_OPTION_KEEPALIVE     "keepalive"

#define CONFIG_OPTION_TOPIC         "topic"
#define CONFIG_OPTION_NAME          "name"
#define CONFIG_OPTION_LABEL         "label"

#define CONFIG_SECTION_RRD          "rrd"
#define CONFIG_OPTION_PATH          "path"

/* ------------------------------------------------------------------------- */
/* Local function declarations                                               */
/* ------------------------------------------------------------------------- */

static int config_set_string(char **destination,
                             const char *value);

static int config_read_string(struct uci_context *context,
                              struct uci_section *section,
                              const char *option_name,
                              char **destination);

static int config_read_uint16(struct uci_context *context,
                              struct uci_section *section,
                              const char *option_name,
                              uint16_t *destination);

static int config_load_mqtt(struct uci_context *context,
                            struct uci_package *package,
                            struct app_config *config);

static int config_load_sensors(struct uci_context *context,
                               struct uci_package *package,
                               struct app_config *config);

static int config_add_sensor(struct app_config *config,
                             const char *topic,
                             const char *name,
                             const char *label);

static int config_check_sensor_duplicate(
                            const struct app_config *config,
                            const char *topic,
                            const char *name);

static int config_load_rrd(struct uci_context *context,
                           struct uci_package *package,
                           struct app_config *config);

static int config_validate(const struct app_config *config);


/* ------------------------------------------------------------------------- */
/* Local functions                                                           */
/* ------------------------------------------------------------------------- */

static int config_set_string(char **destination,
                             const char *value)
{
    char *copy;

    if ((destination == NULL) ||
        (value == NULL) ||
        (value[0] == '\0')) {
        return -1;
    }

    copy = strdup(value);

    if (copy == NULL) {
        return -1;
    }

    free(*destination);
    *destination = copy;

    return 0;
}


static int config_read_string(struct uci_context *context,
                              struct uci_section *section,
                              const char *option_name,
                              char **destination)
{
    const char *value;

    if ((context == NULL) ||
        (section == NULL) ||
        (option_name == NULL) ||
        (destination == NULL)) {
        return -1;
    }

    value = uci_lookup_option_string(context,
                                     section,
                                     option_name);

    if ((value == NULL) || (value[0] == '\0')) {
        log_error("Missing or empty option '%s' in section '%s'",
                  option_name,
                  section->type);

        return -1;
    }

    if (config_set_string(destination, value) != 0) {
        log_error("Cannot store option '%s' from section '%s'",
                  option_name,
                  section->type);

        return -1;
    }

    return 0;
}


static int config_read_uint16(struct uci_context *context,
                              struct uci_section *section,
                              const char *option_name,
                              uint16_t *destination)
{
    const char *value;
    char *end;
    unsigned long number;

    if ((context == NULL) ||
        (section == NULL) ||
        (option_name == NULL) ||
        (destination == NULL)) {
        return -1;
    }

    value = uci_lookup_option_string(context,
                                     section,
                                     option_name);

    if ((value == NULL) || (value[0] == '\0')) {
        log_error("Missing or empty option '%s' in section '%s'",
                  option_name,
                  section->type);

        return -1;
    }

    errno = 0;
    end = NULL;

    number = strtoul(value, &end, 10);

    if ((errno != 0) ||
        (end == value) ||
        (*end != '\0') ||
        (number == 0UL) ||
        (number > UINT16_MAX)) {
        log_error("Invalid value '%s' for option '%s' in section '%s'",
                  value,
                  option_name,
                  section->type);

        return -1;
    }

    *destination = (uint16_t)number;

    return 0;
}


static int config_load_mqtt(struct uci_context *context,
                            struct uci_package *package,
                            struct app_config *config)
{
    struct uci_element *element;
    struct uci_section *mqtt_section;
    unsigned int section_count;

    if ((context == NULL) ||
        (package == NULL) ||
        (config == NULL)) {
        return -1;
    }

    mqtt_section = NULL;
    section_count = 0U;

    uci_foreach_element(&package->sections, element) {
        struct uci_section *section;

        section = uci_to_section(element);

        if ((section->type == NULL) ||
            (strcmp(section->type, CONFIG_SECTION_MQTT) != 0)) {
            continue;
        }

        mqtt_section = section;
        section_count++;
    }

    if (section_count == 0U) {
        log_error("Missing UCI section of type '%s'",
                  CONFIG_SECTION_MQTT);

        return -1;
    }

    if (section_count > 1U) {
        log_error("More than one UCI section of type '%s'",
                  CONFIG_SECTION_MQTT);

        return -1;
    }

    if (config_read_string(context,
                           mqtt_section,
                           CONFIG_OPTION_HOST,
                           &config->mqtt_host) != 0) {
        return -1;
    }

    if (config_read_uint16(context,
                           mqtt_section,
                           CONFIG_OPTION_PORT,
                           &config->mqtt_port) != 0) {
        return -1;
    }

    if (config_read_string(context,
                           mqtt_section,
                           CONFIG_OPTION_CLIENT_ID,
                           &config->mqtt_client_id) != 0) {
        return -1;
    }

    if (config_read_string(context,
                           mqtt_section,
                           CONFIG_OPTION_TOPIC_BASE,
                           &config->mqtt_topic_base) != 0) {
        return -1;
    }

    if (config_read_string(context,
                           mqtt_section,
                           CONFIG_OPTION_STATUS_TOPIC,
                           &config->mqtt_status_topic) != 0) {
        return -1;
    }

    if (config_read_uint16(context,
                           mqtt_section,
                           CONFIG_OPTION_KEEPALIVE,
                           &config->mqtt_keepalive) != 0) {
        return -1;
    }

    return 0;
}

static int config_load_rrd(struct uci_context *context,
                           struct uci_package *package,
                           struct app_config *config)
{
    struct uci_element *element;
    struct uci_section *rrd_section;
    unsigned int section_count;

    if ((context == NULL) ||
        (package == NULL) ||
        (config == NULL)) {
        return -1;
    }

    rrd_section = NULL;
    section_count = 0U;

    uci_foreach_element(&package->sections, element) {
        struct uci_section *section;

        section = uci_to_section(element);

        if ((section->type == NULL) ||
            (strcmp(section->type, CONFIG_SECTION_RRD) != 0)) {
            continue;
        }

        rrd_section = section;
        section_count++;
    }

    if (section_count == 0U) {
        log_error("Missing UCI section of type '%s'",
                  CONFIG_SECTION_RRD);
        return -1;
    }

    if (section_count > 1U) {
        log_error("More than one UCI section of type '%s'",
                  CONFIG_SECTION_RRD);
        return -1;
    }

    if (config_read_string(context,
                           rrd_section,
                           CONFIG_OPTION_PATH,
                           &config->rrd_path) != 0) {
        return -1;
    }

    log_info("RRD path loaded: '%s'",
         (config->rrd_path != NULL) ? config->rrd_path : "(null)");
    
    return 0;
}

static int config_load_sensors(struct uci_context *context,
                               struct uci_package *package,
                               struct app_config *config)
{
    struct uci_element *element;

    if ((context == NULL) ||
        (package == NULL) ||
        (config == NULL)) {
        return -1;
    }

    uci_foreach_element(&package->sections, element) {
        struct uci_section *section;
        const char *topic;
        const char *name;
        const char *label;

        section = uci_to_section(element);

        if ((section->type == NULL) ||
            (strcmp(section->type, CONFIG_SECTION_SENSOR) != 0)) {
            continue;
        }

        if (config->sensor_count >= CONFIG_MAX_SENSORS) {
            log_error("Too many sensor sections, maximum is %u",
                      (unsigned int)CONFIG_MAX_SENSORS);

            return -1;
        }

        topic = uci_lookup_option_string(context,
                                         section,
                                         CONFIG_OPTION_TOPIC);

        name = uci_lookup_option_string(context,
                                        section,
                                        CONFIG_OPTION_NAME);

        label = uci_lookup_option_string(context,
                                         section,
                                         CONFIG_OPTION_LABEL);

        if ((topic == NULL) || (topic[0] == '\0')) {
            log_error("Missing or empty option '%s' in sensor section",
                      CONFIG_OPTION_TOPIC);

            return -1;
        }

        if ((name == NULL) || (name[0] == '\0')) {
            log_error("Missing or empty option '%s' in sensor section",
                      CONFIG_OPTION_NAME);

            return -1;
        }

        if ((label == NULL) || (label[0] == '\0')) {
            log_error("Missing or empty option '%s' in sensor section",
                      CONFIG_OPTION_LABEL);

            return -1;
        }

        if (config_check_sensor_duplicate(config,
                                          topic,
                                          name) != 0) {
            return -1;
        }

        if (config_add_sensor(config,
                              topic,
                              name,
                              label) != 0) {
            return -1;
        }
    }

    return 0;
}


static int config_add_sensor(struct app_config *config,
                             const char *topic,
                             const char *name,
                             const char *label)
{
    struct sensor_config *new_sensors;
    struct sensor_config *sensor;
    size_t new_count;

    if ((config == NULL) ||
        (topic == NULL) ||
        (name == NULL) ||
        (label == NULL)) {
        return -1;
    }

    if (config->sensor_count >= CONFIG_MAX_SENSORS) {
        return -1;
    }

    new_count = config->sensor_count + 1U;

    new_sensors = realloc(config->sensors,
                          new_count * sizeof(*new_sensors));

    if (new_sensors == NULL) {
        log_error("Cannot allocate sensor configuration");

        return -1;
    }

    config->sensors = new_sensors;

    sensor = &config->sensors[config->sensor_count];

    memset(sensor, 0, sizeof(*sensor));

    if (config_set_string(&sensor->topic, topic) != 0) {
        log_error("Cannot store sensor topic '%s'", topic);
        return -1;
    }

    if (config_set_string(&sensor->name, name) != 0) {
        log_error("Cannot store sensor name '%s'", name);

        free(sensor->topic);
        sensor->topic = NULL;

        return -1;
    }

    if (config_set_string(&sensor->label, label) != 0) {
        log_error("Cannot store sensor label '%s'", label);

        free(sensor->name);
        sensor->name = NULL;

        free(sensor->topic);
        sensor->topic = NULL;

        return -1;
    }

    config->sensor_count = new_count;

    return 0;
}


static int config_check_sensor_duplicate(
    const struct app_config *config,
    const char *topic,
    const char *name)
{
    size_t index;

    if ((config == NULL) ||
        (topic == NULL) ||
        (name == NULL)) {
        return -1;
    }

    for (index = 0U;
         index < config->sensor_count;
         index++) {
        const struct sensor_config *sensor;

        sensor = &config->sensors[index];

        if (strcmp(sensor->topic, topic) == 0) {
            log_error("Duplicate sensor topic '%s'", topic);
            return -1;
        }

        if (strcmp(sensor->name, name) == 0) {
            log_error("Duplicate sensor name '%s'", name);
            return -1;
        }
    }

    return 0;
}


static int config_validate(const struct app_config *config)
{
    if (config == NULL) {
        return -1;
    }

    if ((config->mqtt_host == NULL) ||
        (config->mqtt_client_id == NULL) ||
        (config->mqtt_topic_base == NULL) ||
        (config->mqtt_status_topic == NULL)) {
        log_error("Incomplete MQTT configuration");
        return -1;
    }

    if ((config->mqtt_port == 0U) ||
        (config->mqtt_keepalive == 0U)) {
        log_error("Invalid MQTT numeric configuration");
        return -1;
    }

    if ((config->rrd_path == NULL) ||
        (config->rrd_path[0] == '\0')) {
        log_error("Incomplete RRD configuration");
        return -1;
    }
    
    if (config->sensor_count == 0U) {
        log_error("No sensor sections configured");
        return -1;
    }
    
    if (config->rrd_path == NULL) {
    log_error("Incomplete RRD configuration");
    return -1;
    }

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int config_init(struct app_config *config)
{
    if (config == NULL) {
        log_error("Configuration initialization failed: invalid argument");
        return -1;
    }

    memset(config, 0, sizeof(*config));

    return 0;
}


int config_load(struct app_config *config)
{
    struct app_config new_config;
    struct uci_context *context;
    struct uci_package *package;
    int result;

    if (config == NULL) {
        log_error("Configuration loading failed: invalid argument");
        return -1;
    }

    if (config_init(&new_config) != 0) {
        return -1;
    }

    context = uci_alloc_context();

    if (context == NULL) {
        log_error("Cannot allocate UCI context");
        return -1;
    }

    package = NULL;
    result = -1;

    if (uci_load(context,
                 CONFIG_PACKAGE_NAME,
                 &package) != UCI_OK) {
        log_error("Cannot load UCI configuration '%s'",
                  CONFIG_PACKAGE_NAME);

        goto cleanup;
    }

    if (config_load_mqtt(context,
                         package,
                         &new_config) != 0) {
        goto cleanup;
    }

    if (config_load_rrd(context,
                    package,
                    &new_config) != 0) {
        goto cleanup;
    }

    
    if (config_load_sensors(context,
                            package,
                            &new_config) != 0) {
        goto cleanup;
    }

    if (config_validate(&new_config) != 0) {
        goto cleanup;
    }

    config_cleanup(config);
    *config = new_config;

    memset(&new_config, 0, sizeof(new_config));

    log_info("Configuration loaded: broker=%s:%u, sensors=%u",
             config->mqtt_host,
             (unsigned int)config->mqtt_port,
             (unsigned int)config->sensor_count);

    result = 0;

cleanup:

    if (package != NULL) {
        uci_unload(context, package);
    }

    uci_free_context(context);

    config_cleanup(&new_config);

    return result;
}


void config_cleanup(struct app_config *config)
{
    size_t index;

    if (config == NULL) {
        return;
    }

    free(config->mqtt_host);
    config->mqtt_host = NULL;

    free(config->mqtt_client_id);
    config->mqtt_client_id = NULL;

    free(config->mqtt_topic_base);
    config->mqtt_topic_base = NULL;

    free(config->mqtt_status_topic);
    config->mqtt_status_topic = NULL;
    
    free(config->rrd_path);
    config->rrd_path = NULL;
    
    for (index = 0U;
         index < config->sensor_count;
         index++) {
        free(config->sensors[index].topic);
        config->sensors[index].topic = NULL;

        free(config->sensors[index].name);
        config->sensors[index].name = NULL;

        free(config->sensors[index].label);
        config->sensors[index].label = NULL;
    }

    free(config->sensors);
    config->sensors = NULL;

    config->mqtt_port = 0U;
    config->mqtt_keepalive = 0U;
    config->sensor_count = 0U;
}
