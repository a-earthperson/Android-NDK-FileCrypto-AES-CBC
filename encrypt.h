#define F_APPEND 0x01
#define F_COMPRESS 0x02
#define CHUNKSIZE 1024*1024 // 1024K
#define W_BITS 15
#define GZIP_ENCODING 16

#include "openssl/aes.h"
#include "zlib/zlib.h"

void print_data(const char *tittle, const void* data, int len)
{
        const unsigned char * p = (const unsigned char*) data;
        int i = 0;

        printf("%s : ",tittle);
        for (; i<len; ++i)
                printf("%02X ", *p++);
        printf("\n");
}

/* The following macro calls a zlib routine and checks the return
   value. If the return value ("status") is not OK, it prints an error
   message and exits the program. Zlib's error statuses are all less
   than zero. */

/*
   returns :: number of bytes actually written to disk
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


    /* Init IV & AES keys */ //TODO :: set IV to non-zero value.
    AES_KEY enc_key;
    unsigned char IV[AES_BLOCK_SIZE];
    memset(IV, 0x00, AES_BLOCK_SIZE);
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
      {
        //printf("\nBreaking");
        break;
      }

      bytesCompressed = CHUNKSIZE - ZIPstream.avail_out;

      AES_cbc_encrypt(ZIPout, AESout, bytesCompressed, &enc_key, IV, AES_ENCRYPT);
      bytesWritten   += fwrite(AESout, 1, bytesCompressed, fp);
      totalBytesCompressed += bytesCompressed;
      bytesEncrypted += bytesCompressed;


      printf("\n avail_in[%u], avail_out[%u], bytes[%zu], total[%zu], val[%02X]", ZIPstream.avail_in, ZIPstream.avail_out, bytesCompressed, totalBytesCompressed, *ZIPout);
      // for (; i<len; ++i)
      //         printf("%02X ", *p++);
      // print_data("\ncompressed :",ZIPout, bytesCompressed);
      // printf("\n avail_in : %u, avail_out : %u", ZIPstream.avail_in, ZIPstream.avail_out);
      // printf("\n bytesCompressed: %zu, total: %zu",bytesCompressed, totalBytesCompressed);
    }
    deflateEnd (&ZIPstream);

    // totalBytesCompressed -= bytesCompressed;
    // bytesEncrypted -= bytesCompressed;
    // bytesWritten -= bytesCompressed;

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
      const float compressionRatio = 100 * totalBytesCompressed / sizeRAW;
      printf("\n SUCCESS");
      printf("\n\t Ratio     : %0.02f%%", compressionRatio);
      printf("\n\t ZIP_bytes : %zu", totalBytesCompressed);
      printf("\n\t AES_bytes : %zu", bytesEncrypted);
      printf("\n\t RAW_bytes : %zu", sizeRAW);
    }

    return (long) bytesWritten;
}

// while (bytesEncrypted < sizeRAW)
// {
//     AES_cbc_encrypt((RAWout+bytesEncrypted), AESout, enc_buf_len, &enc_key, IV, AES_ENCRYPT);
//     bytesWritten   += fwrite(AESout, 1, enc_buf_len, fp);
//     bytesEncrypted += enc_buf_len;
//     //printf("\n [%zu,%zu] ", bytesEncrypted, bytesWritten);
//     //print_data("encrypting :", AESout, enc_buf_len);
// }
// fclose(fp);
// free(AESout);
//
// const int fileCorrupt = (bytesEncrypted != bytesWritten);
//
// if(fileCorrupt) {
//   printf("\n ERROR : bytesEncrypted(%zu) != bytesWritten(%zu)", bytesEncrypted, bytesWritten);
//   remove(path);
//   return -1;
// // }
// ZIPstream.avail_out = 0;
// printf("avail_out %u", ZIPstream.avail_out);
// do {
//
//   ZIPstream.avail_out = CHUNKSIZE;
//   ZIPstream.next_out  = ZIPout;
//
//   if(deflate(&ZIPstream, Z_FINISH) < 0)
//   {
//     //printf("\nBreaking");
//     break;
//   }
//
//   bytesCompressed = CHUNKSIZE - ZIPstream.avail_out;
//   totalBytesCompressed += bytesCompressed;
//   //fwrite (ZIPout, sizeof (unsigned char), bytesCompressed, stdout);
//   AES_cbc_encrypt(ZIPout, AESout, bytesCompressed, &enc_key, IV, AES_ENCRYPT);
//   bytesWritten   += fwrite(AESout, 1, bytesCompressed, fp);
//   bytesEncrypted += bytesCompressed;
//
//   // print_data("\ncompressed :",ZIPout, bytesCompressed);
//   // printf("\n avail_in : %u, avail_out : %u", ZIPstream.avail_in, ZIPstream.avail_out);
//   // printf("\n bytesCompressed: %zu, total: %zu",bytesCompressed, totalBytesCompressed);
// } while (ZIPstream.avail_out == 0);
// deflateEnd (&ZIPstream);
