/* liteRTC, Using Real time clock of STM8L151F3 easily
 * Coded By TinLethax way back in May. Converted to library at 2021/09/07 +7
 */
 
 #ifndef LITERTC_H
 #define LITERTC_H
 
 
 void liteRTC_Init();
 void liteRTC_SetDMYBCD(uint8_t SetDay,uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear);
 void liteRTC_SetDMY(uint8_t SetDay,uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear);
 void liteRTC_SetHMSBCD(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond);
 void liteRTC_SetHMS(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond);
 void liteRTC_grepTime(uint8_t *rH, uint8_t *rM, uint8_t * rS);
 void liteRTC_grepDate(uint8_t *rDay, uint8_t *rDate, uint8_t *rMo, uint8_t *rYe);
 
 #endif