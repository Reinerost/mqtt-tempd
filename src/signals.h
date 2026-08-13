/*
 * signals.h
 *
 * Öffentliche Schnittstelle zur Signalbehandlung des Dienstes.
 */

#ifndef SIGNALS_H
#define SIGNALS_H


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int signals_init(void);
int signals_terminate_requested(void);


#endif /* SIGNALS_H */
