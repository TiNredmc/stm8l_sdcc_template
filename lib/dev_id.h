#ifndef DEV_ID_H
#define DEV_ID_H

#include "stm8l.h"
#include <stdint.h>

unsigned int req_dev_id(int dev_address);
uint16_t req_wafer_x_co();
uint16_t req_wafer_y_co();
unsigned int req_wafer_num();

#endif