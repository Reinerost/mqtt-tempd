/*
 * rrd.c
 *
 * Aufgabe:
 * Verwaltung der RRD-Datenbanken und des konfigurierten
 * RRD-Speicherpfades.
 */


/* ------------------------------------------------------------------------- */
/* Includes                                                                  */
/* ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <rrd.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "log.h"
#include "rrd.h"

/* ------------------------------------------------------------------------- */
/* Locale Konstanten                                                           */
/* ------------------------------------------------------------------------- */

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x)       STRINGIFY_INNER(x)

#define RRD_STEP_SECONDS       60
#define RRD_HEARTBEAT_SECONDS  180

#define RRD_FILENAME_EXTENSION      ".rrd"

/* ------------------------------------------------------------------------- */
/* Local variables                                                           */
/* ------------------------------------------------------------------------- */

static char *rrd_base_path = NULL;


/* ------------------------------------------------------------------------- */
/* Local functions                                                           */
/* ------------------------------------------------------------------------- */
static int rrd_check_directory(const char *path);

static char *rrd_create_filename(const struct sensor_config *sensor);

static int rrd_create_sensor(const struct sensor_config *sensor);

static int rrd_check_sensor(const struct sensor_config *sensor);

static int rrd_check_directory(const char *path)
{
    struct stat status;

    if ((path == NULL) || (path[0] == '\0')) {
        return -1;
    }

    if (stat(path, &status) != 0) {
        log_error("RRD directory '%s' does not exist", path);
        return -1;
    }

    if (!S_ISDIR(status.st_mode)) {
        log_error("RRD path '%s' is not a directory", path);
        return -1;
    }

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int rrd_module_init(const struct app_config *config)
{
    log_info("RRD init path: '%s'",
         (config->rrd_path != NULL) ? config->rrd_path : "(null)");
    
    size_t index;

    if (config == NULL) {
        log_error("RRD initialization failed: invalid configuration");
        return -1;
    }

    if ((config->rrd_path == NULL) ||
        (config->rrd_path[0] == '\0')) {
        log_error("RRD initialization failed: missing path");
        return -1;
    }

    if (rrd_base_path != NULL) {
        log_warning("RRD module is already initialized");
        return 0;
    }

    if (rrd_check_directory(config->rrd_path) != 0) {
        return -1;
    }

    rrd_base_path = strdup(config->rrd_path);

    if (rrd_base_path == NULL) {
        log_error("Cannot allocate RRD base path");
        return -1;
    }

    log_info("RRD directory: %s", rrd_base_path);

    for (index = 0U;
         index < config->sensor_count;
         index++) {
        if (rrd_check_sensor(&config->sensors[index]) != 0) {
            rrd_module_cleanup();
            return -1;
        }
    }

    log_info("RRD module initialized: %u databases",
             (unsigned int)config->sensor_count);

    return 0;
}

static char *rrd_create_filename(const struct sensor_config *sensor)
{
    char *filename;
    size_t length;
    bool has_trailing_slash;

    if ((rrd_base_path == NULL) ||
        (sensor == NULL) ||
        (sensor->name == NULL) ||
        (sensor->name[0] == '\0')) {
        return NULL;
    }

    has_trailing_slash =
        (rrd_base_path[strlen(rrd_base_path) - 1U] == '/');

    length = strlen(rrd_base_path) +
             strlen(sensor->name) +
             strlen(RRD_FILENAME_EXTENSION) +
             (has_trailing_slash ? 1U : 2U);

    filename = malloc(length);

    if (filename == NULL) {
        return NULL;
    }

    (void)snprintf(filename,
                   length,
                   has_trailing_slash ? "%s%s%s" : "%s/%s%s",
                   rrd_base_path,
                   sensor->name,
                   RRD_FILENAME_EXTENSION);

    return filename;
}

static int rrd_create_sensor(const struct sensor_config *sensor)
{
    char *filename;
    int argc;
    int rc;
    time_t last_update;

    const char *arguments[] = {
        "DS:temp:GAUGE:"
        STRINGIFY(RRD_HEARTBEAT_SECONDS)
        ":U:U",

        "RRA:AVERAGE:0.5:1:2880",
        "RRA:MIN:0.5:1:2880",
        "RRA:MAX:0.5:1:2880",

        "RRA:AVERAGE:0.5:5:4032",
        "RRA:MIN:0.5:5:4032",
        "RRA:MAX:0.5:5:4032",

        "RRA:AVERAGE:0.5:30:4320",
        "RRA:MIN:0.5:30:4320",
        "RRA:MAX:0.5:30:4320",

        "RRA:AVERAGE:0.5:120:8760",
        "RRA:MIN:0.5:120:8760",
        "RRA:MAX:0.5:120:8760",

        "RRA:AVERAGE:0.5:1440:3650",
        "RRA:MIN:0.5:1440:3650",
        "RRA:MAX:0.5:1440:3650"
    };

    if ((sensor == NULL) ||
        (sensor->name == NULL) ||
        (sensor->name[0] == '\0')) {
        log_error("Cannot create RRD: invalid sensor");
        return -1;
    }

    filename = rrd_create_filename(sensor);

    if (filename == NULL) {
        log_error("Cannot create RRD filename for sensor '%s'",
                  sensor->name);
        return -1;
    }

    argc = (int)(sizeof(arguments) / sizeof(arguments[0]));

    last_update = time(NULL) - RRD_STEP_SECONDS;

    rrd_clear_error();

    rc = rrd_create_r(filename,
                      RRD_STEP_SECONDS,
                      last_update,
                      argc,
                      arguments);

    if (rc != 0) {
        log_error("Cannot create RRD '%s': %s",
                  filename,
                  rrd_get_error());

        rrd_clear_error();
        free(filename);

        return -1;
    }

    log_info("RRD created: %s", filename);

    free(filename);

    return 0;
}

static int rrd_check_sensor(const struct sensor_config *sensor)
{
    char *filename;
    struct stat status;

    if (sensor == NULL) {
        return -1;
    }

    filename = rrd_create_filename(sensor);

    if (filename == NULL) {
        log_error("Cannot create RRD filename for sensor '%s'",
                  sensor->name);
        return -1;
    }

    if (stat(filename, &status) == 0) {
        if (!S_ISREG(status.st_mode)) {
            log_error("RRD path '%s' is not a regular file",
                      filename);

            free(filename);
            return -1;
        }

        free(filename);
        return 0;
    }

    if (errno != ENOENT) {
        log_error("Cannot access RRD '%s': %s",
                  filename,
                  strerror(errno));

        free(filename);
        return -1;
    }

    free(filename);

    return rrd_create_sensor(sensor);
}

int rrd_store_value(const struct sensor_config *sensor,
                    double value)
{
    char *filename;
    char update_value[64];
    const char *arguments[1];
    int rc;

    if ((sensor == NULL) ||
        (sensor->name == NULL) ||
        (sensor->name[0] == '\0')) {
        log_error("RRD update failed: invalid sensor");
        return -1;
    }

    filename = rrd_create_filename(sensor);

    if (filename == NULL) {
        log_error("Cannot create RRD filename for sensor '%s'",
                  sensor->name);
        return -1;
    }

    if (snprintf(update_value,
                 sizeof(update_value),
                 "N:%.3f",
                 value) >= (int)sizeof(update_value)) {
        log_error("Cannot format RRD update value for sensor '%s'",
                  sensor->name);

        free(filename);
        return -1;
    }

    arguments[0] = update_value;

    rrd_clear_error();

    rc = rrd_update_r(filename,
                      NULL,
                      1,
                      arguments);

    if (rc != 0) {
        log_error("Cannot update RRD '%s': %s",
                  filename,
                  rrd_get_error());

        rrd_clear_error();
        free(filename);

        return -1;
    }

    log_debug("RRD updated: sensor='%s', value=%.3f",
              sensor->name,
              value);

    free(filename);

    return 0;
}

void rrd_module_cleanup(void)
{
    free(rrd_base_path);
    rrd_base_path = NULL;
}
