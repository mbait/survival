#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

// Minimal stand-ins for the Win32 / DirectX 9 types and macros referenced
// throughout the original 2006 codebase. Allows large stretches of the game
// layer (TIMER, PLAYER, particle/model code, HRESULT-returning functions)
// to compile on non-MSVC toolchains without rewriting every call site.
//
// On a real Windows MSVC build this header collapses to nothing — the real
// definitions come from <windows.h> / <d3d9.h> instead.

#if defined(_WIN32)
// Real Win32 build: rely on the SDK headers, not these shims.
#else

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

// D3DX_PI as a float constant (was a D3DX macro in the original headers).
#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif

// _HUGE — MSVC float-limits sentinel from <float.h>. Used in physics2D
// as a "very large value" initial guess for shortest-distance scans.
#ifndef _HUGE
#define _HUGE (std::numeric_limits<double>::infinity())
#endif

// Millisecond tick count, matching Win32 GetTickCount() semantics
// (monotonic, wraps at 2^32 ms ~= 49 days — same as the original). Uses
// std::chrono::steady_clock so the shim layer has no SDL dependency.
inline DWORD GetTickCount()
{
    using namespace std::chrono;
    return static_cast<DWORD>(
        duration_cast<milliseconds>(
            steady_clock::now().time_since_epoch()).count());
}

#endif // !_WIN32

#endif // WIN32_COMPAT_H
