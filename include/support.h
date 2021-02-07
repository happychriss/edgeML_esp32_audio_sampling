//
// Created by development on 21.11.20.
//

#ifndef MS_ESP32_SUPPORT_H
#define MS_ESP32_SUPPORT_H
#define MYDEBUG
#ifdef MYDEBUG
#define MYDEBUG_CORE
#define DP(x)     Serial.print (x)
#define DPD(x)     Serial.print (x, DEC)
#define DPL(x)  Serial.println (x)
#else
#define DP(x)
#define DPD(x)
#define DPL(x)
#endif


/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */



#endif //MS_ESP32_SUPPORT_H
