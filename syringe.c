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

#include <windows.h>
#include <stdio.h>
#include <syringe.h>
#include <assembly.h>

static HINSTANCE dlls[256];

DWORD syringe_relative(DWORD from, DWORD to)
{
    return from <= to ? to - from : ~(from - to) + 1;
}

__declspec(dllexport) int __stdcall syringe_attach(void **func, void *repl)
{
    printf("    0x%08X -> 0x%08X\n", (unsigned int)*func, (unsigned int)repl);

    /* FIXME: stuff everything below to a single memory allocation with correct offsets */

    /* analyze the start of the hook destination to make a necessary size copy with JMP back */
    char mem[16];
    int align = 0;
    ReadProcessMemory(GetCurrentProcess(), *func, mem, sizeof(mem), NULL);
    assembly_data *a = assembly_new(sizeof(mem));

    assembly_buf(a, mem, 16);
    assembly_reset(a);

    while (align < 5)
    {
        int cur = assembly_next(a);

        if (cur < 0)
        {
            printf("      failed to calculate function alignment\n");
            return SYRINGE_ERROR;
        }

        align += cur;
    }

    printf("      function alignment is %d bytes\n", align);

    void *cb = VirtualAlloc(NULL, sizeof(mem), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    printf("      bridge at %08X\n", (unsigned int)cb);

    DWORD rel_cb = syringe_relative((DWORD)cb + align + 5, (DWORD)*func + align);

    mem[align] = 0xE9;
    memcpy(mem + align + 1, &rel_cb, 4);

    WriteProcessMemory(GetCurrentProcess(), cb, mem, sizeof(mem), NULL);
    assembly_free(a);

    /* this is the JMP which is written over the original code */
    a = assembly_new(5);
    assembly_put(a, 1, 0xE9);
    assembly_dw(a, syringe_relative((unsigned int)*func + 5, (unsigned int)repl));
    WriteProcessMemory(GetCurrentProcess(), *func, a->data, a->pos, NULL);
    assembly_free(a);

    /* overwrite old function pointer with bridge */
    *func = cb;

    return SYRINGE_SUCCESS;
}

static int syringe_main()
{
    HANDLE fh;
    WIN32_FIND_DATA fd;

    syringe_exports export_table = { syringe_attach };

    memset(dlls, 0, sizeof(dlls));

    printf("Searching for modules...\n");

    fh = FindFirstFile("syringe-*.dll", &fd);

    if (fh)
    {
        int idx = 0;

        do
        {
            printf("Loading module %s\n", fd.cFileName);
            dlls[idx] = LoadLibrary(fd.cFileName);
            if (dlls[idx])
            {
                SYRINGE_INIT syringe_init = (SYRINGE_INIT)GetProcAddress(dlls[idx], "syringe_init");

                printf("  loaded at 0x%08X\n", (unsigned int)dlls[idx]);
                printf("  syringe_init @ 0x%08X\n", (unsigned int)syringe_init);

                if (syringe_init)
                {
                    syringe_init(&export_table);
                }
                else
                {
                    printf("  no entrypoint found, ignoring this dll\n");
                    FreeLibrary(dlls[idx]);
                    dlls[idx] = NULL;
                }

                idx++;
            }
            else
            {
                printf("Error loading module!\n");
            }
        } while(FindNextFile(fh, &fd));

        FindClose(fh);
    }

    printf("All modules injected!\n");

    return 0;
}

static void syringe_free()
{
    for (int i = 0; i < 256; i++)
    {
        if (dlls[i] == NULL)
        {
            break;
        }

        printf("Unloading module 0x%08X\n", (unsigned int)dlls[i]);
        FreeLibrary(dlls[i]);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        syringe_main();
    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        syringe_free();
    }
    
    return TRUE;
}
