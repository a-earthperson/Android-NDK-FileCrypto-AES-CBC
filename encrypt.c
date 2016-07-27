#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "encrypt.h"


/* AES key for Encryption and Decryption */
static const unsigned char aes_key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
static const char extension[] = {'.','g','z','.','a','e','s'};
int main(int argc, char const *argv[])
{

        const char *infile_name  = argv[1];
        int fd = open (argv[1], O_RDONLY);
        argc = 0;

        /* Get the size of the file. */
        struct stat s;
        int status = fstat(fd, &s);
        ssize_t size = s.st_size;

        /* MMAP file */
        unsigned char *f = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        const ssize_t fsize = size;
        unsigned char *aes_input = f;
        printf("READ    : [%s]        (size : %zu bytes)", infile_name, fsize);

        for(size = 0; *(infile_name + size) != '\0'; size++) continue;
        const ssize_t infile_nsize  = size;
        const ssize_t outfile_nsize = size + sizeof(extension);
        char outfile_name[outfile_nsize];
        for(size = 0; *(infile_name + size) != '\0'; size++) outfile_name[size] = *(infile_name + size);
        for(size; size <= outfile_nsize; size++) outfile_name[size] = extension[size - infile_nsize];

        const long bytesWritten = zlib_encrypt(outfile_name, aes_input, fsize, aes_key, 16, 0, Z_DEFAULT_COMPRESSION);
        //free(aes_input);
        return bytesWritten;
}
