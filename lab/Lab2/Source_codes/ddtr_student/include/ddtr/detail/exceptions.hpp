#pragma once
#include <cstdlib>

// Keep native clients compatible with -fno-exceptions. Exceptional failures
// terminate in that mode; exception-enabled clients retain their old behavior.
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#define DDTR_TRY try
#define DDTR_CATCH_ALL catch (...)
#define DDTR_RETHROW() throw
#define DDTR_THROW(error) throw error
#else
#define DDTR_TRY if (true)
#define DDTR_CATCH_ALL else if (false)
#define DDTR_RETHROW() std::abort()
#define DDTR_THROW(error) std::abort()
#endif
