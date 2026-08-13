/*
 * rrd.h
 *
 * Öffentliche Schnittstelle für die Verwaltung der
 * RRD-Datenbanken.
 */

#ifndef RRD_H
#define RRD_H

#include "config.h"

/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int rrd_module_init(const struct app_config *config);
void rrd_module_cleanup(void);

int rrd_store_value(const struct sensor_config *sensor,
                    double value);
#endif /* RRD_H */
