/* Copyright  (C) 2010-2017 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (nbio_unixmmap.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if defined(HAVE_MMAP) && defined(__linux__)

#include <stdio.h>
#include <stdlib.h>

#include <file/nbio.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/mman.h>

struct nbio_mmap_unix_t
{
   int fd;
   int map_flags;
   size_t len;
   void* ptr;
};

static void *nbio_mmap_unix_open(const char * filename, unsigned mode)
{
   static const int o_flags[] =   { O_RDONLY,  O_RDWR|O_CREAT|O_TRUNC, O_RDWR,               O_RDONLY,  O_RDWR|O_CREAT|O_TRUNC };
   static const int map_flags[] = { PROT_READ, PROT_WRITE|PROT_READ,   PROT_WRITE|PROT_READ, PROT_READ, PROT_WRITE|PROT_READ   };
   
   size_t len;
   void* ptr                       = NULL;
   struct nbio_mmap_unix_t* handle = NULL;
   int fd                          = open(filename, o_flags[mode]|O_CLOEXEC, 0644);
   if (fd < 0)
      return NULL;
   
   len = lseek(fd, 0, SEEK_END);
   if (len != 0)
      ptr = mmap(NULL, len, map_flags[mode], MAP_SHARED, fd, 0);

   if (ptr == MAP_FAILED)
   {
      close(fd);
      return NULL;
   }
   
   handle            = malloc(sizeof(struct nbio_mmap_unix_t));
   handle->fd        = fd;
   handle->map_flags = map_flags[mode];
   handle->len       = len;
   handle->ptr       = ptr;
   return handle;
}

static void nbio_mmap_unix_begin_read(void *data)
{
   /* not needed */
}

static void nbio_mmap_unix_begin_write(void *data)
{
   /* not needed */
}

static bool nbio_mmap_unix_iterate(void *data)
{
   return true; /* not needed */
}

static void nbio_mmap_unix_resize(void *data, size_t len)
{
   struct nbio_mmap_unix_t* handle = (struct nbio_mmap_unix_t*)data;
   if (!handle)
      return;
   if (len < handle->len)
   {
      /* this works perfectly fine if this check is removed, but it 
       * won't work on other nbio implementations */
      /* therefore, it's blocked so nobody accidentally relies on it */
      puts("ERROR - attempted file shrink operation, not implemented");
      abort();
   }

   if (ftruncate(handle->fd, len) != 0)
   {
      puts("ERROR - couldn't resize file (ftruncate)");
      abort(); /* this one returns void and I can't find any other 
                  way for it to report failure */
   }

   munmap(handle->ptr, handle->len);

   handle->ptr = mmap(NULL, len, handle->map_flags, MAP_SHARED, handle->fd, 0);
   handle->len = len;

   if (handle->ptr == MAP_FAILED)
   {
      puts("ERROR - couldn't resize file (mmap)");
      abort();
   }
}

static void *nbio_mmap_unix_get_ptr(void *data, size_t* len)
{
   struct nbio_mmap_unix_t* handle = (struct nbio_mmap_unix_t*)data;
   if (!handle)
      return NULL;
   if (len)
      *len = handle->len;
   return handle->ptr;
}

static void nbio_mmap_unix_cancel(void *data)
{
   /* not needed */
}

static void nbio_mmap_unix_free(void *data)
{
   struct nbio_mmap_unix_t* handle = (struct nbio_mmap_unix_t*)data;
   if (!handle)
      return;
   close(handle->fd);
   munmap(handle->ptr, handle->len);
   free(handle);
}

nbio_intf_t nbio_mmap_unix = {
   nbio_mmap_unix_open,
   nbio_mmap_unix_begin_read,
   nbio_mmap_unix_begin_write,
   nbio_mmap_unix_iterate,
   nbio_mmap_unix_resize,
   nbio_mmap_unix_get_ptr,
   nbio_mmap_unix_cancel,
   nbio_mmap_unix_free,
   "nbio_mmap_unix",
};

#endif
