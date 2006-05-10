#ifndef KARMUTILITY_H
#define KARMUTILITY_H

#include <QString>

/**
 * Format time for output.  All times output on screen or report output go
 * through this function.
 *
 * If the second argument is true, the time is output as a two-place decimal.
 * Otherwise the format is hh:mi.
 *
 */
QString formatTime( long minutes, bool decimal=false );

#endif
