#ifndef ACQUISITION_H
#define ACQUISITION_H
#include "scope.h"
void acquisition_init(void);
void acquisition_start(ScopeConfig cfg);
void acquisition_stop(void);
bool acquisition_ready(void);
bool acquisition_failed(void);
uint32_t acquisition_faults(void);
bool acquisition_copy(uint16_t data[SCOPE_N]); /* returns true for auto timeout */
#endif
