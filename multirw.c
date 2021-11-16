/**
* MultiRW: A multi-threaded program to stress a file system with parallel IOs
*          in a single shared file.
*
* URL       https://github.com/jyvet/multirw
* License   MIT
* Author    Jean-Yves VET <contact[at]jean-yves.vet>
* Copyright (c) 2021
******************************************************************************/

#define _GNU_SOURCE /* Required to use O_DIRECT flag */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <pthread.h>
#include "args.h"

static mrw_args_t g_args;

typedef struct thread_args
{
    pthread_t  thread;
    uint32_t   thread_id;
    uint32_t   seed;
    int        fd;
    void      *read_buf;
    void      *write_buf;
    char      *fd_mmap;
} thread_args_t;

/**
 * Open the destination file with given flag, create if necessary
 */
static int
mrw_file_open_internal(int arg_flag)
{
    int flags = arg_flag | O_CREAT;
    if (g_args.is_cache_bypass)
        flags |= O_DIRECT;

    int fd = open(g_args.file_path, flags, 0644);
    if (fd <= 0)
    {
        fprintf(stderr, "Unable to open %s: %s\n",
                g_args.file_path, strerror(errno));
        exit(1);
    }
    return fd;
}

/**
 * Open the destination file based on the IO type
 */
static int
mrw_file_open(void)
{
    switch (g_args.io_type)
    {
    case MRW_READ:
        return mrw_file_open_internal(O_RDONLY);
    case MRW_WRITE:
        return mrw_file_open_internal(O_WRONLY);
    case MRW_RW:
        return mrw_file_open_internal(O_RDWR);
    default:
        return -1;
    }
}

/**
 * Initialize the destination file (create + truncate to final size)
 */
static void
mrw_file_init(void)
{
    int fd = mrw_file_open_internal(O_RDWR);
    int ret = ftruncate(fd, g_args.file_size);
    if (ret < 0)
    {
        fprintf(stderr, "Unable to truncate %s: %s\n",
                g_args.file_path, strerror(errno));
        exit(1);
    }
    close(fd);
}

/**
 * Perform a read operation (pread or memory mapped)
 *
 * @param   args[inout]        Pointer to the thread arguments
 * @param   bytes[in]          Size of the IO (bytes)
 * @param   offest[in]         Offset of the IO in the file (bytes)
 */
static void
mrw_read(thread_args_t *args, size_t bytes, off_t offset)
{
    ssize_t ret = bytes;
    void *mmap = args->fd_mmap + offset;
    void *ret_mmap = args->read_buf;

    if (g_args.is_mmap)
        ret_mmap = memcpy(args->read_buf, mmap, bytes);
    else
        ret = pread(args->fd, args->read_buf, bytes, offset);

    if (ret != bytes || ret_mmap != args->read_buf)
    {
        fprintf(stderr, "Read error (fd: %d, offset: %lu, size: %lu) : %ld\n",
                args->fd, offset, bytes, ret);
        exit(1);
    }
}

/**
 * Perform a write operation (pwrite or memory mapped)
 *
 * @param   args[inout]        Pointer to the thread arguments
 * @param   bytes[in]          Size of the IO (bytes)
 * @param   offest[in]         Offset of the IO in the file (bytes)
 */
static void
mrw_write(thread_args_t *args, size_t bytes, off_t offset)
{
    ssize_t ret = bytes;
    void *mmap = args->fd_mmap + offset;
    void *ret_mmap = mmap;

    if (g_args.is_mmap)
        ret_mmap = memcpy(mmap, args->write_buf, bytes);
    else
        ret = pwrite(args->fd, args->write_buf, bytes, offset);

    if (ret != bytes || ret_mmap != mmap)
    {
        fprintf(stderr, "Write error (fd: %d, offset: %lu, size: %lu) : %ld\n",
                args->fd, offset, bytes, ret);
        exit(1);
    }
}

/**
 * Do an IO burst = single IO type (read or write).
 *
 * @param   args[inout]        Pointer to the thread arguments
 */
static void
mrw_do_io_burst(thread_args_t *args)
{
    io_type_t io_type;

    /* Figure out the IO type */
    if (g_args.io_type != MRW_RW)
        io_type = g_args.io_type;
    else
        io_type = (rand_r(&args->seed) % 2) ? MRW_READ : MRW_WRITE;

    if (io_type == MRW_READ)
    {
        if (g_args.verbosity_lvl)
            printf("Thread #%u \t- read burst (%u IOs with random size & offset)\n",
                   args->thread_id, g_args.io_burst_count);

        for (int i = 0; i < g_args.io_burst_count; i++)
        {
            size_t size = rand_r(&args->seed) % g_args.io_size_max;
            off_t offset = rand_r(&args->seed) % (g_args.file_size - size);
            mrw_read(args, size, offset);
        }
    }
    else
    {
        if (g_args.verbosity_lvl)
            printf("Thread #%u \t- write burst (%u IOs with random size & offset)\n",
                   args->thread_id, g_args.io_burst_count);

        for (int i = 0; i < g_args.io_burst_count; i++)
        {
            size_t size = rand_r(&args->seed) % g_args.io_size_max;
            off_t offset = rand_r(&args->seed) % (g_args.file_size - size);
            mrw_write(args, size, offset);
        }
    }
}

/**
 * Open destination file for the given thread
 *
 * @param   targs[inout]    Pointer to the thread arguments
 */
void
io_stream_open(thread_args_t *targs)
{
    targs->fd = mrw_file_open();
    if (g_args.is_mmap)
    {
        targs->fd_mmap = mmap(0, g_args.file_size,
                             PROT_READ | PROT_WRITE, MAP_SHARED,
                             targs->fd, 0);
        if (targs->fd_mmap == MAP_FAILED)
        {
            fprintf(stderr, "Unable to mmap file: %s\n",
                    strerror(errno));
            exit(1);
        }
    }
}

/**
 * IO stream for a thread.
 *
 * @param   args[in]        Pointer to the thread arguments
 * @return  void*           As defined by the thread function signature
 */
void *
io_stream(void *args)
{
    time_t start_time = time(NULL);

    thread_args_t *targs = (thread_args_t *)args;
    targs->seed          = g_args.first_seed + targs->thread_id;
    if (g_args.is_multiple_fd)
        io_stream_open(targs);

    /* Allocate read and write buffers */
    targs->write_buf = malloc(g_args.io_size_max);
    assert(targs->write_buf != NULL);
    targs->read_buf = malloc(g_args.io_size_max);
    assert(targs->read_buf != NULL);

    /* Initialize write buffer for the current stream */
    memset(targs->write_buf, rand_r(&targs->seed) % 256, g_args.io_size_max);

    /* Do IOs during predefined time period */
    while (time(NULL) - start_time < g_args.runtime_s)
        mrw_do_io_burst(targs);

    /* End by reading or writing last file chunk */
    if (g_args.is_last_chunk)
    {
        size_t size = rand_r(&targs->seed) % g_args.io_size_max;
        off_t offset = g_args.file_size - 1 - size;

        if (g_args.verbosity_lvl)
            printf("Thread #%u \t- Reading last %lu bytes.\n",
                   targs->thread_id, size);

        if (g_args.io_type == MRW_WRITE)
            mrw_write(targs, size, offset);
        else
            mrw_read(targs, size, offset);
    }

    if (g_args.is_multiple_fd)
        close(targs->fd);

    free(targs->read_buf);
    free(targs->write_buf);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    mrw_args_retrieve(argc, argv, &g_args);
    printf("MultiRW [seed: %u] %u threads to file '%s' during %us\n",
           g_args.first_seed, g_args.nb_threads,
           g_args.file_path, g_args.runtime_s);

    if (g_args.verbosity_lvl)
        printf("[file size: %lu, mmap: %u, iotype: %u, "
               "io size max: %u,\nio burst: %u, "
               "read last bytes: %u, use a FD per thread: %u]\n\n",
               g_args.file_size, g_args.is_mmap,
               g_args.io_type, g_args.io_size_max,
               g_args.io_burst_count, g_args.is_last_chunk,
               g_args.is_multiple_fd);

    mrw_file_init();

    thread_args_t *targs = malloc(sizeof(thread_args_t) * g_args.nb_threads);

    /* Open destination file only once for the whole process */
    if (!g_args.is_multiple_fd)
        io_stream_open(&targs[0]);

    /* Prepare thread arguments and spawn each thread */
    for (uint32_t i = 0; i < g_args.nb_threads; i++)
    {
        targs[i].fd = targs[0].fd;
        targs[i].fd_mmap = targs[0].fd_mmap;
        targs[i].thread_id = i;
        pthread_create(&targs[i].thread, NULL, io_stream, &targs[i]);
    }

    for (uint32_t i = 0; i < g_args.nb_threads; i++)
        pthread_join(targs[i].thread, NULL);

    /* Close file */
    if (!g_args.is_multiple_fd)
        close(targs[0].fd);

    free(targs);
    return 0;
}
