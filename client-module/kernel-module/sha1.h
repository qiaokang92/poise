// sha1.h

#ifndef __SHA_1_H
#define __SHA_1_H

#define HASH_LENGTH		20
#define BLOCK_LENGTH	64

#define SHA1_K0  0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

void sha1_init(void);
void sha1_initHmac(const uint8_t* secret, uint8_t secretLength);
uint8_t* sha1_result(void);
uint8_t* sha1_resultHmac(void);
void sha1_write(uint8_t);
void sha1_writeArray(uint8_t* buffer, uint8_t size);

#endif // __SHA_1_H
