/**
* MultiRW: A multi-threaded program to stress a file system with parallel IOs
*          in a single shared file.
* args.c : Parse program arguments
*
* URL       https://github.com/jyvet/multirw
* License   MIT
* Author    Jean-Yves VET <contact[at]jean-yves.vet>
* Copyright (c) 2021
******************************************************************************/

#ifndef MULTIRW_ARGS_H
#define MULTIRW_ARGS_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define MULTIRW_VERSION      "0.1"
#define MULTIRW_CONTACT      "contact[at]jean-yves.vet"


#define BUF_SIZE_MAX          524288  /* 512KB */
#define IO_SIZE_MAX           524288  /* 512KB */
#define FILE_SIZE_DEFAULT    (512 * 1024 * 1024 + 1) /* 512MB + one byte */
#define NB_THREADS_DEFAULT    10
#define LAST_CHUNK_DEFAULT    true
#define MULTIPLE_FD_DEFAULT   false
#define CACHE_BYPASS_DEFAULT  false
#define MMAP_DEFAULT          false
#define RUNTIME_SEC_DEFAULT   10 /* 10 seconds */
#define IO_BURST_COUNT       (64 * 1024)

typedef enum
{
    MRW_READ = 0,
    MRW_WRITE,
    MRW_RW
} io_type_t;

typedef struct mrw_args
{
    char      file_path[PATH_MAX];
    uint64_t  file_size;
    uint32_t  nb_threads;
    bool      is_mmap;
    bool      is_last_chunk;
    bool      is_multiple_fd;
    bool      is_cache_bypass;
    io_type_t io_type;
    uint32_t  runtime_s;
    uint32_t  first_seed;
    uint32_t  io_size_max;
    uint32_t  io_burst_count;
    uint8_t   verbosity_lvl;
} mrw_args_t;

void mrw_args_retrieve(int argc, char **argv, mrw_args_t *out_args);

#endif /* MULTIRW_ARGS_H */

