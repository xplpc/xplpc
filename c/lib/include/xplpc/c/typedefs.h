#ifndef XPLPC_C_TYPEDEFS_H
#define XPLPC_C_TYPEDEFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A layer, a call or a buffer, which is never zero when it is valid.
typedef uint64_t xplpc_handle;

typedef enum
{
    XPLPC_POST_ANSWER = 0,
    XPLPC_POST_INVOKE = 1,
    XPLPC_POST_RELEASE = 2,
} xplpc_post_kind;

// Serves a call synchronously, and the request is borrowed for the length of the callback.
typedef void (*xplpc_invoke_callback)(void *context, xplpc_handle call, const char *request, size_t requestSize);

// Carries everything the host receives later, in the order it happened, and the host owns the data and frees it with xplpc_free.
typedef void (*xplpc_post_callback)(void *context, xplpc_post_kind kind, uint64_t id, char *data, size_t dataSize);

typedef struct
{
    void *context;
    xplpc_invoke_callback invoke;
    xplpc_post_callback post;
    bool enterableAnywhere;

    // A host attaching under the name another one is attached under takes its place, which is what a runtime restarted in the same process does, and the one it replaces is never reached again.
    const char *name;
    size_t nameSize;
} xplpc_host;

typedef struct
{
    xplpc_handle call;
    char *data;
    size_t dataSize;
} xplpc_call_result;

#endif
