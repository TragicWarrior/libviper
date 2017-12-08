#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "viper.h"
#include "private.h"
#include "vk_object.h"


/*
    this is our static template which will be passed into the object
    allocator and also to the constructor.
*/
declare_klass(VK_OBJECT_KLASS)
{
    .name = KLASS_NAME(vk_object_t),
    .size = KLASS_SIZE(vk_object_t),
};

vk_object_t*
vk_object_construct(const void *klass, ...)
{
    vk_object_t     *object;
    va_list         argp;

    if(klass == NULL) return NULL;

    object = calloc(1, VK_OBJECT(klass)->size);

    // copy template to newly alloced object
    memcpy(object, klass, sizeof(vk_object_t));

    if(object->ctor != NULL)
    {
        va_start(argp, klass);

        // pass the pointer to the variable argument list structure
        object->ctor(object, &argp);

        va_end(argp);
    }

    return object;
}

int
vk_object_push_keystroke(vk_object_t *object, int32_t keystroke)
{
    int retval;

    if(object == NULL) return -1;

    if(object->kmio != NULL)
    {
        retval = object->kmio(object, keystroke);
    }

    return retval;
}

int
vk_object_destroy(vk_object_t *object)
{
    /*
        call the objects destructor.  if it was installed correctly, object
        tear down should cascade from the topmost derivative until it hits
        the bottom--which is _vk_object_dtor().
    */
    if(!vk_object_assert(object, vk_object_t))
    {
        object->dtor(object);
    }

    free(object);

    return 0;
}

