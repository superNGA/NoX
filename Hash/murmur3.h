//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source
// code.

#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_


#ifdef __cplusplus
extern "C" {
#endif



//-----------------------------------------------------------------------------

void MurmurHash3_x86_32 (const void *key, int len, unsigned int seed, void *out);

void MurmurHash3_x86_128(const void *key, int len, unsigned int seed, void *out);

void MurmurHash3_x64_128(const void *key, int len, unsigned int seed, void *out);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // _MURMURHASH3_H_
