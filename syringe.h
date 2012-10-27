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

#ifndef __SYRINGE_H_

#ifdef __GNUC__
    #define GET_THIS(this) __asm__ __volatile__ ("movl %%ecx, %0" : "=m" (this))
    #define SET_THIS(this) __asm__ __volatile__ ("movl %0, %%ecx" : "=m" (this))
#else
    #error "Need explicit compiler support for thiscall"
#endif

enum {
    SYRINGE_ERROR,
    SYRINGE_SUCCESS
};

typedef struct {
    int __stdcall (*attach)(void **func, void *repl);
} syringe_exports;

#define __SYRINGE_H_
#endif
