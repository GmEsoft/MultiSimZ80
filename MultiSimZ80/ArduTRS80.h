#ifndef ARDUTRS80_H
#define ARDUTRS80_H

#ifdef __cplusplus
# define CFUNC extern "C"
#else
# define CFUNC
#endif


CFUNC void ArduTRS80_setup();

CFUNC void ArduTRS80_loop();

CFUNC void ArduTRS80_subloop( void * );

#endif
