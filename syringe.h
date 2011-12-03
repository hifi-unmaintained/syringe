/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
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

#include <stdint.h>

#ifndef __SYRINGE_H_

extern intptr_t *_syringe_cb_addr;

typedef struct
{
    intptr_t func;      /* local function to map */
    intptr_t addr;      /* address in the program memory to patch */
    const char *name;   /* for logging, optional */
} syringe_hook;

#define SYRINGE_CDECL ((int __cdecl (*)())*_syringe_cb_addr)
#define SYRINGE_STDCALL ((int __stdcall (*)())*_syringe_cb_addr)
#define SYRINGE_FASTCALL ((int __fastcall (*)())*_syringe_cb_addr)

#define SYRINGE_CB(func, addr) { (intptr_t)func, (intptr_t)addr, NULL }
#define SYRINGE_CB_EX(func, addr, name) { (intptr_t)func, (intptr_t)addr, (const char *)name }
#define SYRINGE_EXPORT(...) \
    intptr_t *_syringe_cb_addr; \
    __declspec(dllexport) int _syringe_export(syringe_hook **_hooks, intptr_t *cb) \
    { \
        static syringe_hook a[] = { \
            __VA_ARGS__ \
        }; \
        *_hooks = (syringe_hook *)&a; \
        _syringe_cb_addr = cb; \
        return sizeof(a) / sizeof(syringe_hook); \
    }

#define __SYRINGE_H_
#endif
