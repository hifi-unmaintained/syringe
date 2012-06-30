/*
 * Copyright (c) 2011, 2012 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(__i386__) && !defined(_M_IX86)
#error "Only X86 is supported"
#endif

#if !defined(_INTPTR_T_DEFINED) && !defined(_intptr_t_defined)
#include <inttypes.h>
#endif

#ifndef __SYRINGE_H_

typedef struct
{
    intptr_t func;      /* local function to map */
    intptr_t addr;      /* address in the program memory to patch */
    intptr_t callback;  /* address to call the original function */
    const char *name;   /* for logging, optional */
} syringe_hook;

#define SYRINGE_EXPORT(...) \
    __declspec(dllexport) int _syringe_export(syringe_hook ***_hooks) \
    { \
        static syringe_hook *a[] = { \
            __VA_ARGS__ \
        }; \
        *_hooks = a; \
        return sizeof a / sizeof (syringe_hook *); \
    }

#define SYRINGE_HOOKDEF(type, name, ...) \
    type name##_func(__VA_ARGS__); \
    syringe_hook name;

#define SYRINGE_HOOK(addr, type, name, ...) \
    XSYRINGE_HOOK(addr, type, name, #type, #name, #__VA_ARGS__, __VA_ARGS__)

#define XSYRINGE_HOOK(addr, type, name, stype, sname, sargs, ...) \
    SYRINGE_HOOKDEF(type, name, __VA_ARGS__) \
    syringe_hook name = { (intptr_t)name##_func, (intptr_t)addr, 0xBAADF00D, stype " " sname "(" sargs ")" }; \
    type name##_func(__VA_ARGS__)

#define SYRINGE_CALL(name, ...) \
    name##_func(__VA_ARGS__)

typedef int (__cdecl *SYRINGE_CDECL_TYPE)();
typedef int (__stdcall *SYRINGE_STDCALL_TYPE)();
typedef int (__fastcall *SYRINGE_FASTCALL_TYPE)();

#define SYRINGE_CDECL(name, ...) \
    ((SYRINGE_CDECL_TYPE)name.callback)(__VA_ARGS__)
#define SYRINGE_STDCALL(name, ...) \
    ((SYRINGE_STDCALL_TYPE)name.callback)(__VA_ARGS__)
#define SYRINGE_FASTCALL(name, ...) \
    ((SYRINGE_FASTCALL_TYPE)name.callback)(__VA_ARGS__)

#define __SYRINGE_H_
#endif
