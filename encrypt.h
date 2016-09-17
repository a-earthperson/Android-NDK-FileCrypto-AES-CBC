#define F_APPEND   0x01
#define F_COMPRESS 0x02
#define CHUNKSIZE  0x100000 // 1024K
#define W_BITS     15
#define GZIP_ENCODING 16
#define N_THREADS 12

#include "openssl/aes.h"
#include "zlib/zlib.h"
#include <pthread.h>

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
   IV_len  :: use given key_len, or if new key generated, set len here
 */

typedef struct {
 	ssize_t id;
  ssize_t start;
  ssize_t count;

  int gzip;
  ssize_t ZIPsize;
  ssize_t ZIP_allocated;
  unsigned char *ZIPout;
} params;

/* global vars needed by each thread */
static const unsigned char *RAW_in;

void *do_zlib_encrypt(void *args) {

  params *p = (params *) args;

  /* init buffer for saving compressed data */
  const ssize_t BLOCKSIZE = (CHUNKSIZE > p->count) ? p->count : CHUNKSIZE;
  p->ZIP_allocated = BLOCKSIZE;
  p->ZIPsize = 0;

  p->ZIPout = (unsigned char *) malloc(BLOCKSIZE * sizeof(unsigned char));
  if(p->ZIPout == NULL)
        exit(EXIT_FAILURE);

  /* Init zlib compression */
  z_stream ZIPstream;
  ZIPstream.zalloc = Z_NULL;
  ZIPstream.zfree  = Z_NULL;
  ZIPstream.opaque = Z_NULL;
  if (deflateInit2(&ZIPstream, p->gzip, Z_DEFLATED, W_BITS | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) < 0)
          exit (EXIT_FAILURE);

  ZIPstream.next_in   = (unsigned char *) (RAW_in + (p->start));
  ZIPstream.avail_in  = p->count;
  ZIPstream.avail_out = 0;

  // TODO :: what happens when BLOCKSIZE is too small ? CHUNCK CORRECTLY MAN
  while(ZIPstream.avail_out == 0)
  {
          ZIPstream.next_out  = p->ZIPout;
          ZIPstream.avail_out = BLOCKSIZE;

          if(deflate(&ZIPstream, Z_FINISH) < 0)
                  break;

          p->ZIPsize += (p->count - ZIPstream.avail_out);
          //if(bytesCompressed > ) increase bytesallocated
  }
  deflateEnd (&ZIPstream);
  //printf("\nID[%zu] START[%zu] COUNT[%zu] COMPLETE[%zu]", p->id, p->start, p->count, p->ZIPsize);
  pthread_exit(NULL);
}

long zlib_encrypt(const char *path, const unsigned char *RAWout, const ssize_t sizeRAW,
                  const unsigned char *aes_key, const ssize_t key_len, const int flags, const int compressLevel) {

        /** SETUP FOR P_THREADS **/
        const ssize_t numCPU = N_THREADS;//sysconf(_SC_NPROCESSORS_ONLN);
        const ssize_t offset = sizeRAW % numCPU;
        const ssize_t count  = sizeRAW / numCPU;
        printf("\nnumCores: %zu", numCPU);
        RAW_in = RAWout;
        ssize_t i;

        params *IDs = (params *) calloc(numCPU, sizeof(params));
        if(IDs == NULL) return -1;
        pthread_t threads[numCPU];


        /* Start up thread */
      	for (i = 0; i < numCPU; i++)
      	{
      		IDs[i].id    = i;
          IDs[i].gzip  = compressLevel;
          IDs[i].count = count + ((i == 0) ? offset : 0);
          IDs[i].start = (i == 0) ? 0 : ((i * IDs[i].count) + offset + 1);
      		if(pthread_create(&threads[i], NULL, do_zlib_encrypt, (void *)(IDs + i)))
                  return -1; // TODO :: need dynamic solution to reallocate workloads if the thread dies
      	}

      	for (i = 0; i < numCPU; i++)
      		pthread_join(threads[i], NULL);

        /* Open file */
        const char *o_flag = (F_APPEND && flags) ? "a+" : "w";
        FILE *fp = fopen(path, o_flag);
        if(fp == NULL)
            return -1;

        /* now write the data from each thread to disk */
        ssize_t bytesWritten = 0;
        for (i = 0; i < numCPU; i++)
        {
          bytesWritten   += fwrite(IDs[i].ZIPout, 1, IDs[i].ZIPsize, fp);
        }

      	free(IDs);

        const float compressionRatio = 100 * (float) bytesWritten / (float) sizeRAW;
        printf("\nSUCCESS : [%s] (size : %zu bytes)", path, bytesWritten);
        printf("\n\t Ratio     : %0.2f%%", compressionRatio);
        printf("\n\t ZIP_bytes : %zu", bytesWritten);
        printf("\n\t RAW_bytes : %zu\n", sizeRAW);

        return (long) bytesWritten;





        //
        // /* Init IV & AES keys */
        // ssize_t i = 0;
        // srand(time(NULL));
        // unsigned char IV[AES_BLOCK_SIZE];
        // for(; i < AES_BLOCK_SIZE; i++)
        //   memset(IV+i, rand(), 1);
        //
        // AES_KEY enc_key;
        // private_AES_set_encrypt_key(aes_key, 8*key_len, &enc_key);
        // GZ_AESout = (unsigned char *) calloc(sizeRAW, sizeof(unsigned char)); // TODO :: back to
        // if(GZ_AESout == NULL)
        //         return -1;
        //
        //
        // const int fileCorrupt = bytesEncrypted != bytesWritten;
        // const int compressFailed = ZIPstream.avail_in != 0 && ZIPstream.avail_out == 0;
        //
        // if(compressFailed || fileCorrupt || bytesWritten == 0) {
        //         printf("\n ERROR : bytesEncrypted(%zu) != bytesWritten(%zu) != totalBytesCompressed(%zu)", bytesEncrypted, bytesWritten, totalBytesCompressed);
        //         remove(path);
        //         return -1;
        // }
        // else {
        //         const float compressionRatio = 100 * (float) totalBytesCompressed / (float) sizeRAW;
        //         printf("\nSUCCESS : [%s] (size : %zu bytes)", path, bytesWritten);
        //         printf("\n\t Ratio     : %0.2f%%", compressionRatio);
        //         printf("\n\t ZIP_bytes : %zu", totalBytesCompressed);
        //         printf("\n\t AES_bytes : %zu", bytesEncrypted);
        //         printf("\n\t RAW_bytes : %zu\n", sizeRAW);
        // }
        // return (long) bytesWritten;
}

// const ssize_t enc_buf_len = CHUNKSIZE > sizeRAW ? sizeRAW : CHUNKSIZE;

/* Init zlib compression */
//unsigned char *ZIPout = (unsigned char *)calloc(CHUNKSIZE, sizeof(unsigned char));
// z_stream ZIPstream;
// ZIPstream.zalloc = Z_NULL;
// ZIPstream.zfree  = Z_NULL;
// ZIPstream.opaque = Z_NULL;
// if (deflateInit2(&ZIPstream, lvl, Z_DEFLATED, W_BITS | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) < 0)
//         exit (EXIT_FAILURE);
//
// ssize_t bytesCompressed = 0, totalBytesCompressed = 0, bytesEncrypted = 0, bytesWritten = 0;
// ZIPstream.next_in  = (unsigned char *)RAWout;
// ZIPstream.avail_in = sizeRAW;
// ZIPstream.avail_out = 0;
//
// while(ZIPstream.avail_out == 0) {
//
//         ZIPstream.avail_out = CHUNKSIZE;
//         ZIPstream.next_out  = GZ_AESout;//ZIPout;
//
//         if(deflate(&ZIPstream, Z_FINISH) < 0)
//                 break;
//
//         bytesCompressed = CHUNKSIZE - ZIPstream.avail_out;
//         AES_cbc_encrypt(GZ_AESout, GZ_AESout, bytesCompressed, &enc_key, IV, AES_ENCRYPT); //TODO :; back to ZIPout
//         bytesWritten   += fwrite(GZ_AESout, 1, bytesCompressed, fp);
//         totalBytesCompressed += bytesCompressed;
//         bytesEncrypted += bytesCompressed;
// }
// deflateEnd (&ZIPstream);
//
// fclose(fp);
// free(GZ_AESout);
// //free(ZIPout);
//












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
