#include "BGN.h"

void BGN::cleanup() {
    element_clear(G1);
    element_clear(P);
    element_clear(Q);
    mpz_clear(q1);
    mpz_clear(q2);
    mpz_clear(n);
    pairing_clear(pairing);
    gmp_randclear(state);
}

void BGN::encrypt(mpz_t m, element_t& c) {
    element_t temp1, temp2;
    element_init_same_as(temp1, P);
    element_init_same_as(temp2, Q);
    element_init_same_as(c, G1);

    mpz_t r;
    mpz_init(r);
    mpz_urandomm(r, state, n);

    element_pow_mpz(temp1, P, m);
    element_pow_mpz(temp2, Q, r);
    element_mul(c, temp1, temp2);

    mpz_clear(r);
    element_clear(temp1);
    element_clear(temp2);
}

void BGN::decrypt(element_t c, mpz_t& result) {
    element_t gsk, csk;
    element_init_same_as(gsk, P);
    element_init_same_as(csk, c);

    element_pow_mpz(csk, c, q1);
    element_pow_mpz(gsk, P, q1);

    mpz_set_ui(result, 0);

    element_t temp;
    element_init_same_as(temp, gsk);
    mpz_t i;
    mpz_init(i);

    for (mpz_set_ui(i, 0); mpz_cmp(i, q1) < 0; mpz_add_ui(i, i, 1)) {
        element_pow_mpz(temp, gsk, i);
        if (element_cmp(temp, csk) == 0) {
            mpz_set(result, i);
            break;
        }
    }

    element_clear(temp);
    mpz_clear(i);
    element_clear(gsk);
    element_clear(csk);
}

void BGN::decryptGT(element_t c, mpz_t& result) {
    element_t csk, gsk, base;
    element_init_GT(csk, pairing);
    element_init_GT(gsk, pairing);
    element_init_GT(base, pairing);

    pairing_apply(base, P, P, pairing);
    element_pow_mpz(csk, c, q1);
    element_pow_mpz(gsk, base, q1);

    if (element_is1(csk)) {
        mpz_set_ui(result, 0);
        element_clear(gsk);
        element_clear(csk);
        element_clear(base);
        return;
    }

    mpz_set_ui(result, 0);
    element_t temp;
    element_init_GT(temp, pairing);
    element_set1(temp);

    mpz_t i, max_search;
    mpz_init(i);
    mpz_init(max_search);
    mpz_set_ui(max_search, 2000);// Limit search to 2000

    bool found = false;
    if (element_cmp(temp, csk) == 0) {
        mpz_set_ui(result, 0);
        found = true;
    } else {
        for (mpz_set_ui(i, 1); mpz_cmp(i, max_search) < 0; mpz_add_ui(i, i, 1)) {
            element_mul(temp, temp, gsk);
            if (element_cmp(temp, csk) == 0) {
                mpz_set(result, i);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        std::cerr << "Warning: GT decryption failed" << std::endl;
    }

    element_clear(temp);
    mpz_clear(i);
    mpz_clear(max_search);
    element_clear(gsk);
    element_clear(csk);
    element_clear(base);
}


void BGN::findGenerator(element_t &P, element_t &G1, mpz_t q1, mpz_t q2, mpz_t n) {
    element_t temp;
    element_init_same_as(temp, G1);

    while (true) {
        element_random(P);

        if (element_is1(P)) {
            continue;
        }

        element_pow_mpz(temp, P, q1);
        if (element_is1(temp)) {
            continue;
        }

        element_pow_mpz(temp, P, q2);
        if (element_is1(temp)) {
            continue;
        }

        element_pow_mpz(temp, P, n);
        if (element_is1(temp)) {
            break;
        }
    }

    element_clear(temp);
}

BGN::BGN() {
    gmp_randinit_default(state);
    mpz_t seed;
    mpz_init(seed);
    mpz_set_ui(seed, static_cast<unsigned long>(time(nullptr)));
    gmp_randseed(state, seed);
    mpz_clear(seed);

    mpz_init(q1);
    mpz_init(q2);
    mpz_init(n);
    mpz_set_str(q1, "65521", 10);
    mpz_set_str(q2, "65537", 10);
    mpz_mul(n, q1, q2);

    pbc_param_t params;
    pbc_param_init_a1_gen(params, n);
    pairing_init_pbc_param(pairing, params);
    pbc_param_clear(params);

    element_init_G1(G1, pairing);
    element_init_G1(P, pairing);
    element_init_G1(Q, pairing);

    findGenerator(P, G1, q1, q2, n);
    element_pow_mpz(Q, P, q2);
}
void BGN::add(element_t c1, element_t c2, element_t& result) {
    element_init_same_as(result, G1);
    element_mul(result, c1, c2);
}

void BGN::mul(element_t c1, element_t c2, element_t& result) {
    element_init_GT(result, pairing);
    pairing_apply(result, c1, c2, pairing);
}

bool BGN::com(element_t c1, element_t c2) {//simpled version
    element_t c1_prime, c2_prime;
    element_init_same_as(c1_prime, c1);
    element_init_same_as(c2_prime, c2);
    element_set(c1_prime, c1);
    element_set(c2_prime, c2);

    mpz_t b, r;
    mpz_init(b);
    mpz_init(r);

    mpz_urandomm(b, state, n);
    mpz_mod_ui(b, b, 2);

    mpz_t max_r;
    mpz_init(max_r);
    mpz_set_ui(max_r, 100);
    mpz_urandomm(r, state, max_r);
    if (mpz_cmp_ui(r, 0) == 0) {
        mpz_set_ui(r, 1);
    }
    mpz_clear(max_r);

    element_t l, temp, c2_inv;
    element_init_same_as(l, G1);
    element_init_same_as(temp, G1);
    element_init_same_as(c2_inv, G1);

    element_invert(c2_inv, c2_prime);

    if (mpz_cmp_ui(b, 1) == 0) {
        element_mul(temp, c1_prime, c2_inv);
        element_pow_mpz(l, temp, r);
    } else {
        element_t c1_inv;
        element_init_same_as(c1_inv, G1);
        element_invert(c1_inv, c1_prime);
        element_mul(temp, c1_inv, c2_prime);
        element_pow_mpz(l, temp, r);
        element_clear(c1_inv);
    }

    mpz_t decrypted_value;
    mpz_init(decrypted_value);
    decrypt(l, decrypted_value);

    mpz_t half_q1;
    mpz_init(half_q1);
    mpz_fdiv_q_ui(half_q1, q1, 2);

    int f_prime = (mpz_cmp(decrypted_value, half_q1) > 0) ? 1 : 0;
    int f = (mpz_cmp_ui(b, 1) == 0) ? f_prime : (1 - f_prime);
    bool result = (f == 0);

    mpz_clear(b);
    mpz_clear(r);
    mpz_clear(decrypted_value);
    mpz_clear(half_q1);
    element_clear(c1_prime);
    element_clear(c2_prime);
    element_clear(l);
    element_clear(temp);
    element_clear(c2_inv);

    return result;
}

// Simple comparison: decrypt both ciphertexts and compare
bool BGN::comSimple(element_t c1, element_t c2) {
    mpz_t m1, m2;
    mpz_init(m1);
    mpz_init(m2);

    // Decrypt both ciphertexts
    decrypt(c1, m1);
    decrypt(c2, m2);

    // Compare the decrypted values: c1 >= c2
    bool result = (mpz_cmp(m1, m2) >= 0);

    mpz_clear(m1);
    mpz_clear(m2);

    return result;
}


BGN::~BGN() {
    cleanup();
    
}
/*int main() {
        std::cout << "=== BGN Test ===" << std::endl << std::endl;

        BGN bgn;

        mpz_t m1, m2;
        mpz_init_set_ui(m1, 42);
        mpz_init_set_ui(m2, 32);

        gmp_printf("m1=%Zd, m2=%Zd\n\n", m1, m2);

        element_t c1, c2;
        bgn.encrypt(m1, c1);
        bgn.encrypt(m2, c2);

        mpz_t decrypted;
        mpz_init(decrypted);

        element_t add_result;
        bgn.add(c1, c2, add_result);
        bgn.decrypt(add_result, decrypted);
        gmp_printf("Addition Result: %Zd\n\n", decrypted);

        element_t mul_result;
        bgn.mul(c1, c2, mul_result);
        bgn.decryptGT(mul_result, decrypted);
        gmp_printf("Multiplication Result: %Zd\n\n", decrypted);


        if(bgn.com(c1, c2)){
            gmp_printf("Comparison Result: m1 >= m2 (%Zd >= %Zd)\n", m1, m2);
        } else {
            gmp_printf("Comparison Result: m1 < m2 (%Zd < %Zd)\n", m1, m2);
        }


        mpz_clear(m1);
        mpz_clear(m2);
        mpz_clear(decrypted);
        element_clear(c1);
        element_clear(c2);
        element_clear(add_result);
        element_clear(mul_result);

    return 0;
}*/
