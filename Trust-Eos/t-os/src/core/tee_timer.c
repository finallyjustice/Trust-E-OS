#include "tee_timer.h"
#include "tee_internal_api.h"


void TEE_Delay(uint32_t mill_seconds)
{
    volatile uint32_t i,j,k;
    for (i=0; i!= mill_seconds; i++) {
        for (j=0; j<0xff; j++) {
            for (k=0; k<0xff; k++) {
            }
        }
    }
}