 /*
 * mqtt.h
 *
 * Aufgabe:
 * Öffentliche Schnittstelle für die MQTT-Verbindung und die Verarbeitung
 * eingehender MQTT-Nachrichten.
 */

#ifndef MQTT_H
#define MQTT_H


/* ==========================================================================
 * Includes
 * ========================================================================== */

#include "config.h"


/* ==========================================================================
 * Public functions
 * ========================================================================== */

/**
 * Initialisiert den MQTT-Client und verbindet ihn mit dem Broker.
 *
 * @param config Zeiger auf die aktuelle Anwendungskonfiguration.
 *
 * @return 0 bei Erfolg, sonst -1.
 */
int mqtt_init(const struct app_config *config);


/**
 * Verarbeitet anstehende MQTT-Ereignisse.
 *
 * Die Funktion soll regelmäßig aus der Hauptschleife aufgerufen werden.
 *
 * @return 0 bei Erfolg, sonst -1.
 */
int mqtt_process(void);


/**
 * Trennt die MQTT-Verbindung und gibt alle Ressourcen frei.
 */
void mqtt_cleanup(void);


#endif /* MQTT_H */
