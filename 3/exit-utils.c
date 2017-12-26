#include "exit-utils.h"
#include <stdarg.h>


int exitUnless( int condicional, int codigoErro, FILE* streamMsgErro, char *stringErro, ... ) {

	if ( !condicional ) {

		va_list args ;
		va_start (args, stringErro);
		vfprintf(streamMsgErro, stringErro, args);
		va_end(args);

		exit(codigoErro);

	}

}


int exitIf( int condicional, int codigoErro, FILE* streamMsgErro, char *stringErro, ... ) {

	if ( condicional ) {

			va_list args ;
			va_start (args, stringErro);
			vfprintf(streamMsgErro, stringErro, args);
			va_end(args);

			exit(codigoErro);

		}

}
