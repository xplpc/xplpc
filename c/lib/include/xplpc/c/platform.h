#ifndef XPLPC_C_PLATFORM_H
#define XPLPC_C_PLATFORM_H

#include "xplpc/c/typedefs.h"

#if defined(_WIN32)
#define XPLPC_API __declspec(dllexport)
#else
#define XPLPC_API __attribute__((visibility("default"))) __attribute__((used))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    XPLPC_API
    xplpc_handle xplpc_attach(const xplpc_host *host);

    XPLPC_API
    void xplpc_detach(xplpc_handle layer);

    // Detaches a layer whose host is gone without reaching that host again, which is what a runtime calls from a finalizer as it shuts down, so the handle arrives through the one pointer a finalizer hands over.
    XPLPC_API
    void xplpc_abandon(const xplpc_handle *layer);

    XPLPC_API
    void xplpc_declare(xplpc_handle layer, const char *name, size_t nameSize);

    XPLPC_API
    void xplpc_retract(xplpc_handle layer, const char *name, size_t nameSize);

    XPLPC_API
    bool xplpc_call(xplpc_handle layer, uint64_t token, const char *request, size_t requestSize, xplpc_call_result *result);

    XPLPC_API
    bool xplpc_cancel(xplpc_handle layer, xplpc_handle call);

    XPLPC_API
    void xplpc_answer(xplpc_handle call, const char *response, size_t responseSize);

    XPLPC_API
    xplpc_handle xplpc_buffer_register(xplpc_handle layer, void *data, size_t size);

    // Answers false and sets the address to null and the size to zero when the handle names no registered buffer.
    XPLPC_API
    bool xplpc_buffer_read(xplpc_handle buffer, void **data, size_t *size);

    XPLPC_API
    bool xplpc_buffer_release(xplpc_handle layer, xplpc_handle buffer);

    XPLPC_API
    size_t xplpc_pending(void);

    XPLPC_API
    void xplpc_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
