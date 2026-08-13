/*
 * signals.c
 *
 * Initialisiert die Signalbehandlung und verwaltet die Anforderung
 * zum Beenden des Dienstes.
 */


/* ------------------------------------------------------------------------- */
/* Includes                                                                  */
/* ------------------------------------------------------------------------- */

#include <signal.h>
#include <stddef.h>

#include "signals.h"


/* ------------------------------------------------------------------------- */
/* Local variables                                                           */
/* ------------------------------------------------------------------------- */

static volatile sig_atomic_t terminate_requested = 0;


/* ------------------------------------------------------------------------- */
/* Local functions                                                           */
/* ------------------------------------------------------------------------- */

static void signal_handler(int signal_number)
{
    (void)signal_number;
    terminate_requested = 1;
}


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

int signals_init(void)
{
    struct sigaction action;

    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) != 0)
        return -1;

    if (sigaction(SIGTERM, &action, NULL) != 0)
        return -1;

    return 0;
}


int signals_terminate_requested(void)
{
    return terminate_requested != 0;
}
