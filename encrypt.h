#define F_APPEND   0x01
#define F_COMPRESS 0x02
#define CHUNKSIZE  0x100000 // 1024K
#define W_BITS     15
#define GZIP_ENCODING 16

#include "openssl/aes.h"
#include "zlib/zlib.h"

/*
   returns :: struct typedef AES_key + IV + thier lenghts // TODO
   path    :: fopen file with a+ options, if unable to open return EXIT_FAILURE
   RAWout  :: constant byte array (to be written to disk)
   len     :: read 'len' bytes.
   AES_key :: use given encryption key if newkey flag unset. else create a new
              key & store it here.
   key_len :: use given key_len, or if new key generated, set len here.
   IV_key  :: use given encryption key if newkey flag unset. else create a new
              key & store it here.
   IV_len  :: use given key_len, or if new key generated, set len here.
 */

long zlib_encrypt(const char *path, const unsigned char *RAWout, const ssize_t sizeRAW,
                  const unsigned char *aes_key, const ssize_t key_len, const int flags, const int lvl) {

        /* Open file */
        const char *o_flag = (F_APPEND && flags) ? "a+" : "w";
        FILE *fp = fopen(path, o_flag);
        if(fp == NULL)
                return -1;

        /* Init zlib compression */
        unsigned char *ZIPout = (unsigned char *)calloc(CHUNKSIZE, sizeof(unsigned char));
        z_stream ZIPstream;
        ZIPstream.zalloc = Z_NULL;
        ZIPstream.zfree  = Z_NULL;
        ZIPstream.opaque = Z_NULL;
        if (deflateInit2(&ZIPstream, lvl, Z_DEFLATED, W_BITS | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) < 0)
                exit (EXIT_FAILURE);


        /* Init IV & AES keys */

        ssize_t i = 0;
        srand(time(NULL));
        unsigned char IV[AES_BLOCK_SIZE];
        for(; i < AES_BLOCK_SIZE; i++)
                memset(IV+i, rand(), 1);

        AES_KEY enc_key;
        private_AES_set_encrypt_key(aes_key, 8*key_len, &enc_key);
        const ssize_t enc_buf_len = CHUNKSIZE > sizeRAW ? sizeRAW : CHUNKSIZE;
        unsigned char *AESout = (unsigned char *)calloc(CHUNKSIZE, sizeof(unsigned char));

        ssize_t bytesCompressed = 0, totalBytesCompressed = 0, bytesEncrypted = 0, bytesWritten = 0;
        ZIPstream.next_in  = (unsigned char *)RAWout;
        ZIPstream.avail_in = sizeRAW;
        ZIPstream.avail_out = 0;

        while(ZIPstream.avail_out == 0) {

                ZIPstream.avail_out = CHUNKSIZE;
                ZIPstream.next_out  = ZIPout;

                if(deflate(&ZIPstream, Z_FINISH) < 0)
                        break;

                bytesCompressed = CHUNKSIZE - ZIPstream.avail_out;
                AES_cbc_encrypt(ZIPout, AESout, bytesCompressed, &enc_key, IV, AES_ENCRYPT);
                bytesWritten   += fwrite(AESout, 1, bytesCompressed, fp);
                totalBytesCompressed += bytesCompressed;
                bytesEncrypted += bytesCompressed;
        }
        deflateEnd (&ZIPstream);

        fclose(fp);
        free(AESout);
        free(ZIPout);

        const int fileCorrupt = bytesEncrypted != bytesWritten;
        const int compressFailed = ZIPstream.avail_in != 0 && ZIPstream.avail_out == 0;

        if(compressFailed || fileCorrupt || bytesWritten == 0) {
                printf("\n ERROR : bytesEncrypted(%zu) != bytesWritten(%zu) != totalBytesCompressed(%zu)", bytesEncrypted, bytesWritten, totalBytesCompressed);
                remove(path);
                return -1;
        }
        else {
                const float compressionRatio = 100 * (float) totalBytesCompressed / (float) sizeRAW;
                printf("\nSUCCESS : [%s] (size : %zu bytes)", path, bytesWritten);
                printf("\n\t Ratio     : %0.2f%%", compressionRatio);
                printf("\n\t ZIP_bytes : %zu", totalBytesCompressed);
                printf("\n\t AES_bytes : %zu", bytesEncrypted);
                printf("\n\t RAW_bytes : %zu\n", sizeRAW);
        }
        return (long) bytesWritten;
}

//printf("\n avail_in[%u], avail_out[%u], bytes[%zu], total[%zu], val[%02X]", ZIPstream.avail_in, ZIPstream.avail_out, bytesCompressed, totalBytesCompressed, *ZIPout);
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

/* Input data to encrypt */
//  unsigned char *aes_input = (unsigned char*)calloc(fsize, sizeof(unsigned char));//={0x0,0x1,0x2,0x3,0x4,0x5,0x0,0x1,0x2,0x3,0x4,0x5};
// srand(time(NULL));
// ssize_t i = 0;
// for(;i<fsize;i=i+32)
//   memset(aes_input+i, rand(), 32);
