#include "cryptorand.h"

void cryptorand_init(cryptorand_t state) {
  cryptorand_initseed(state, DEFAULT_SEED, NULL);
}

void cryptorand_initseed(cryptorand_t state, const char *seed, const char *additional) {
  state->cryptorand_init = 1;
  state->ctr = 0;
  if(!(state->iv = (unsigned char*)malloc(EVP_CIPHER_iv_length(AES_ALGORITHM)))) {
    perror("Error setting IV for state");
    return;
  }
  memset(state->iv, 0, EVP_CIPHER_iv_length(AES_ALGORITHM));

  if(additional == NULL) {
    additional = "";
  }

  char *digest;
  if(!(digest = (char*)malloc(strlen(seed) + strlen(additional) + 1))) {
    perror("Error setting SHA digest");
    return;
  }

  digest[0] = 0;
  strcat(digest, seed);
  strcat(digest, additional);

   EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("Failed to create message digest context");
        free(digest);
        return;
    }

    // Initialize, update, and finalize SHA-256 digest
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(mdctx, digest, strlen(digest)) != 1 ||
        EVP_DigestFinal_ex(mdctx, state->key, NULL) != 1) {
        perror("SHA-256 digest computation failed");
        EVP_MD_CTX_free(mdctx);
        free(digest);
        return;
    }

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    free(digest);

}

void cryptorand_clear(cryptorand_t state) {
  free(state->iv);
}

void fmpz_randm_crypto(fmpz_t f, cryptorand_t state, const fmpz_t m) {
  mpz_t x, rop;
  mpz_init(x);
  mpz_init(rop);
  fmpz_get_mpz(x, m);
  mpz_urandomm_crypto(rop, state, x);
  fmpz_set_mpz(f, rop);
  mpz_clear(x);
  mpz_clear(rop);
}

void mpz_urandomm_crypto(mpz_t rop, cryptorand_t state, const mpz_t m) {
  unsigned long size = mpz_sizeinbase(m, 2);

  while(1) {
    mpz_urandomb_crypto(rop, state, size);
    if(mpz_cmp(rop, m) < 0) {
      break;
    }
  }
}

void mpz_urandomb_crypto(mpz_t rop, cryptorand_t state, mp_bitcnt_t n) {
  unsigned long ctr_iv;
  #pragma omp critical(update_iv_counter)
  {
    // update the internal counter, works at most 2^64 times
    ctr_iv = state->ctr++;
  }

  memcpy(state->iv, &ctr_iv, sizeof(ctr_iv));
  mp_bitcnt_t nb = n/8+1; // number of bytes

  if(!(state->ctx = EVP_CIPHER_CTX_new())) {
    perror("Error in initializing new cipher context");
    return;
  }
  if(!EVP_EncryptInit_ex(state->ctx, AES_ALGORITHM, NULL, state->key,
        state->iv)) {
    perror("Error in calling EncryptInit");
    return;
  }

  unsigned char *output;
  if(!(output =(unsigned char*) malloc(2 * (nb + EVP_MAX_IV_LENGTH)))) {
    perror("Error in initializing output buffer");
    return;
  }
  mp_bitcnt_t outlen = 0;

  int in_size = nb;
  unsigned char in[in_size];
  memset(in, 0, in_size);


  while(outlen < nb) {
    int buflen = 0;
    if(!EVP_EncryptUpdate(state->ctx, output+outlen, &buflen, in, in_size)) {
      perror("Error in calling EncryptUpdate");
      //goto output_exit;
    }
    outlen += buflen;
  }
  int final_len = 0;
  if(!EVP_EncryptFinal(state->ctx, output+outlen, &final_len)) {
    perror("Error in calling EncryptFinal");
    //goto output_exit;
  }
  outlen += final_len;

  if(outlen > nb) {
    outlen = nb; // we will only use nb bytes
  }

  mp_bitcnt_t true_len = outlen + 4;
  mp_bitcnt_t bytelen = outlen;

  unsigned char *buf;
  if(!(buf =(unsigned char*) malloc(true_len))) {
    perror("Error in initializing buf");
    goto output_exit;
  }
  memset(buf, 0, true_len);
  memcpy(buf+4, output, outlen);
  buf[4] >>= ((outlen*8) - (unsigned int) n);

  for(int i = 3; i >= 0; i--) {
    buf[i] = (unsigned char) (bytelen % (1 << 8));
    bytelen /= (1 << 8);
  }

  // generate a random n-bit number
  FILE *fp;
  if(!(fp = fmemopen(buf, true_len, "rb"))) {
    perror("Error in calling fmemopen");
    goto buf_exit;
  }

  if(!mpz_inp_raw(rop, fp)) {
    fprintf(stderr, "Error in parsing randomness.\n");
  }

  fclose(fp);

buf_exit:
  free(buf);

output_exit:
  free(output);

  EVP_CIPHER_CTX_cleanup(state->ctx);
  free(state->ctx);

}

