#pragma once

//------------------------------------------------------------------------------
// Compiler detection
//------------------------------------------------------------------------------
#ifdef _MSC_VER
#   if _MSC_FULL_VER >= 170050727 // NOV 2012 CTP
#       define BK_CONFIG_COMPILER_MSVC _MSC_FULL_VER
#   else
#       error unsupported compiler
#   endif
#else
#   error unsupported compiler
#endif

//------------------------------------------------------------------------------
// Architecture detection
//------------------------------------------------------------------------------
#ifdef BK_CONFIG_COMPILER_MSVC
#   if defined(_M_X64) || defined(_M_AMD64)
#       define BK_CONFIG_ARCH_X64
#   elif defined (_M_IX86)
#       define BK_CONFIG_ARCH_X86
#   elif defined (_M_ARM_FP)
#       define BK_CONFIG_ARCH_ARM
#   else
#       error unsupported architecture
#   endif
#else
#   error unsupported architecture
#endif

//------------------------------------------------------------------------------
// Platform detection
//------------------------------------------------------------------------------
#ifdef BK_CONFIG_COMPILER_MSVC
#   if defined(_WIN64)
#       define BK_CONFIG_PLATFORM_WIN 64
#   elif defined(_WIN32)
#       define BK_CONFIG_PLATFORM_WIN 32
#   else
#       error unsupported platform
#   endif
#else
#   error unsupported platform
#endif

//------------------------------------------------------------------------------
// Endianness detection
//------------------------------------------------------------------------------
#if defined(BK_CONFIG_ARCH_X64) || defined(BK_CONFIG_ARCH_X86)
#   define BK_CONFIG_ENDIAN_LITTLE
#   define BK_CONFIG_BYTE_ORDER 1234
#else
#   error unsupported endianness
#endif

//------------------------------------------------------------------------------
// Workaround for no noexcept in MSVC
//------------------------------------------------------------------------------
#if defined(BK_CONFIG_COMPILER_MSVC)
#   define BK_NOEXCEPT throw ()
#else
#   define BK_NOEXCEPT noexcept
#endif
