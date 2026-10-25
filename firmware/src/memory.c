/* Freestanding compiler support; no heap or C library is required. */
#include <stddef.h>
void *memcpy(void *destination,const void *source,size_t n) {
    volatile unsigned char *d=destination;const volatile unsigned char *s=source;
    while(n--) *d++=*s++;
    return destination;
}
void *memset(void *destination,int value,size_t n) {
    volatile unsigned char *d=destination;
    while(n--) *d++=(unsigned char)value;
    return destination;
}
