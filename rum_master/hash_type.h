#ifndef __HASH_TYPE_H__
#define __HASH_TYPE_H__


#define HASH_LENGTH             33
#define HASH_STR(a)		((char*)(a).hash_str)

typedef struct hash_type{
	unsigned char hash_str[HASH_LENGTH];
} HASH_TYPE;


#endif /*__HASH_TYPE_H__*/