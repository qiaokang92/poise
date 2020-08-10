// totp.h
#ifndef __TOTP_H
#define __TOTP_H

#include <linux/time.h>

void			totp_init(uint8_t* hmacKey, uint8_t keyLength, uint32_t timeStep);
void			totp_setTimezone(uint8_t timezone);
uint32_t	totp_getCodeFromTimestamp(uint32_t timeStamp);
uint32_t	totp_getCodeFromTimeStruct(struct tm time);
uint32_t	totp_getCodeFromSteps(uint32_t steps);

#endif // __TOTP_H
