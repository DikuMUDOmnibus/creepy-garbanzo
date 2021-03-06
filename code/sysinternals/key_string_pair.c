#include "sysinternals.h"
#include <stdlib.h> //malloc, calloc, free, _Exit
#include <string.h> //memset, strlen
#include <assert.h> //assert
#include <signal.h> //raise
#include <stdarg.h>
#include <stdio.h> //vsprintf

#ifdef S_SPLINT_S
#define _Exit exit
#endif



struct array_list *kvp_create_array(size_t numelements)
{
    struct array_list *array;

    array_list_create(array, struct key_string_pair, numelements);

    return array;
}

void kvp_array_append_copy(struct array_list *array, const char *key, const char *value)
{
    size_t keylength = strlen(key);
    size_t vallength = strlen(value);
    char *akey;
    char *avalue;
    struct key_string_pair *kvp;

    if (array->top == array->size)
    {
        if (raise(SIGABRT) != 0) {
            _Exit(EXIT_FAILURE);
        }
    }


    akey = calloc(keylength+1, sizeof(char));
    assert(akey != NULL);
    avalue = calloc(vallength+1, sizeof(char));
    assert(avalue != NULL);

    strncpy(akey, key, keylength);
    strncpy(avalue, value, vallength);
    kvp = &((struct key_string_pair *)array->items)[array->top];
    kvp->key = akey;
    kvp->value = avalue;
    array->top += 1;
}

const char *kvp_array_find(const struct array_list *array, const char *key)
{
    size_t idx;
    size_t keylen = strlen(key);

    for(idx = 0; idx < array->top; idx++) {
        if (strncmp(key, ((struct key_string_pair *)array->items)[idx].key, keylen+1) == 0)
            return ((struct key_string_pair *)array->items)[idx].value;
    }

    return NULL;
}

const char *kvp_array_valueat(const struct array_list *array, const size_t idx) {
    assert(idx < array->top);
    assert(idx > 0);
    return ((struct key_string_pair *)array->items)[idx].value;
}

void kvp_free_array(struct array_list *array)
{
    struct key_string_pair *node; 
    size_t i; 

    if (array == NULL)
        return;

    array_list_each(array, i) {
        node = array_list_node_at(array, struct key_string_pair, i);
        free((char *)node->key); 
        free((char *)node->value);
    } 
    array_list_free(array, struct key_string_pair, kvp_free);
}


#define CALC_HASH_BUCKET(key, numhashbuckets) ((HASHBUCKETTYPE)(calchashvalue((key)) % (HASHVALUETYPE)(numhashbuckets)))
#define DEFAULT_NUMHASHBUCKETS 53
struct keyvaluepairhash *keyvaluepairhash_create(struct array_list *array, size_t numelements, size_t numbuckets)
{
    int idx;
    struct keyvaluepairhash *hash;
    size_t bucketsize = (size_t)UCEILING((unsigned int)numelements, (unsigned int)numbuckets) + UCEILING((unsigned int)numelements, 10);

    hash = malloc(sizeof(struct keyvaluepairhash));
    assert(hash != NULL);
    hash->numhashbuckets = (numbuckets == 0 ? DEFAULT_NUMHASHBUCKETS : numbuckets);
    hash->lookup = calloc(sizeof(struct keyvaluepairhashnode), (size_t)hash->numhashbuckets);
    assert(hash->lookup != NULL);
    hash->masterlist = calloc(sizeof(key_string_pair_ptr), bucketsize * hash->numhashbuckets);
    assert(hash->masterlist != NULL);

    for (idx = 0; idx < hash->numhashbuckets; idx++) {
        struct keyvaluepairhashnode *hashnode = &hash->lookup[idx];
        // perfect world means numelements/numbuckets per bucket, but allow for margin of error.
        hashnode->size = bucketsize;
        hashnode->top = 0;
        hashnode->items = &hash->masterlist[idx * numelements];
        assert(hashnode->items != NULL);
    }

    for(idx = 0; idx < (int)numelements; idx++) {
        HASHBUCKETTYPE hashbucket = CALC_HASH_BUCKET(((struct key_string_pair *)array->items)[idx].key, hash->numhashbuckets);
        struct keyvaluepairhashnode *hashnode = &hash->lookup[hashbucket];
        if (hashnode->top == hashnode->size)
        {
            if (raise(SIGABRT) != 0) {
                _Exit(EXIT_FAILURE);
            }
        }

        ((key_string_pair_ptr *)hashnode->items)[hashnode->top] = &((struct key_string_pair *)array->items)[idx];
        hashnode->top++;
    }

    return hash;
}

const char *keyvaluepairhash_get(struct keyvaluepairhash *hash, const char * const key)
{
    HASHBUCKETTYPE hashbucket = CALC_HASH_BUCKET(key, hash->numhashbuckets);
    size_t keylen = strlen(key);
    struct keyvaluepairhashnode *hashnode = &hash->lookup[hashbucket];
    size_t idx;

    for (idx = 0; idx < hashnode->top; idx++) {
        if (strncmp(key, hashnode->items[idx]->key, keylen+1) == 0) {
            return hashnode->items[idx]->value;
        }
    }

    return NULL;
}

void keyvaluepairhash_free(struct keyvaluepairhash *hash)
{
    if (hash == NULL) return;

    free(hash->masterlist);
    free(hash->lookup);
    free(hash);
}



