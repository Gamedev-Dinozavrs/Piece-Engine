// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
//
// Vendored from the .NET SDK (Microsoft.NETCore.App.Host) native headers.
// Only the subset required by Piece's minimal hosting bootstrap is kept:
// initializing hostfxr from a .runtimeconfig.json and obtaining the
// load_assembly_and_get_function_pointer delegate.

#ifndef HAVE_HOSTFXR_H
#define HAVE_HOSTFXR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#if defined(_WIN32)
    #define HOSTFXR_CALLTYPE __cdecl
    #ifdef _WCHAR_T_DEFINED
        typedef wchar_t char_t;
    #else
        typedef unsigned short char_t;
    #endif
#else
    #define HOSTFXR_CALLTYPE
    typedef char char_t;
#endif

enum hostfxr_delegate_type
{
    hdt_com_activation,
    hdt_load_in_memory_assembly,
    hdt_winrt_activation,
    hdt_com_register,
    hdt_com_unregister,
    hdt_load_assembly_and_get_function_pointer,
    hdt_get_function_pointer,
    hdt_load_assembly,
    hdt_load_assembly_bytes,
};

typedef void* hostfxr_handle;
struct hostfxr_initialize_parameters
{
    size_t size;
    const char_t *host_path;
    const char_t *dotnet_root;
};

// Initializes the hosting components using a .runtimeconfig.json file.
// This function does not load the runtime.
typedef int32_t(HOSTFXR_CALLTYPE *hostfxr_initialize_for_runtime_config_fn)(
    const char_t *runtime_config_path,
    const struct hostfxr_initialize_parameters *parameters,
    /*out*/ hostfxr_handle *host_context_handle);

// Gets a typed delegate from the currently loaded CoreCLR or from a newly created one.
typedef int32_t(HOSTFXR_CALLTYPE *hostfxr_get_runtime_delegate_fn)(
    const hostfxr_handle host_context_handle,
    enum hostfxr_delegate_type type,
    /*out*/ void **delegate);

// Closes an initialized host context.
typedef int32_t(HOSTFXR_CALLTYPE *hostfxr_close_fn)(const hostfxr_handle host_context_handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAVE_HOSTFXR_H
