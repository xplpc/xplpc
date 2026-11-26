/* A host reaching this library through the abi writes plain c, so the public headers are compiled as c here rather than only as c++. */

#include "xplpc/c/platform.h"
#include "xplpc/c/typedefs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*AnyEntryPoint)(void);

static void onInvoke(void *context, xplpc_handle call, const char *request, size_t requestSize)
{
    (void)context;
    (void)call;
    (void)request;
    (void)requestSize;
}

static void onPost(void *context, xplpc_post_kind kind, uint64_t id, char *data, size_t dataSize)
{
    (void)context;
    (void)kind;
    (void)id;
    (void)dataSize;

    xplpc_free(data);
}

size_t xplpcCEntryPointCount(void);

size_t xplpcCEntryPointCount(void)
{
    const xplpc_invoke_callback invoke = onInvoke;
    const xplpc_post_callback post = onPost;
    const xplpc_host host = {NULL, invoke, post, false, "host", 4};
    const xplpc_call_result result = {0, NULL, 0};
    const xplpc_post_kind kinds[] = {XPLPC_POST_ANSWER, XPLPC_POST_INVOKE, XPLPC_POST_RELEASE};
    const xplpc_handle layer = 0;

    const AnyEntryPoint entryPoints[] = {
        (AnyEntryPoint)xplpc_attach,
        (AnyEntryPoint)xplpc_detach,
        (AnyEntryPoint)xplpc_abandon,
        (AnyEntryPoint)xplpc_declare,
        (AnyEntryPoint)xplpc_retract,
        (AnyEntryPoint)xplpc_call,
        (AnyEntryPoint)xplpc_cancel,
        (AnyEntryPoint)xplpc_answer,
        (AnyEntryPoint)xplpc_buffer_register,
        (AnyEntryPoint)xplpc_buffer_read,
        (AnyEntryPoint)xplpc_buffer_release,
        (AnyEntryPoint)xplpc_pending,
        (AnyEntryPoint)xplpc_free,
    };

    if (!host.invoke || !host.post || host.nameSize != 4 || result.data || layer != 0 || sizeof(kinds) / sizeof(kinds[0]) != 3)
    {
        return 0;
    }

    return sizeof(entryPoints) / sizeof(entryPoints[0]);
}
