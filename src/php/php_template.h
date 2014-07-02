#ifndef __PHP_TEMPLATE_H_
#define __PHP_TEMPLATE_H_

#include <stdint.h>

#include <Zend/zend_API.h>

#ifdef __cplusplus
extern "C" {
#endif

struct template_buffer {
    char* ptr;
    size_t str_length;
    size_t allocated_length;
};
#define BUFFER_CHUNK_SIZE 256

typedef void (*template_fn)(HashTable*, struct template_buffer*, HashTable*);

#ifdef __cplusplus
}
#endif

#endif // __PHP_TEMPLATE_H_
