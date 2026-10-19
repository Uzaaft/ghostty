// Supply a typed function pointer to the JavaScript callback without putting
// any test logic here. The smoke test exercises the linked archive's C API.
#include <ghostty/vt.h>

__attribute__((import_module("test"), import_name("log")))
extern void log_callback(void* userdata,
                         GhosttySysLogLevel level,
                         const uint8_t* scope,
                         size_t scope_len,
                         const uint8_t* message,
                         size_t message_len);

GhosttySysLogFn callback_pointer(void) {
    return log_callback;
}
