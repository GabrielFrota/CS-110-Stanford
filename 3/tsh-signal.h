/**
 * File: tsh-signal.h
 * ------------------
 * Defines the installSignalHandler function, which is defined
 * under the name Signal in B&O, 2nd edition on page 752.
 */

#ifndef _tsh_signal_
#define _tsh_signal_

/**
 * Type: handler_t
 * ---------------
 * Defines the class of functions that accept a single integer
 * and return nothing at all.  This function class is compatible
 * with the types of the functions that can be installed to handle
 * signals.
 */

typedef void (*handler_t)(int);

/**
 * Function: installSignalHandler
 * ------------------------------
 * Installs the specified function to catch and handle any 
 * and all signals of the specified signum.
 */
handler_t installSignalHandler(int signum, handler_t handler);

#endif
