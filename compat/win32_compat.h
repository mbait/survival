#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

// Minimal stand-ins for the Win32 / DirectX 9 types and macros referenced
// throughout the original 2006 codebase. Allows the game layer (TIMER,
// PLAYER, particle/model code, HRESULT-returning functions) to compile
// on non-MSVC toolchains without rewriting every call site.
//
// On Windows the Win32 SDK already supplies HRESULT, DWORD, BYTE, WORD,
// LPSTR / LPCSTR / LPVOID, FAILED / SUCCEEDED, S_OK / E_FAIL, and
// GetTickCount() via <windows.h>; we pull a lean subset of windows.h in
// here so any TU that includes this shim is self-sufficient on MSVC too.

#ifdef _WIN32

  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>

  // <windows.h> with WIN32_LEAN_AND_MEAN does not pull in <rpcndr.h>,
  // so the lowercase `byte` typedef the legacy code uses isn't defined.
  // Add it explicitly. Safe even when <rpcndr.h> is later included
  // because both definitions are `unsigned char`.
  using byte = unsigned char;

#else  // !_WIN32

  #include <chrono>
  #include <cstdint>
  #include <limits>

  using BYTE     = std::uint8_t;
  using byte     = std::uint8_t;
  using WORD     = std::uint16_t;
  using DWORD    = std::uint32_t;
  using LPSTR    = char*;
  using LPCSTR   = const char*;
  using LPVOID   = void*;
  using HRESULT  = int;

  constexpr HRESULT S_OK   = 0;
  constexpr HRESULT E_FAIL = -1;

  #define SUCCEEDED(hr) ((hr) >= 0)
  #define FAILED(hr)    ((hr) <  0)

  // Millisecond tick count, matching Win32 GetTickCount() semantics
  // (monotonic, wraps at 2^32 ms ~= 49 days — same as the original).
  // Uses std::chrono::steady_clock so the shim layer has no SDL
  // dependency.
  inline DWORD GetTickCount()
  {
      using namespace std::chrono;
      return static_cast<DWORD>(
          duration_cast<milliseconds>(
              steady_clock::now().time_since_epoch()).count());
  }

#endif // !_WIN32

// ---- always-available constants (both platforms) -----------------------
// D3DX_PI was a macro in the original D3DX9 headers; we don't link that
// library any more, so define it ourselves on every platform.
#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif

// _HUGE — MSVC float-limits sentinel used in physics2D as a "very large
// value" initial guess for shortest-distance scans. The Win32 UCRT
// declares it as `extern double const _HUGE;` in <float.h>; do NOT
// redefine it as a macro on Windows or <corecrt_math.h>'s declaration
// at line 78 expands to invalid syntax. On other platforms there's no
// such symbol, so a macro to +infinity is the cleanest substitute.
#ifdef _WIN32
  #include <cfloat>  // gives us _HUGE as `extern double const`
#else
  #include <limits>
  #define _HUGE (std::numeric_limits<double>::infinity())
#endif

#endif // WIN32_COMPAT_H
