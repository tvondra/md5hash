/*
 * md5hash.c - data type representing md5 hash values (128-bit)
 *
 * Copyright (C) Tomas Vondra, 2011
 */

#include "postgres.h"
#include <string.h>

#include "access/htup.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define HASH_BYTES 16
#define HASH_CHARS 22 // urlsafe base64 编码
#define HASH_LENGTH (HASH_CHARS + 1)

PG_FUNCTION_INFO_V1(md5_bytea_in);

PG_FUNCTION_INFO_V1(md5_in);
PG_FUNCTION_INFO_V1(md5_out);

PG_FUNCTION_INFO_V1(md5_recv);
PG_FUNCTION_INFO_V1(md5_send);

PG_FUNCTION_INFO_V1(md5_eq);
PG_FUNCTION_INFO_V1(md5_neq);

PG_FUNCTION_INFO_V1(md5_leq);
PG_FUNCTION_INFO_V1(md5_lt);

PG_FUNCTION_INFO_V1(md5_geq);
PG_FUNCTION_INFO_V1(md5_gt);
PG_FUNCTION_INFO_V1(md5_cmp);

Datum md5_in(PG_FUNCTION_ARGS);
Datum md5_out(PG_FUNCTION_ARGS);

Datum md5_eq(PG_FUNCTION_ARGS);
Datum md5_neq(PG_FUNCTION_ARGS);

Datum md5_leq(PG_FUNCTION_ARGS);
Datum md5_lt(PG_FUNCTION_ARGS);

Datum md5_geq(PG_FUNCTION_ARGS);
Datum md5_gt(PG_FUNCTION_ARGS);

Datum md5_cmp(PG_FUNCTION_ARGS);

Datum md5_recv(PG_FUNCTION_ARGS);
Datum md5_send(PG_FUNCTION_ARGS);

typedef struct hash_t {
  unsigned char bytes[HASH_BYTES];
} hash_t;

__inline__ static char* encode(hash_t* h);
__inline__ static char decode(char byte[2]);

/* encode binary value (unsigned char) as a hex value */

__inline__ static char* encode(hash_t* hash)
{
  int i;
  static char chars[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_' };

  /* 22 chars max (+ a terminator) */
  char* result = (char*)palloc(HASH_LENGTH);

  memset(result, 0, HASH_LENGTH);

  /* first 64 bits */
  int j = 0;
  for (i = 0; i < 15; i += 3) {
    uint32_t triplet = (hash->bytes[i] << 16) | (hash->bytes[i + 1] << 8) | hash->bytes[i + 2];
    result[j++] = chars[(triplet >> 18) & 0x3F];
    result[j++] = chars[(triplet >> 12) & 0x3F];
    result[j++] = chars[(triplet >> 6) & 0x3F];
    result[j++] = chars[triplet & 0x3F];
  }

  uint32_t triplet = (hash->bytes[i] << 16);
  result[j++] = chars[(triplet >> 18) & 0x3F];
  result[j++] = chars[(triplet >> 12) & 0x3F];

  return result;
}

__inline__ static char
decode(char byte[2])
{
  unsigned char result = 0;

  /* process two characters at once */
  unsigned char a = toupper(byte[0]);
  unsigned char b = toupper(byte[1]);

  if (a >= 48 && a <= 57)
    result = 16 * (a - 48);
  else
    result = 16 * (a - 65 + 10);

  if (b >= 48 && b <= 57)
    result += (b - 48);
  else
    result += (b - 65 + 10);

  return result;
}

// 将字符转换成对应的数字
int char_to_num(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  else if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  else if (c >= '0' && c <= '9')
    return c - '0' + 52;
  else if (c == '-')
    return 62;
  else if (c == '_')
    return 63;
  else
    return -1;
}

// 对一个22字节的urlsafe base64字符串进行解码
void urlsafe_base64_decode(const char* src, unsigned char* dst)
{
  int i, j, s[4];
  for (i = 0, j = 0; i < 22; i += 4, j += 3) {
    s[0] = char_to_num(src[i]);
    s[1] = char_to_num(src[i + 1]);
    s[2] = char_to_num(src[i + 2]);
    s[3] = char_to_num(src[i + 3]);

    dst[j] = (s[0] << 2) | (s[1] >> 4);
    dst[j + 1] = (s[1] << 4) | (s[2] >> 2);
    dst[j + 2] = (s[2] << 6) | s[3];
  }
}

Datum md5_in(PG_FUNCTION_ARGS)
{
  int i = 0;
  char* str = PG_GETARG_CSTRING(0);

  /* 128bit = 2x 64bit */
  hash_t* result = (hash_t*)palloc(sizeof(hash_t));

  auto len = strlen(str);
  if (len == 22) // base64url encode
  {
    urlsafe_base64_decode(str, result->bytes);
    PG_RETURN_POINTER(result);
  }

  if (len != 32)
    ereport(ERROR,
        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
            errmsg("invalid input length for hash: \"%s\" , expected 22 chars (base64url) or 32 chars (hex)",
                str)));

  for (i = 0; i < 32; i++) {
    char c = toupper(str[i]);

    if (!((c >= 48 && c <= 57) || (c >= 65 && c <= 70)))
      ereport(ERROR,
          (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
              errmsg("invalid character: \"%c\" (expected 0-9, A-F)", c)));
  }

  /* first half (64 bits = 8 pairs) */
  for (i = 0; i < HASH_BYTES; i++)
    result->bytes[i] = decode(&str[2 * i]);

  PG_RETURN_POINTER(result);
}

Datum md5_out(PG_FUNCTION_ARGS)
{
  hash_t* hash = (hash_t*)PG_GETARG_POINTER(0);
  char* result = encode(hash);

  PG_RETURN_CSTRING(result);
}

Datum md5_eq(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r == 0);
}

Datum md5_neq(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r != 0);
}

Datum md5_leq(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r <= 0);
}

Datum md5_lt(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r < 0);
}

Datum md5_geq(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r >= 0);
}

Datum md5_gt(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_BOOL(r > 0);
}

Datum md5_cmp(PG_FUNCTION_ARGS)
{
  hash_t* a = (hash_t*)PG_GETARG_POINTER(0);
  hash_t* b = (hash_t*)PG_GETARG_POINTER(1);

  int r = memcmp(a->bytes, b->bytes, HASH_BYTES);

  PG_RETURN_INT32(r);
}

Datum md5_recv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);
  hash_t* result;

  result = (hash_t*)palloc(sizeof(hash_t));
  pq_copymsgbytes(buf, (char*)result->bytes, HASH_BYTES);

  PG_RETURN_POINTER(result);
}

Datum md5_send(PG_FUNCTION_ARGS)
{
  hash_t* hash = (hash_t*)PG_GETARG_POINTER(0);
  StringInfoData buf;

  pq_begintypsend(&buf);
  pq_sendbytes(&buf, (char*)hash->bytes, HASH_BYTES);

  PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum md5_bytea_in(PG_FUNCTION_ARGS)
{
  hash_t* result = (hash_t*)palloc(sizeof(hash_t));
  bytea* data = PG_GETARG_BYTEA_P(0);
  size_t data_length = VARSIZE_ANY_EXHDR(data);
  if (data_length != HASH_BYTES)
    ereport(ERROR,
        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
            errmsg("invalid input length for hash: expected %d , get %ld", HASH_BYTES, data_length)));

  memcpy(result->bytes, data->vl_dat, HASH_BYTES);

  PG_RETURN_POINTER(result);
}
