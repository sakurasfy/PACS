#include "aipe.h"
#include <fmpz_mat.h>
#include <iostream>

using namespace std;

void testAIPE() {

    long rowsP = 1;
    long colsP = 5;
    long rowsQ = 1;

    cout << "--- AIPE Test ---" << endl;
    cout << "dimensions: " << rowsP << " x " << colsP << endl << endl;

    fmpz_mat_t P, Q, EncP, EncQ;
    fmpz_mat_init(P, rowsP, colsP);
    fmpz_mat_init(Q, rowsQ, colsP);


    flint_rand_t state;
    flint_randinit(state);

    cout << "P: ";
    for (long i = 0; i < rowsP; i++) {
        for (long j = 0; j < colsP; j++) {
            fmpz_set_ui(fmpz_mat_entry(P, i, j), n_randint(state, 10) + 1);
        }
    }
    fmpz_mat_print_pretty(P);
    cout << endl;

    cout << "Q: ";
    for (long i = 0; i < rowsQ; i++) {
        for (long j = 0; j < colsP; j++) {
            fmpz_set_ui(fmpz_mat_entry(Q, i, j), n_randint(state, 10) + 1);
        }
    }
    fmpz_mat_print_pretty(Q);
    cout << endl;

    fmpz_t expected;
    fmpz_init(expected);
    fmpz_set_ui(expected, 0);
    for (long j = 0; j < colsP; j++) {
        fmpz_t temp;
        fmpz_init(temp);
        fmpz_mul(temp, fmpz_mat_entry(P, 0, j), fmpz_mat_entry(Q, 0, j));
        fmpz_add(expected, expected, temp);
        fmpz_clear(temp);
    }
    cout << "Expected result: ";
    fmpz_print(expected);
    cout << endl << endl;

    cout << "Initializing AIPE with 1024-bit security..." << endl;
    AIPE aipe(1024, colsP);

    aipe.encP(P, EncP);
    aipe.encQ(Q, 1, EncQ);

    mpz_t re;
    mpz_init(re);
    aipe.secureIP(EncP, EncQ, re);

    cout << "AIPE IP";
    gmp_printf("%Zd\n", re);

    mpz_t expected_mpz;
    mpz_init(expected_mpz);
    fmpz_get_mpz(expected_mpz, expected);

    if (mpz_cmp(expected_mpz, re) == 0) {
        cout << "\nInner product computed correctly." << endl;
    } else {
        cout << "\nResults do not match." << endl;
    }

    mpz_clear(re);
    mpz_clear(expected_mpz);
    fmpz_clear(expected);
    fmpz_mat_clear(P);
    fmpz_mat_clear(Q);
    fmpz_mat_clear(EncP);
    fmpz_mat_clear(EncQ);
    flint_randclear(state);
}

int main() {
    testAIPE();
    return 0;
}