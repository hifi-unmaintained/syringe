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

#include <windows.h>
#include <stdio.h>
#include <syringe.h>
#include <assembly.h>

static HINSTANCE dlls[256];
static int syringe_main();
static void syringe_free();
int syringe_current_cb = 0;

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

DWORD syringe_relative(DWORD from, DWORD to)
{
    return from <= to ? to - from : ~(from - to) + 1;
}

static int syringe_main()
{
    HANDLE fh;
    WIN32_FIND_DATA fd;

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
                FARPROC _syringe_export = GetProcAddress(dlls[idx], "_syringe_export");

                printf("  loaded at 0x%08X\n", (intptr_t)dlls[idx]);
                printf("  _syringe_export @ 0x%08X\n", (intptr_t)_syringe_export);

                if (_syringe_export)
                {
                    syringe_hook *hooks = NULL;
                    int ret = _syringe_export(&hooks, &syringe_current_cb);

                    printf("  returned %d hook(s) @ 0x%08X\n", ret, (intptr_t)hooks);

                    for (int i = 0; i < ret; i ++)
                    {
                        printf("    hook %s @ 0x%08X -> 0x%08X\n", (hooks[i].name ? hooks[i].name : "<unnamed>"), (intptr_t)hooks[i].func, (intptr_t)hooks[i].addr);

                        /* FIXME: stuff everything below to a single memory allocation with correct offsets */

                        /* analyze the start of the hook destination to make a necessary size copy with JMP back */
                        char mem[16];
                        int align = 0;
                        ReadProcessMemory(GetCurrentProcess(), (LPVOID)hooks[i].addr, mem, sizeof(mem), NULL);
                        _asm_data *a = _asm_new(sizeof(mem));

                        _asm_buf(a, mem, 16);
                        _asm_reset(a);

                        while (align < 5)
                        {
                            int cur = _asm_next(a);
                            if (cur < 0)
                                exit(1);
                            align += cur;
                        }

                        printf("      function alignment is %d bytes\n", align);

                        void *cb = VirtualAlloc(NULL, sizeof(mem), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                        printf("      callback at %08X\n", (unsigned int)cb);

                        DWORD rel_cb = syringe_relative((DWORD)cb + align + 5, (DWORD)hooks[i].addr + align);

                        mem[align] = 0xE9;
                        memcpy(mem + align + 1, &rel_cb, 4);

                        WriteProcessMemory(GetCurrentProcess(), cb, mem, sizeof(mem), NULL);
                        _asm_free(a);

                        void *bridge = VirtualAlloc(NULL, 16, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                        a = _asm_new(16);
                        _asm(a, 3, 0x36, 0xC7, 0x05);
                        _asm_dw(a, (DWORD)&syringe_current_cb);
                        _asm_dw(a, (DWORD)cb);
                        _asm(a, 1, 0xE9);
                        _asm_dw(a, syringe_relative((DWORD)bridge + 16, (DWORD)hooks[i].func));
                        WriteProcessMemory(GetCurrentProcess(), bridge, a->data, a->pos, NULL);
                        _asm_free(a);
                        printf("      bridge at %08X\n", (unsigned int)bridge);

                        /* this is the JMP which is written over the original code */
                        a = _asm_new(5);
                        _asm(a, 1, 0xE9);
                        _asm_dw(a, syringe_relative(hooks[i].addr + 5, (DWORD)bridge));
                        WriteProcessMemory(GetCurrentProcess(), (LPVOID)hooks[i].addr, a->data, a->pos, NULL);
                        _asm_free(a);
                    }
                }
                else
                {
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

        printf("Unloading module 0x%08X\n", (intptr_t)dlls[i]);
        FreeLibrary(dlls[i]);
    }
}
