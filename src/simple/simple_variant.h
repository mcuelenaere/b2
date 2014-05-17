#ifndef __LLVM_VARIANT_H_
#define __LLVM_VARIANT_H_

#ifdef __cplusplus
extern "C" {
#endif

enum VariantType {
    VariantNoneType,
    VariantDoubleType,
    VariantIntegerType,
    VariantBoolType,
    VariantStringType,
    VariantArrayType,
    VariantMapType,
};

struct KeyValueTuple;

struct Variant {
    union {
        double d;
        long l;
        bool b;
        const char* s;
        struct Variant* a;
        struct KeyValueTuple* m;
    } v;
    enum VariantType type;
};

struct KeyValueTuple {
    const char* key;
    struct Variant value;
};

#ifdef __cplusplus
}
#endif

#endif /* __LLVM_VARIANT_H_ */
