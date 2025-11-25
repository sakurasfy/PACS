#ifndef BGN_H_
#define BGN_H_
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>
#include <pbc/pbc.h>
#include <gmp.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>


struct mpz_pair {
    mpz_t first;
    mpz_t second;
    
    mpz_pair() {
        mpz_init(first);
        mpz_init(second);
    }
    
    ~mpz_pair() {
        mpz_clear(first);
        mpz_clear(second);
    }
};

class BGN {
private:
    gmp_randstate_t state;
public:
    
    element_t G1,P,Q;  // Group generators
    mpz_t q1, q2, n;    // Prime factors and modulus
    
public:
    pairing_t pairing;
    void cleanup();
    void encrypt(mpz_t m, element_t& c);  // Encrypt plaintext m to ciphertext c in G1
    void decrypt(element_t c, mpz_t& result);  // Decrypt G1 ciphertext to plaintext
    void decryptGT(element_t c, mpz_t& result);  // Decrypt GT ciphertext (from mul) to plaintext
    void findGenerator(element_t &P, element_t &G1, mpz_t q1, mpz_t q2, mpz_t n);
    bool pollard_rho_discrete_log(element_t g, element_t h, mpz_t order_q, mpz_t &result) ;
    void add(element_t c1, element_t c2, element_t& result);  // Homomorphic addition (G1 -> G1)
    void mul(element_t c1, element_t c2, element_t& result);  // Homomorphic multiplication (G1 x G1 -> GT)
    bool com(element_t c1, element_t c2);  // Compare two ciphertexts (original secure version)
    bool comSimple(element_t c1, element_t c2);  // Compare two ciphertexts (simple decrypt-and-compare version)
    BGN();
    ~BGN();
};
#endif
