// sha1.c

#include <linux/string.h>
#include "sha1.h"

static uint8_t	bufferOffset;
static uint32_t	byteCount;
static uint8_t	keyBuffer[BLOCK_LENGTH];
static uint8_t	innerHash[HASH_LENGTH];

static union _sha1_buffer {
	uint8_t		b[BLOCK_LENGTH];
	uint32_t	w[BLOCK_LENGTH/4];
} sha1_buffer;

static union _sha1_state {
	uint8_t		b[HASH_LENGTH];
	uint32_t	w[HASH_LENGTH/4];
} sha1_state;

static uint8_t sha1InitState[] = {
  0x01,0x23,0x45,0x67, // H0
  0x89,0xab,0xcd,0xef, // H1
  0xfe,0xdc,0xba,0x98, // H2
  0x76,0x54,0x32,0x10, // H3
  0xf0,0xe1,0xd2,0xc3  // H4
};

static uint32_t rol32(uint32_t number, uint8_t bits) {
  return ((number << bits) | (uint32_t)(number >> (32-bits)));
}

static void hashBlock() {
  uint8_t i;
  uint32_t a,b,c,d,e,t;

  a = sha1_state.w[0];
  b = sha1_state.w[1];
  c = sha1_state.w[2];
  d = sha1_state.w[3];
  e = sha1_state.w[4];
  for (i=0; i<80; i++) {
    if (i>=16) {
      t = sha1_buffer.w[(i+13)&15] ^ sha1_buffer.w[(i+8)&15] ^ sha1_buffer.w[(i+2)&15] ^ sha1_buffer.w[i&15];
      sha1_buffer.w[i&15] = rol32(t,1);
    }
    if (i<20) {
      t = (d ^ (b & (c ^ d))) + SHA1_K0;
    } else if (i<40) {
      t = (b ^ c ^ d) + SHA1_K20;
    } else if (i<60) {
      t = ((b & c) | (d & (b | c))) + SHA1_K40;
    } else {
      t = (b ^ c ^ d) + SHA1_K60;
    }
    t+=rol32(a,5) + e + sha1_buffer.w[i&15];
    e=d;
    d=c;
    c=rol32(b,30);
    b=a;
    a=t;
  }
  sha1_state.w[0] += a;
  sha1_state.w[1] += b;
  sha1_state.w[2] += c;
  sha1_state.w[3] += d;
  sha1_state.w[4] += e;
}

static void addUncounted(uint8_t data) {
  sha1_buffer.b[bufferOffset ^ 3] = data;
  bufferOffset++;
  if (bufferOffset == BLOCK_LENGTH) {
    hashBlock();
    bufferOffset = 0;
  }
}

static void pad() {
  // Implement SHA-1 padding (fips180-2 ˜5.1.1)

  // Pad with 0x80 followed by 0x00 until the end of the block
  addUncounted(0x80);
  while (bufferOffset != 56) addUncounted(0x00);

  // Append length in the last 8 bytes
  addUncounted(0); // We're only using 32 bit lengths
  addUncounted(0); // But SHA-1 supports 64 bit lengths
  addUncounted(0); // So zero pad the top bits
  addUncounted(byteCount >> 29); // Shifting to multiply by 8
  addUncounted(byteCount >> 21); // as SHA-1 supports bitstreams as well as
  addUncounted(byteCount >> 13); // byte.
  addUncounted(byteCount >> 5);
  addUncounted(byteCount << 3);
}

void sha1_init(void) {
  memcpy(sha1_state.b, sha1InitState, HASH_LENGTH);
  byteCount = 0;
  bufferOffset = 0;
}

void sha1_write(uint8_t data) {
  ++byteCount;
  addUncounted(data);

  return;
}

void sha1_writeArray(uint8_t *buffer, uint8_t size){
    while (size--) {
        sha1_write(*buffer++);
    }
}

uint8_t* sha1_result(void) {
  // Pad to complete the last block
  pad();

  // Swap byte order back
  uint8_t i;
  for (i=0; i<5; i++) {
    uint32_t a,b;
    a = sha1_state.w[i];
    b = a<<24;
    b |= (a<<8) & 0x00ff0000;
    b |= (a>>8) & 0x0000ff00;
    b |= a>>24;
    sha1_state.w[i] = b;
  }

  // Return pointer to hash (20 characters)
  return sha1_state.b;
}

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

void sha1_initHmac(const uint8_t* key, uint8_t keyLength) {
  uint8_t i;
  memset(keyBuffer,0,BLOCK_LENGTH);
  if (keyLength > BLOCK_LENGTH) {
    // Hash long keys
    sha1_init();
    for (;keyLength--;) sha1_write(*key++);
    memcpy(keyBuffer, sha1_result(), HASH_LENGTH);
  } else {
    // Block length keys are used as is
    memcpy(keyBuffer,key,keyLength);
  }
  // Start inner hash
  sha1_init();
  for (i=0; i<BLOCK_LENGTH; i++) {
    sha1_write(keyBuffer[i] ^ HMAC_IPAD);
  }
}

uint8_t* sha1_resultHmac(void) {
  uint8_t i;
  // Complete inner hash
  memcpy(innerHash, sha1_result(), HASH_LENGTH);
  // Calculate outer hash
  sha1_init();
  for (i=0; i<BLOCK_LENGTH; i++) sha1_write(keyBuffer[i] ^ HMAC_OPAD);
  for (i=0; i<HASH_LENGTH; i++) sha1_write(innerHash[i]);
  return sha1_result();
}
