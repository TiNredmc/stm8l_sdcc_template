/* request unique ID. Coded by TinLethax */ 

#include "dev_id.h"

unsigned int req_dev_id(int dev_address){ // from 0x4926 (address 0) to 0x4931 (address 12)
 return (_MEM_(0x4926 + dev_address));
}

uint16_t req_wafer_x_co(){ //return x co-ordinate on wafer in 16 bit int
	uint16_t xco ;
	xco = ((_MEM_(0x4927)) << 8) || (_MEM_(0x4926)) ; //combine the data from the address 0x4926 and 0x4927 into 16 bit integer number of x co-ordinate on wafer
 return xco ;
}

uint16_t req_wafer_y_co(){ //return y co-ordinate on wafer in 16 bit int
	uint16_t yco ;
	yco = ((_MEM_(0x4929)) << 8) || (_MEM_(0x4928)) ; //combine the data from the address 0x4928 and 0x4927 into 16 bit integer number of y co-ordinate on wafer
 return yco ;
}

unsigned int req_wafer_num(){ //return wafer number
 return _MEM_(0x492A) ;
}