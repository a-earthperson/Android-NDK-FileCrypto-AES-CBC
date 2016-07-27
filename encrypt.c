#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include "encrypt.h"
#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>


/* AES key for Encryption and Decryption */
static const unsigned char aes_key[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

int main(int argc, char const *argv[])
{

        const char * file_name = argv[1];
        int fd = open (argv[1], O_RDONLY);

        /* Get the size of the file. */
        struct stat s;
        int status = fstat(fd, &s);
        int size = s.st_size;

        unsigned char *f = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        const ssize_t fsize = size;
        unsigned char *aes_input = f;
        printf("READ : [%s] (size : %zu bytes)", file_name, fsize);
        /* Input data to encrypt */
      //  unsigned char *aes_input = (unsigned char*)calloc(fsize, sizeof(unsigned char));//={0x0,0x1,0x2,0x3,0x4,0x5,0x0,0x1,0x2,0x3,0x4,0x5};
        // srand(time(NULL));
        // ssize_t i = 0;
        // for(;i<fsize;i=i+32)
        //   memset(aes_input+i, rand(), 32);
        /* Init vector */
        unsigned char iv[AES_BLOCK_SIZE];
        memset(iv, 0x00, AES_BLOCK_SIZE);

        // /* Buffers for Encryption and Decryption */
        // unsigned char enc_out[sizeof(aes_input)];
        // unsigned char dec_out[sizeof(aes_input)];
        //
        // /* AES-128 bit CBC Encryption */
        // AES_KEY enc_key, dec_key;
        // private_AES_set_encrypt_key(aes_key, sizeof(aes_key)*8, &enc_key);
        // AES_cbc_encrypt(aes_input, enc_out, sizeof(aes_input), &enc_key, iv, AES_ENCRYPT);
        //
        // /* AES-128 bit CBC Decryption */
        // memset(iv, 0x00, AES_BLOCK_SIZE); // don't forget to set iv vector again, else you can't decrypt data properly
        // private_AES_set_decrypt_key(aes_key, sizeof(aes_key)*8, &dec_key); // Size of key is in bits
        // AES_cbc_encrypt(enc_out, dec_out, sizeof(aes_input), &dec_key, iv, AES_DECRYPT);

        /* Printing and Verifying */
        //print_data("\n Original ",aes_input, fsize); // you can not print data as a string, because after Encryption its not ASCII
        //print_data("\n Encrypted",enc_out, sizeof(enc_out));
        //print_data("\n Decrypted",dec_out, sizeof(dec_out));

        const long bytesWritten = zlib_encrypt("gyro.raw.gz.aes", aes_input, fsize, aes_key, 16, 0, 9);
        //free(aes_input);
        return bytesWritten;
}
