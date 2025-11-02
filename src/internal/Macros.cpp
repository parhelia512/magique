// SPDX-License-Identifier: zlib-acknowledgement
#include <magique/internal/Macros.h>
#include <magique/util/Logging.h>

namespace magique
{

    void internal::AssertHandler(const char* expr, const char* file, const int line, const char* function,
                                 const char* message)
    {
        LogEx(LEVEL_FATAL, file, line, function, "Assert failed %s:%d\n %s:%s\nMessage: %s\n", file, line, function,
              expr, message);
    }

} // namespace magique
