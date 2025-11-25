 #ifndef ENCRYPT_H_INCLUDED
#define ENCRYPT_H_INCLUDED
#include <fstream>//读
#include <iostream>//写
#include <string>
#include <vector>
#include <algorithm>
#include <AIPE/aipe.h>
#include <BGN/BGN.h>
#include "fmpz_mat.h"

#define NODES_NUM 963// vertex number
#define EDGES_NUM 18813 //edge number
#define ATTRIBUTE_NUM 20 //attribute number
#define Max_Core 44 //max core number

struct Community{
	int id;//community id
	int core;//core number
	std::vector<int> avec;//attribute vector
	std::vector<int> evec;//edge vector

};
struct Edge {
	int startNode;
	int endNode;
};
struct Token{
	element_t theta;//th encrypted of core number constraint
	fmpz_mat_t qvec;//search attribute vector aipe

	Token(BGN& bgn,AIPE&aipe){
    	element_init_same_as(theta, bgn.G1);
    	fmpz_mat_init(qvec, 1, 2*ATTRIBUTE_NUM+1);
    }
    ~Token(){
    	element_clear(theta);
    	fmpz_mat_clear(qvec);
    }
    Token(Token& other) {
        element_init_same_as(theta, other.theta);
        element_set(theta, other.theta);
        
        fmpz_mat_init_set(qvec, other.qvec);
    }

    // Assignment operator
    /*Token& operator=(Token& other) {
        if (this != &other) {  // Self-assignment check
            // Clear existing data
            element_clear(theta);
            fmpz_mat_clear(qvec);
            
            // Copy theta
            element_init_same_as(theta, other.theta);
            element_set(theta, other.theta);
            
            // Copy qvec
            fmpz_mat_init_set(qvec, other.qvec);
        }
        return *this;
    }*/
};
struct CanCommunity{
	int id;
	mpz_t score;

	CanCommunity() { mpz_init(score); }

    // Copy Constructor
    CanCommunity(const CanCommunity& other) {
        mpz_init_set(score, other.score);  // Copy value of score
        id = other.id;
    }

    // Move Constructor
    CanCommunity(CanCommunity&& other) noexcept {
        mpz_init(score);
        mpz_swap(score, other.score);  // Efficiently swap values
        id = other.id;
    }

    // Destructor
    ~CanCommunity() { mpz_clear(score); }
};
struct EncIndex{
	int id;
	element_t core;
	fmpz_mat_t avec;
	element_t evec[EDGES_NUM];

	EncIndex(BGN& bgn,AIPE&aipe){
     // Initialize with BGN pairing context
        id=0;
    	element_init_same_as(core, bgn.G1);
    	fmpz_mat_init(avec, 1, 2*ATTRIBUTE_NUM+1);

        for (int i = 0; i < EDGES_NUM; ++i) {
            element_init_same_as(evec[i], bgn.G1); // Initialize temp
    }

	}
    ~EncIndex(){
    	
    	element_clear(core);
    	fmpz_mat_clear(avec);
        for (int i = 0; i < EDGES_NUM; ++i) {
            element_clear(evec[i]);
          }

    }
     // Delete copy constructor and copy assignment operator
    EncIndex(const EncIndex&) = delete;
    EncIndex& operator=(const EncIndex&) = delete;

    EncIndex(EncIndex&& other) noexcept {
        id = other.id;

        element_init_same_as(core, other.core);
        element_set(core, other.core);

        fmpz_mat_init(avec, other.avec->r, other.avec->c);
        fmpz_mat_set(avec, other.avec);

        for (int i = 0; i < EDGES_NUM; ++i) {
            element_init_same_as(evec[i], other.evec[i]);
            element_set(evec[i], other.evec[i]);
            //element_clear(other.evec[i]);
        }

        //element_clear(other.core);
        //fmpz_mat_clear(other.avec);

    //std::cout << "EncIndex with ID: " << id << " moved successfully." << std::endl;

    }

    // Enable move assignment operator
    EncIndex& operator=(EncIndex&& other) noexcept {
        if (this != &other) {
            element_clear(core);
            fmpz_mat_clear(avec);
           for (auto& e:evec) {
                element_clear(e);
            }

            id = other.id;
            
            element_init_same_as(core, other.core);
            element_set(core, other.core);

            fmpz_mat_init(avec, other.avec->r, other.avec->c);
            fmpz_mat_set(avec, other.avec);

            for (int i = 0; i < EDGES_NUM; ++i) {
                element_init_same_as(evec[i], other.evec[i]);
                element_set(evec[i], other.evec[i]);
                element_clear(other.evec[i]);
            }

            element_clear(other.core);
            fmpz_mat_clear(other.avec);

        }
        return *this;
    }

};
struct EncET{
	element_t startNode;
	element_t endNode;
};

class Encrypt{

private:
	void vectorToMatrix(const std::vector<int>& avec,int size,fmpz_mat_t P);
	BGN bgn;
	AIPE aipe;
    
	std::vector<int> genSearchAttribute();
	int getLargestScoreCommunityId(const std::vector<CanCommunity>& candidate);
public:
	std::vector<Community> cIndex;
	std::vector<Edge> eTable;
	std::vector<EncIndex> CI;
	EncET ET[EDGES_NUM];


	Encrypt(const BGN& bgn_obj, const AIPE& aipe_obj) : bgn(bgn_obj),aipe(aipe_obj) {
        std::cout << "Encrypt object created at address: " << this << std::endl;
    }
	//Encrypt(BGN& bgn_obj) : bgn(bgn_obj) {}
	~Encrypt(){
            std::cout << "Encrypt object destroyed at address: " << this << std::endl;
    	CI.clear(); 
    	cIndex.clear();
    	eTable.clear();
	}

	void readcIndex(const std::string& filename);
	void readeTable(const std::string& filename);
	void buildCI();
	void buildET();
	void genToken(int theta,Token& stoken);
	int Search(Token& stoken);
	std::vector<EncET> determineET(int id);
	int decryptEdges(std::vector<EncET> encryptedEdges);

	void printcIndex();
	void printeTable();
	void printCI();
	void printET();
	

};
#endif 