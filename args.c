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

#include <argp.h>
#include "args.h"

/* Expand macro values to string */
#define STR_VALUE(var)   #var
#define STR(var)         STR_VALUE(var)

const char *argp_program_version = "MultiRW "MULTIRW_VERSION;
const char *argp_program_bug_address = "<"MULTIRW_CONTACT">";

/* Program documentation */
static char doc[] = "This application spawns several threads to create "
    "multiple IO streams accessing a single file. It is designed to "
    "stress a file system with random parallel IOs to a single shared file. "
    "It accepts the following optional arguments:";

/* A description of the arguments we accept */
static char args_doc[] = "<file>";

/* Options */
static struct argp_option options[] =
{
    {"bypass-cache", 'b', "<value>",    0, "Use O_DIRECT flag <0=disabled|1=enabled>\n[default: "
                                            STR(CACHE_BYPASS_DEFAULT) "]"},
    {"duration",     'd', "<seconds>",  0, "Time period the program should run [default: "
                                            STR(RUNTIME_SEC_DEFAULT) "s]"},
    {"threads",      't', "<value>",    0, "Amount of threads [default: "
                                            STR(NB_THREADS_DEFAULT) "]"},
    {"mmap",         'm', "<value>",    0, "Enable memory mapped file <0=disabled|1=enabled> "
                                           "[default: " STR(MMAP_DEFAULT) "]"},
    {"pattern",      'p', "<value>",    0, "IO pattern <0=read|1=write|2=rw> [default: 2]"},
    {"seed",         's', "<value>",    0, "Initial seed [default: seed=PID]"},
    {"verbose",      'v', 0,            0, "Enable verbose mode"},
    {"io-size-max",  'i', "<bytes>",    0, "IO size max [default: " STR(IO_SIZE_MAX) " bytes]"},
    {"multiple-fd",  'F', "<value>",    0, "Open a file descriptor per thread "
                                           "<0=disabled,1=enabled> [default: "
                                            STR(MULTIPLE_FD_DEFAULT) "]"},
    {"file-size",    'f', "<bytes>",    0, "File size [default: 512 MB + 1 byte]"},
    {"last-chunk",   'l', "<bool>",     0, "Last IO should access last file chunk "
                                           "<0=disabled,1=enabled> [default: "
                                            STR(LAST_CHUNK_DEFAULT) "]"},
    { 0 }
};

/* Parse a single option */
static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    mrw_args_t *args = state->input;

    switch (key)
    {
        case 'b':
            args->is_cache_bypass = atoi(arg);
            break;
        case 'd':
            args->runtime_s = atoi(arg);
            break;
        case 't':
            args->nb_threads = atoi(arg);
            break;
        case 'm':
            args->is_mmap = atoi(arg);
            break;
        case 'p':
            args->io_type = atoi(arg);
            break;
        case 'l':
            args->is_last_chunk = atoi(arg);
            break;
        case 's':
            args->first_seed = atol(arg);
            break;
        case 'v':
            args->verbosity_lvl = 1;
            break;
        case 'i':
            args->io_size_max = atol(arg);
            break;
        case 'f':
            args->file_size = atol(arg);
            break;
        case 'F':
            args->is_multiple_fd = atoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0)
                strcpy(args->file_path, arg);
            break;
        case ARGP_KEY_END:
            if (state->arg_num != 1)
            {
                argp_usage (state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/* Argp parser */
static struct argp argp = { options, parse_opt, args_doc, doc };

/**
 * Parse arguments
 *
 * @param   argc[in]        Amount of arguments
 * @param   argv[in]        Array of arguments
 * @param   out_args[out]   Parsed arguments
 */
void
mrw_args_retrieve(int argc, char *argv[], mrw_args_t *out_args)
{
    /* Set defaults */
    out_args->file_size       = FILE_SIZE_DEFAULT;
    out_args->nb_threads      = NB_THREADS_DEFAULT;
    out_args->is_mmap         = MMAP_DEFAULT;
    out_args->io_type         = MRW_RW;
    out_args->runtime_s       = RUNTIME_SEC_DEFAULT;
    out_args->first_seed      = getpid();
    out_args->io_size_max     = IO_SIZE_MAX;
    out_args->io_burst_count  = IO_BURST_COUNT;
    out_args->verbosity_lvl   = 0;
    out_args->is_last_chunk   = LAST_CHUNK_DEFAULT;
    out_args->is_multiple_fd  = MULTIPLE_FD_DEFAULT;
    out_args->is_cache_bypass = CACHE_BYPASS_DEFAULT;

    argp_parse (&argp, argc, argv, 0, 0, out_args);
}
