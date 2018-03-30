/*
 * Copyright (c) 2011-2015 CrystaX.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY CrystaX ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CrystaX OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of CrystaX.
 */

#include <sys/auxv.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <crystax/atomic.h>
#include <crystax/log.h>

#if __LP64__
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

#define DUMPABLE_VALUE 1

static ElfW(auxv_t) empty_auxv = {
    AT_NULL,
    { 0 }
};

static ElfW(auxv_t) *readauxv()
{
    int fd;
    ElfW(auxv_t) *auxv = NULL;
    size_t auxvlen = 0;
    size_t auxvidx = 0;
    ElfW(auxv_t) buf;
    size_t buflen = 0;
    const char *fname = "/proc/self/auxv";

    int dumpable;
    struct rlimit rl;

    dumpable = prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);
    if (dumpable < 0)
        PANIC("can't get PR_GET_DUMPABLE: %s", strerror(errno));

    if (dumpable != DUMPABLE_VALUE) {
        if (prctl(PR_SET_DUMPABLE, DUMPABLE_VALUE, 0, 0, 0) < 0) {
            CRYSTAX_ERR("can't set PR_SET_DUMPABLE: %s", strerror(errno));
            return &empty_auxv;
        }

        rl.rlim_cur = 0;
        rl.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rl) < 0) {
            CRYSTAX_ERR("Can't set RLIMIT_CORE: %s", strerror(errno));
            return &empty_auxv;
        }
    }

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        CRYSTAX_ERR("can't open %s: %s", fname, strerror(errno));
        return &empty_auxv;
    }

    for (;;)
    {
        ssize_t n;
        n = read(fd, (uint8_t*)&buf + buflen, sizeof(buf) - buflen);
        if (n == -1)
            PANIC("can't read %s: %s", fname, strerror(errno));
        if (n == 0)
            break;

        buflen += n;
        if (buflen < sizeof(buf))
            continue;

        if (auxvidx >= auxvlen)
        {
            auxvlen += sizeof(buf);
            ElfW(auxv_t) *p = (ElfW(auxv_t)*)realloc(auxv, auxvlen * sizeof(buf));
            if (!p)
                PANIC("can't allocate ELF AUX vector");
            auxv = p;
        }

        memcpy(&auxv[auxvidx], &buf, sizeof(buf));

        buflen = 0;
        ++auxvidx;
    }

    close(fd);

    if (dumpable != DUMPABLE_VALUE) {
        if (prctl(PR_SET_DUMPABLE, dumpable, 0, 0, 0) < 0)
            PANIC("can't set PR_SET_DUMPABLE(%d): %s", dumpable, strerror(errno));
    }

    return auxv;
}

unsigned long int getauxval(unsigned long int type)
{
    static ElfW(auxv_t) *__crystax_auxv = NULL;

    ElfW(auxv_t) * auxv = __crystax_atomic_fetch(&__crystax_auxv);
    if (!auxv)
    {
        auxv = readauxv();
        __crystax_atomic_swap(&__crystax_auxv, auxv);
    }

    for (; auxv->a_type != AT_NULL; ++auxv)
    {
        if (auxv->a_type == type)
            return auxv->a_un.a_val;
    }

    return 0;
}
