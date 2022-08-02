/**
 * @file  vl53l1_platform.h
 * @brief Those platform functions are platform dependent and have to be implemented by the user
 */
 
#ifndef _VL53L1_PLATFORM_H_
#define _VL53L1_PLATFORM_H_

#include <stdint.h>

/** @brief VL53L1_WriteMulti() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_WriteMulti(
		uint8_t 			dev,
		uint16_t      index,
		uint8_t      *pdata,
		uint32_t      count);
/** @brief VL53L1_ReadMulti() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_ReadMulti(
		uint8_t 			dev,
		uint16_t      index,
		uint8_t      *pdata,
		uint32_t      count);
/** @brief VL53L1_WrByte() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_WrByte(
		uint8_t dev,
		uint16_t      index,
		uint8_t       data);
/** @brief VL53L1_WrWord() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_WrWord(
		uint8_t dev,
		uint16_t      index,
		uint16_t      data);
/** @brief VL53L1_WrDWord() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_WrDWord(
		uint8_t dev,
		uint16_t      index,
		uint32_t      data);
/** @brief VL53L1_RdByte() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_RdByte(
		uint8_t dev,
		uint16_t      index,
		uint8_t      *pdata);
/** @brief VL53L1_RdWord() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_RdWord(
		uint8_t dev,
		uint16_t      index,
		uint16_t     *pdata);
/** @brief VL53L1_RdDWord() definition.\n
 * To be implemented by the developer
 */
uint8_t VL53L1_RdDWord(
		uint8_t dev,
		uint16_t      index,
		uint32_t     *pdata);


#endif
