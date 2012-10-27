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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assembly.h>

assembly_data *assembly_new(int size)
{
    assembly_data *d = malloc(sizeof(assembly_data));
    d->data = malloc(size);
    memset(d->data, 0, size);
    d->size = size;
    d->pos = 0;
    return d;
}

void assembly_free(assembly_data *d)
{
    free(d->data);
    free(d);
}

void assembly_put(assembly_data *d, int cnt, ...)
{
    va_list ap;

    va_start(ap, cnt);

    for (int i=0; i < cnt; i++)
    {
        memset(d->data+d->pos, (unsigned char)va_arg(ap, int), 1);
        d->pos++;
    }

    va_end(ap);
}

void assembly_dw(assembly_data *d, unsigned int dw)
{
    assembly_put(d, 4, dw, dw >> 8, dw >> 16, dw >> 24);
}

void assembly_buf(assembly_data *d, char *buf, int len)
{
    memcpy(d->data+d->pos, buf, len);
    d->pos += len;
}

void assembly_reset(assembly_data *d)
{
    d->pos = 0;
}

/* very crude way to calculate function alignment for 5 byte JMP */
int assembly_next(assembly_data *d)
{
    unsigned char *cur = (unsigned char *)d->data + d->pos;

    /* PUSH ECX */
    if (*cur == 0x51)
    {
        d->pos++;
        return 1;
    }

    /* PUSH EDX */
    if (*cur == 0x52)
    {
        d->pos++;
        return 1;
    }

    /* PUSH EBX */
    if (*cur == 0x53)
    {
        d->pos++;
        return 1;
    }

    /* PUSH EBP */
    if (*cur == 0x55)
    {
        d->pos++;
        return 1;
    }

    /* PUSH ESI */
    if (*cur == 0x56)
    {
        d->pos++;
        return 1;
    }

    /* PUSH EDI */
    if (*cur == 0x57)
    {
        d->pos++;
        return 1;
    }

    /* MOV EBP,ESP */
    if (*cur == 0x89)
    {
        cur++;
        d->pos++;

        if (*cur == 0xE5)
        {
            d->pos++;
            return 2;
        }
    }

    /* SUB ESP,<INT8> */
    if (*cur == 0x83)
    {
        cur++;
        d->pos++;

        if (*cur == 0xEC)
        {
            d->pos+=2;
            return 3;
        }
    }

    /* MOV EBP,ESP */
    if (*cur == 0x8B)
    {
        cur++;
        d->pos++;

        if (*cur == 0xEC)
        {
            d->pos++;
            return 2;
        }
    }

    printf("assembly_next: Error analyzing opcode %02X\n", *cur);
    return -1;
}

void assembly_dump(assembly_data *d)
{
    printf("Dumping %d bytes of compiled code from %p in hex:\n", d->pos, d);
    for (int i = 0; i < d->pos; i++)
    {
        printf(" %02X", (unsigned char)(*(d->data + i)));
        if ((i+1) % 8 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}
