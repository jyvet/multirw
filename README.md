MultiRW
=======

This application spawns several threads to create multiple IO streams accessing
a single file. It is designed to stress a file system with random parallel IOs
to a single shared file.


How to build MultiRW
--------------------

    % make


How to run MultiRW
------------------

    % ./multirw [OPTION...] <file>


* **file:** output file on the file system to stress


Options are :

    -b, --bypass-cache=<value> Use O_DIRECT flag <0=disabled|1=enabled>
                               [default: 0]
    -d, --duration=<seconds>   Time period the program should run [default: 10s]
    -f, --file-size=<bytes>    File size [default: 512 MB + 1 byte]
    -F, --multiple-fd=<value>  Open a file descriptor per thread
                               <0=disabled,1=enabled> [default: 0]
    -i, --io-size-max=<bytes>  IO size max [default: 524288 bytes]
    -l, --last-chunk=<bool>    Last IO should access last file chunk
                               <0=disabled,1=enabled> [default: 1]
    -m, --mmap=<value>         Enable memory mapped file <0=disabled|1=enabled>
                               [default: 0]
    -p, --pattern=<value>      IO pattern <0=read|1=write|2=rw> [default: 2]
    -s, --seed=<value>         Initial seed [default: seed=PID]
    -t, --threads=<value>      Amount of threads [default: 10]
    -v, --verbose              Enable verbose mode
    -?, --help                 Give this help list
        --usage                Give a short usage message
    -V, --version              Print program version


Examples
--------

Use 50 threads in write only mode:

    % ./multirw -t 50 -p 1 /myfs/output_file.data


Use 100 threads, memory map the file and use O_DIRECT flag:

    % ./multirw -t 100 -m 1 -b 1  /myfs/output_file.data


Use 20 threads and make the output file 1GB:

    % ./multirw -t 20 -f $((1024**3)) /myfs/output_file.data
