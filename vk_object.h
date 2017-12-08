#ifndef _VK_OBJECT_H_
#define _VK_OBJECT_H_

#include "viper.h"
#include "stdarg.h"

#define KLASS_SIZE(klass_size)  sizeof(klass_size)
#define KLASS_NAME(klass_name)  #klass_name

#define         declare_klass(klass) \
                    vk_object_t *klass = &(vk_object_t)

#define         require_klass(klass) \
                    extern vk_object_t *klass

#define         vk_object_create(klass, ...) \
                    vk_object_construct(klass, __VA_ARGS__)

vk_object_t*    vk_object_construct(const void *klass, ...);

#define         vk_object_assert(klass, type) \
                    _assert_klass((char *)VK_OBJECT(klass)->name, \
                    (char *) #type, VK_OBJECT(klass)->size, \
                    sizeof(type))

#define         vk_object_demote(object, type) \
                    _demote_klass(VK_OBJECT(object), \
                    (char *) #type, sizeof(type))

struct _vk_object_s
{
    size_t          size;
    const char      *name;
    int             (*ctor)         (vk_object_t *, va_list *, ...);
    int             (*dtor)         (vk_object_t *);
    int             (*kmio)         (vk_object_t *, int32_t);
};

static inline int
_assert_klass(char *kn1, char *kn2, int ks1, int ks2)
{
    if((strcmp(kn1, kn2) != 0) || (ks1 != ks2)) return 0;

    return 1;
}

static inline void
_demote_klass(vk_object_t *object, char *kn, int ks)
{
    object->name = kn;
    object->size = ks;

    return;
}

#endif
