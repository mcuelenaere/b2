#include "php_template.h"

#include <stdbool.h>
#include <stdint.h>

#include <main/php.h>
#include <Zend/zend_API.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_hash.h>

#define ALWAYS_INLINE __attribute__((always_inline))

void php_prototype(HashTable* assignments, struct template_buffer* buffer)
{
}

static void add_to_buffer(struct template_buffer* buffer, zval* str)
{
    int str_len = Z_STRLEN_P(str);

    if (buffer->str_length + str_len > buffer->allocated_length) {
        size_t new_length = buffer->allocated_length + (str_len > BUFFER_CHUNK_SIZE ? str_len : BUFFER_CHUNK_SIZE);
        buffer->ptr = erealloc(buffer->ptr, new_length);
        buffer->allocated_length = new_length;
    }

    memcpy(&buffer->ptr[buffer->str_length], Z_STRVAL_P(str), str_len);
    buffer->str_length += str_len;
}

ALWAYS_INLINE void print_double(double v, struct template_buffer* buffer)
{
    zval zv;
    INIT_ZVAL(zv);
    ZVAL_DOUBLE(&zv, v);
    convert_to_string(&zv);
    add_to_buffer(buffer, &zv);
    zval_dtor(&zv);
}

ALWAYS_INLINE void print_integer(long v, struct template_buffer* buffer)
{
    zval zv;
    INIT_ZVAL(zv);
    ZVAL_LONG(&zv, v);
    convert_to_string(&zv);
    add_to_buffer(buffer, &zv);
    zval_dtor(&zv);
}

ALWAYS_INLINE void print_boolean(bool v, struct template_buffer* buffer)
{
    zval zv;
    INIT_ZVAL(zv);
    ZVAL_STRING(&zv, v ? "1" : "", false);
    add_to_buffer(buffer, &zv);
    // don't destruct zv, because it doesn't contain a string copy!
}

ALWAYS_INLINE void print_string(const char* v, struct template_buffer* buffer)
{
    zval zv;
    INIT_ZVAL(zv);
    ZVAL_STRING(&zv, v, false);
    add_to_buffer(buffer, &zv);
    // don't destruct zv, because it doesn't contain a string copy!
}

ALWAYS_INLINE void print_variant(zval* v, struct template_buffer* buffer)
{
    if (Z_TYPE_P(v) != IS_STRING) {
        zval str_v;
        INIT_ZVAL(str_v);
        ZVAL_COPY_VALUE(&str_v, v);
        convert_to_string(&str_v);
        add_to_buffer(buffer, &str_v);
        zval_dtor(&str_v);
    } else {
        add_to_buffer(buffer, v);
    }
}

bool compare_variant(const uint16_t op, bool* cmp_result, zval* left, zval* right)
{
#define STR_TO_SHORT(x, y) (uint16_t)((x << 8) | y)
	switch (op) {
		case STR_TO_SHORT('&', '&'):
			*cmp_result = zval_is_true(left) && zval_is_true(right);
			return true;
		case STR_TO_SHORT('|', '|'):
			*cmp_result = zval_is_true(left) || zval_is_true(right);
			return true;
		default:
			// continued below
			break;
	}

	zval z_result;
    long result;
    INIT_ZVAL(z_result);
    if (compare_function(&z_result, left, right TSRMLS_CC) == FAILURE) {
        return false;
    }
    result = Z_LVAL(z_result);

    switch (op) {
        case STR_TO_SHORT('=', '='):
            *cmp_result = (result == 0);
            return true;
        case STR_TO_SHORT('!', '='):
            *cmp_result = (result != 0);
            return true;
        case STR_TO_SHORT('<', '='):
            *cmp_result = (result <= 0);
            return true;
        case STR_TO_SHORT('>', '='):
            *cmp_result = (result >= 0);
            return true;
        case STR_TO_SHORT('>', '\0'):
            *cmp_result = (result > 0);
            return true;
        case STR_TO_SHORT('<', '\0'):
            *cmp_result = (result < 0);
            return true;
        default:
            zend_throw_exception_ex(NULL, 0, "Unknown compare operator '%c%c'", op >> 8, op & 0xFF);
            return false;
    }
#undef STR_TO_SHORT
}

bool binary_variant_operation(const char op, zval** result, zval* left, zval* right)
{
    MAKE_STD_ZVAL(*result);

    switch (op) {
        case '+':
            return add_function(*result, left, right) == SUCCESS;
        case '-':
            return sub_function(*result, left, right) == SUCCESS;
        case '*':
            return mul_function(*result, left, right) == SUCCESS;
        case '/':
            return div_function(*result, left, right) == SUCCESS;
        case '%':
            return mod_function(*result, left, right) == SUCCESS;
        default:
            zend_throw_exception_ex(NULL, 0, "Unknown binary operator '%c'", op);
            return false;
    }
}

bool unary_variant_operation(const char op, zval** result, zval* value)
{
	MAKE_STD_ZVAL(*result);

	switch (op) {
		case '!':
			return boolean_not_function(*result, value) == SUCCESS;
		case '+': {
			zval zero;
			ZVAL_LONG(&zero, 0);
			return add_function(*result, &zero, value) == SUCCESS;
		}
		case '-': {
			zval zero;
			ZVAL_LONG(&zero, 0);
			return sub_function(*result, &zero, value) == SUCCESS;
		}
		default:
			zend_throw_exception_ex(NULL, 0, "Unknown unnary operator '%c'", op);
			return false;
	}
}

/*
 * The return value of this function should never be destroyed nor refcount decremented!
 */
zval* get_value_from_hashtable(HashTable* map, const char* key, uint keyLength)
{
    zval** value;
    // HashTable API expect key lengths to be the length of the null terminated string, including the null termination
    if (zend_hash_find(map, key, keyLength + 1, (void**) &value) != SUCCESS) {
        // return NULL zval
        return &zval_used_for_init;
    }

    return *value;
}

/*
 * The return value of this function should never be destroyed nor refcount decremented!
 */
zval* get_attribute(zval* map, const char* key, uint keyLength)
{
    HashTable* ht;

    if (Z_TYPE_P(map) == IS_OBJECT) {
        ht = Z_OBJPROP_P(map);
    } else if (Z_TYPE_P(map) == IS_ARRAY) {
        ht = Z_ARRVAL_P(map);
    } else {
        // shouldn't ever happen, return NULL zval
        return &zval_used_for_init;
    }

    return get_value_from_hashtable(ht, key, keyLength);
}

zval* do_method_call(const char* functionName, uint functionNameLength, zend_uint param_count, zval* params[])
{
    zval *return_value;

    zval z_function_name;
    INIT_ZVAL(z_function_name);
    ZVAL_STRINGL(&z_function_name, functionName, functionNameLength, false);

    if (call_user_function(CG(function_table), NULL, &z_function_name, return_value, param_count, params TSRMLS_CC) != SUCCESS) {
        ALLOC_INIT_ZVAL(return_value);
    }

    zval_dtor(&z_function_name);

    // we assume the refcount of the returned value is already ok
    return return_value;
}

/*
 * Resets the internal hash pointer of `value` to zero and returns the HashTable, if it has one and contains more than 1 element;
 */
ALWAYS_INLINE HashTable* forloop_init(zval* value, HashPosition* ht_pos)
{
    HashTable* ht;
    if (Z_TYPE_P(value) == IS_OBJECT) {
        ht = Z_OBJPROP_P(value);
    } else if (Z_TYPE_P(value) == IS_ARRAY) {
        ht = Z_ARRVAL_P(value);
    } else {
        return NULL;
    }

    zend_hash_internal_pointer_reset_ex(ht, ht_pos);

    return zend_hash_num_elements(ht) > 0 ? ht : NULL;
}

/*
 * Sets `key` and `value` to the key and value of the current position `ht_pos` in hashtable `ht`.
 *
 * NOTE: `value` should _not_ be destructed after use, `key` _should_ be destructed after use
 */
ALWAYS_INLINE void forloop_getvalues(HashTable* ht, HashPosition* ht_pos, zval** value, zval** key)
{
    if (value) {
        zval** _value;
        if (zend_hash_get_current_data_ex(ht, (void**) &_value, ht_pos) == SUCCESS) {
            // set value
            *value = *_value;
        } else {
            // set value to NULL zval
            *value = &zval_used_for_init;
        }
    }

    if (key) {
        // init key with empty variable and refcount of 1
        MAKE_STD_ZVAL(*key);

        zend_hash_get_current_key_zval_ex(ht, *key, ht_pos);
    }
}

ALWAYS_INLINE bool forloop_next(HashTable* ht, HashPosition* ht_pos)
{
    return (zend_hash_move_forward_ex(ht, ht_pos) == SUCCESS) && (*ht_pos != NULL);
}

ALWAYS_INLINE void set_zval_double(zval* zv, double value)
{
    INIT_PZVAL(zv);
    ZVAL_DOUBLE(zv, value);
}

ALWAYS_INLINE void set_zval_bool(zval* zv, bool value)
{
    INIT_PZVAL(zv);
    ZVAL_BOOL(zv, value);
}

ALWAYS_INLINE void set_zval_int(zval* zv, long value)
{
    INIT_PZVAL(zv);
    ZVAL_LONG(zv, value);
}

ALWAYS_INLINE void set_zval_string_nocopy(zval* zv, const char* value)
{
    INIT_PZVAL(zv);
    ZVAL_STRING(zv, value, false);
}

ALWAYS_INLINE void destruct_zval(zval* v)
{
    // decrement refcount and destroy if necessary
    zval_ptr_dtor(&v);
}
