#ifndef AIPE_H_INCLUDED
#define AIPE_H_INCLUDED

#include <math.h>
#include <vector>
#include <queue>
#include <iomanip>

#include "skip.h"


// the pure AIPE without ip packing and sequential scan

class AIPE
{
private:

    //vector<vector<IPTuple> > results;   // store the top-k answers of each query

    long dim;                    // dim of data and query vectors

    PublicParameter ppl;
    MasterSecretKey mskl;

    //fmpz_mat_t EncP;           // encrypted data vectors
    //fmpz_mat_t EncQ;           // encrypted query vectors

public:
    AIPE(int bits, long c);

    void encP(fmpz_mat_t P,fmpz_mat_t EncP);

    void encQ(fmpz_mat_t Q, int queryNum,fmpz_mat_t EncQ);

    void secureIP(fmpz_mat_t EncP,fmpz_mat_t EncQ,mpz_t re);

    //void phase3(vector<vector<IPTuple> > results);

    //void printTopK(vector<vector<IPTuple> > results);                                   // print the results

    void clearResult();

    ~AIPE();
};

AIPE::AIPE(int bits, long c){//bit安全参数 c向量长度

    initialize(&ppl, &mskl, c);
    setup(&ppl, &mskl, 1024);
    dim=c;
}

/***************************************************************************/

void AIPE::encP(fmpz_mat_t P,fmpz_mat_t EncP)
{

    long rows, cols;

    rows=P->r;
    cols=P->c;

    fmpz_mat_init(EncP, rows, 2*cols+1);

    // iterate on each record

    for (long i=0; i<rows; i++)
    {
        encrypt(P->rows[i], EncP->rows[i], &ppl, &mskl);
    }

    //fmpz_mat_print_pretty(EncP);

}


void AIPE::encQ(fmpz_mat_t Q, int queryNum,fmpz_mat_t EncQ)
{
    if(Q->r<queryNum)
    {
        perror("query number exceeds\n");
        exit(0);
    }


    fmpz_mat_init(EncQ, queryNum, 2*Q->c+1);

    for(long i=0; i<queryNum; i++)
    {
        keyGen(Q->rows[i], EncQ->rows[i], &ppl, &mskl);
    }

}


void AIPE::secureIP(fmpz_mat_t EncP,fmpz_mat_t EncQ,mpz_t re)
{
    vector<vector<IPTuple> > results;  
    fmpz_t temp;  // Temporary variable for holding intermediate results
    fmpz_init(temp);

    fmpz_t ip;
    mpz_init(re);
    mpz_set_ui(re, 0);

    fmpz_init(ip);

    for(long i=0; i<EncQ->r; i++)
    {
        priority_queue<IPTuple, vector<IPTuple>, comparator > topK;

        // push k empty tuples into the heap

        for(int j=0; j<1; j++)
        {
            IPTuple t;

            fmpz_init(t.ip);

            t.auxilary=_fmpz_vec_init(2*dim+1);

            fmpz_set_ui(t.ip, 0);

            topK.push(t);
        }

        // linear scan on every encrypted data

        for(long j=0; j<EncP->r; j++)
        {

            decrypt(EncP->rows[j], EncQ->rows[i], ip, &ppl);              // compute q^T p

            if(fmpz_cmp(ip, topK.top().ip) >0)
            {
                IPTuple result = topK.top();

                topK.pop();

                fmpz_set(result.ip, ip);                                  // set ip

                _fmpz_vec_set(result.auxilary, EncP->rows[j], 2*dim+1);

                topK.push(result);
            }

        }

        // store the results into answer sets

        vector<IPTuple> answers;

        IPTuple result;

        for(int j=0; j<1; j++)
        {
            result=topK.top();

            topK.pop();

            answers.push_back(result);

        }

        results.push_back(answers);

    }

    fmpz_clear(ip);

    for (ulong i=0; i<results.size(); i++)
    {
        // deal with the answer set of i-th query

        for(ulong j=0; j<results[i].size(); j++)
        {
            decrypt2(results[i][j].auxilary, &ppl, &mskl);    // the plain auxilary vector is stored in the last dim items
        }

    }

    for (ulong i=0; i<results.size(); i++)
    {
        // deal with the answer set of i-th query

        for(ulong j=0; j<results[i].size(); j++)
        {
            fmpz_get_mpz(re,results[i][j].ip);
            //fmpz_print(results[i][j].ip);

            //cout<<" ";

            if(j==results[i].size()-1){
                //cout<<endl;
            }
        }

    }

    // Clean up allocated memory for IPTuple structures
    for (ulong i=0; i<results.size(); i++)
    {
        for(ulong j=0; j<results[i].size(); j++)
        {
            fmpz_clear(results[i][j].ip);
            _fmpz_vec_clear(results[i][j].auxilary, 2*dim+1);
        }
    }

    fmpz_clear(temp);

}



void AIPE::clearResult()
{
    
}

AIPE::~AIPE()
{
    clearup(&ppl, &mskl);

}

#endif // AIPE_H_INCLUDED
