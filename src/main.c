/*
 * main.c
 *
 * Einstiegspunkt und zentrale Ablaufsteuerung des MQTT-Temperaturdienstes.
 */


/* ------------------------------------------------------------------------- */
/* Includes                                                                  */
/* ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "main.h"
#include "signals.h"
#include "mqtt.h"
#include "rrd.h"

int app_reload(struct app_config *config);

/* ------------------------------------------------------------------------- */
/* Main                                                                      */
/* ------------------------------------------------------------------------- */

int main(void)
{
    struct app_config config;

    log_init(PROGRAM_NAME);

    log_info("%s %s starting",
             PROGRAM_NAME,
             PROGRAM_VERSION);

    if (signals_init() != 0) {
        log_cleanup();
        return EXIT_FAILURE;
    }

    if (config_init(&config) != 0) {
        log_cleanup();
        return EXIT_FAILURE;
    }

    if (config_load(&config) != 0) {
        config_cleanup(&config);
        log_cleanup();
        return EXIT_FAILURE;
    }

    if (rrd_module_init(&config) != 0) {
        config_cleanup(&config);
        log_cleanup();
        return EXIT_FAILURE;
    }

    if (mqtt_init(&config) != 0) {
        rrd_module_cleanup();
        config_cleanup(&config);
        log_cleanup();
        return EXIT_FAILURE;
    }
    

    while (!signals_terminate_requested()) {
        (void)mqtt_process();
    }

    log_info("%s stopping", PROGRAM_NAME);

    mqtt_cleanup();
    rrd_module_cleanup();
    config_cleanup(&config);
    log_cleanup();

    return EXIT_SUCCESS;
}

int app_reload(struct app_config *config)
{
    struct app_config new_config;
    struct app_config old_config;

    if (config == NULL) {
        return -1;
    }

    /*
     * Neue Konfiguration zunächst unabhängig von der laufenden
     * Konfiguration einlesen und vollständig prüfen.
     */
    if (config_init(&new_config) != 0) {
        return -1;
    }

    if (config_load(&new_config) != 0) {
        config_cleanup(&new_config);

        log_error("Configuration reload failed");
        return -1;
    }

    /*
     * Die neue Konfiguration ist gültig.
     * Jetzt können die von der Konfiguration abhängigen Module
     * kontrolliert beendet werden.
     */
    mqtt_cleanup();
    rrd_module_cleanup();

    /*
     * Besitz der bisherigen Konfiguration sichern.
     *
     * Es wird nur die Struktur kopiert. Die darin enthaltenen
     * Speicherbereiche bleiben erhalten und gehören jetzt old_config.
     */
    old_config = *config;

    /*
     * Neue Konfiguration übernehmen.
     */
    *config = new_config;
    new_config = (struct app_config){0};

    /*
     * Module mit der neuen Konfiguration starten.
     */
    if (rrd_module_init(config) != 0) {
        log_error("RRD initialization with new configuration failed");
        goto rollback;
    }

    if (mqtt_init(config) != 0) {
        log_error("MQTT initialization with new configuration failed");

        rrd_module_cleanup();
        goto rollback;
    }

    /*
     * Neue Konfiguration läuft erfolgreich.
     * Die alte Konfiguration wird nicht mehr benötigt.
     */
    config_cleanup(&old_config);

    log_notice("Application configuration reloaded");

    return 0;


rollback:

    /*
     * Eventuell teilweise initialisierte neue Module beenden.
     */
    mqtt_cleanup();
    rrd_module_cleanup();

    /*
     * Fehlgeschlagene neue Konfiguration freigeben.
     */
    config_cleanup(config);

    /*
     * Alte Konfiguration wieder übernehmen.
     */
    *config = old_config;
    old_config = (struct app_config){0};

    log_warning("Restoring previous configuration");

    /*
     * Module mit der bisherigen Konfiguration wieder starten.
     */
    if (rrd_module_init(config) != 0) {
        log_error("Cannot restore RRD module with previous configuration");
        return -1;
    }

    if (mqtt_init(config) != 0) {
        log_error("Cannot restore MQTT module with previous configuration");

        rrd_module_cleanup();
        return -1;
    }

    log_notice("Previous configuration restored");

    return -1;
}
