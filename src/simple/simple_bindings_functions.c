#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "simple_variant.h"

#define ERR(...) fprintf(stderr, __VA_ARGS__)
#define ALWAYS_INLINE __attribute__((always_inline))

static const char* type_to_string(enum VariantType type)
{
    switch (type) {
        case VariantNoneType: return "none";
        case VariantDoubleType: return "double";
        case VariantIntegerType: return "integer";
        case VariantBoolType: return "bool";
        case VariantStringType: return "string";
        case VariantArrayType: return "array";
        case VariantMapType: return "map";
        default: return "unknown";
    }
}

ALWAYS_INLINE void print_double(double v)
{
    printf("%f", v);
}

ALWAYS_INLINE void print_integer(long v)
{
    printf("%ld", v);
}

ALWAYS_INLINE void print_boolean(bool v)
{
    printf("%d", v);
}

ALWAYS_INLINE void print_string(const char* v)
{
    printf("%s", v);
}

void print_variant(struct Variant v)
{
    switch (v.type) {
        case VariantStringType:
            print_string(v.v.s);
            break;
        case VariantDoubleType:
            print_double(v.v.d);
            break;
        case VariantIntegerType:
            print_integer(v.v.l);
            break;
        case VariantBoolType:
            print_boolean(v.v.b);
            break;
        // TODO: other types
    }
}

#define VARIANT_DOUBLE(x) ((struct Variant){ .type = VariantDoubleType, .v.d = (x) })
#define VARIANT_INTEGER(x) ((struct Variant){ .type = VariantIntegerType, .v.l = (x) })

bool compare_variant(const uint16_t op, struct Variant left, struct Variant right)
{
    if (left.type != VariantDoubleType && left.type != VariantIntegerType) {
        ERR("left isn't a double nor an integer, but a %s!", type_to_string(left.type));
        return false;
    } else if (right.type != VariantDoubleType && right.type != VariantIntegerType) {
        ERR("right isn't a double nor an integer, but a %s!", type_to_string(right.type));
        return false;
    }

#define LEFT (left.type == VariantDoubleType ? left.v.d : left.v.l)
#define RIGHT (right.type == VariantDoubleType ? right.v.d : right.v.l)
#define STR_TO_SHORT(x, y) (uint16_t)(x | (y << 8))

    switch (op) {
        case STR_TO_SHORT('=', '='): return LEFT == RIGHT;
        case STR_TO_SHORT('!', '='): return LEFT != RIGHT;
        case STR_TO_SHORT('<', '='): return LEFT <= RIGHT;
        case STR_TO_SHORT('>', '='): return LEFT >= RIGHT;
        case STR_TO_SHORT('>', 0): return LEFT > RIGHT;
        case STR_TO_SHORT('<', 0): return LEFT < RIGHT;
        default: ERR("unknown compare operation: %c%c!", op >> 8, op & 0xFF); break;
    }

#undef LEFT
#undef RIGHT
#undef STR_TO_SHORT
}

struct Variant binary_variant_operation(char op, struct Variant left, struct Variant right)
{
    if (left.type != VariantDoubleType && left.type != VariantIntegerType) {
        fprintf(stderr, "left isn't a double nor an integer, but a %s!", type_to_string(left.type));
        return VARIANT_INTEGER(0);
    } else if (right.type != VariantDoubleType && right.type != VariantIntegerType) {
        fprintf(stderr, "right isn't a double nor an integer, but a %s!", type_to_string(right.type));
        return VARIANT_INTEGER(0);
    }

    if (left.type == VariantDoubleType || right.type == VariantDoubleType) {
        // one of the two is a double, so return value is also a double
#define LEFT (left.type == VariantDoubleType ? left.v.d : left.v.l)
#define RIGHT (right.type == VariantDoubleType ? right.v.d : right.v.l)
        switch (op) {
            case '+': return VARIANT_DOUBLE(LEFT + RIGHT);
            case '-': return VARIANT_DOUBLE(LEFT - RIGHT);
            case '*': return VARIANT_DOUBLE(LEFT * RIGHT);
            case '/': return VARIANT_DOUBLE(LEFT / RIGHT); // TODO: zero check
            case '%': ERR("can't do modulus operation on float values!"); break;
            default: ERR("unknown binary operation: %c!", op); break;
        }
#undef LEFT
#undef RIGHT
    } else {
#define LEFT (left.v.l)
#define RIGHT (right.v.l)
        // both are integer, so return value is also integer
        switch (op) {
            case '+': return VARIANT_INTEGER(LEFT + RIGHT);
            case '-': return VARIANT_INTEGER(LEFT - RIGHT);
            case '*': return VARIANT_INTEGER(LEFT * RIGHT);
            case '/': return VARIANT_INTEGER(LEFT / RIGHT); // TODO: zero check
            case '%': return VARIANT_INTEGER(LEFT % RIGHT); // TODO: zero check
            default: ERR("unknown binary operation: %c!", op); break;
        }
#undef LEFT
#undef RIGHT
    }
}

struct KeyValue {
    const char* key;
    struct Variant value;
};

struct Variant lookup_variable(struct KeyValue* values, const char* key)
{
    for (; values->key; values++) {
        if (!strcmp(values->key, key)) {
            return values->value;
        }
    }

    // nothing found!
    struct Variant emptyStruct;
    emptyStruct.type = VariantBoolType;
    emptyStruct.v.b = false;
    return emptyStruct;
}
