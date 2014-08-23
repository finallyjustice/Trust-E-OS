#ifndef _TRUSTZONE_COMMUNICATION_SERVICES_H_
#define _TRUSTZONE_COMMUNICATION_SERVICES_H_

#include "communication/types.h"

// STRING 服务
#define TASK_STRING_UUID            {0xD1059173, \
                                    0x97A7, \
                                    0xED77, \
                                    {0x61, 0x9D, 0xAB, \
                                     0x32, 0x6D, 0xBA, \
                                     0x89, 0x4B}} 
enum TASK_STRING_COMMAND {
    STRING_LOWER_COMMAND = 0x80000000,
    STRING_UPPER_COMMAND
};

// ROBUST 服务
// 2FA6CBA8-A166-E216-8B2E-5D732293F36E
#define  TASK_TEST_ROBUST_UUID       {0x2FA6CBA8, \
                                    0xA166, \
                                    0xE216, \
                                    {0x8B, 0x2E, 0x5D, \
                                     0x73, 0x22, 0x93, \
                                     0xF3, 0x6E}}

enum TASK_TEST_ROBUST_COMMAND {
    TEST_ROBUST_COMMUNICATION_COMMAND = 0x80000000
};

#endif
