/*
 * config.h
 *
 * Öffentliche Schnittstelle und Datenstrukturen für die
 * Anwendungskonfiguration.
 */

#ifndef CONFIG_H
#define CONFIG_H


/* ------------------------------------------------------------------------- */
/* Includes                                                                  */
/* ------------------------------------------------------------------------- */

#include <stddef.h>
#include <stdint.h>


/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

#define CONFIG_MAX_SENSORS 32U


/* ------------------------------------------------------------------------- */
/* Public types                                                              */
/* ------------------------------------------------------------------------- */

struct sensor_config
{
    char *topic;
    char *name;
    char *label;
};

struct app_config
{
    
    char *mqtt_host;
    uint16_t mqtt_port;

    char *mqtt_client_id;
    char *mqtt_topic_base;
    char *mqtt_status_topic;

    uint16_t mqtt_keepalive;
    
    char *rrd_path;

    struct sensor_config *sensors;
    size_t sensor_count;
};


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int config_init(struct app_config *config);
int config_load(struct app_config *config);
void config_cleanup(struct app_config *config);


#endif /* CONFIG_H */
