#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip> 
#include <sys/time.h>
#include<ctime>
#include<random>
#include <time.h>
#include <list>
#include <stdlib.h>
#include "Encrypt.h"


using namespace std;

double TimeCI = 0.0;//
double TimeET = 0.0;//
double TimeSearch1 = 0.0;//time for 100 searches
double TimeSearch2 = 0.0;//
double TimeSearch3 = 0.0;//
double TimeSearch4 = 0.0;//
double TimeSearch5 = 0.0;//
double TimeDecrypt = 0.0;//the single decryption time


double timeCost(timespec start, timespec end) {

	timespec temp;
	if ((end.tv_nsec - start.tv_nsec) < 0) {// tv_nsec 纳秒数
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else {
		temp.tv_sec = end.tv_sec - start.tv_sec;//tv_sec 秒数
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	double ret;
	ret = (double)temp.tv_sec + (double)temp.tv_nsec / 1000000000;
	return ret;
}

void Encrypt::readcIndex(const std::string& filename){
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Community temp;

        iss >> temp.id >> temp.core;

        for (int i = 0; i < ATTRIBUTE_NUM; ++i) {
            int attribute;
            iss >> attribute;
            temp.avec.push_back(attribute);
        }


        for (int i = 0; i < EDGES_NUM; ++i) {
            int edge;
            iss >> edge;
            temp.evec.push_back(edge);
        }

        cIndex.push_back(temp);
    }

    file.close();
}
void Encrypt::readeTable(const std::string& filename){
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Edge temp;
        iss >> temp.startNode >> temp.endNode;
        eTable.push_back(temp);
    }

    file.close();
}
void Encrypt::vectorToMatrix(const std::vector<int>& avec,int size,fmpz_mat_t P){//convert attribute vector to matrix, size=ATTRIBUTE_NUM

	fmpz_mat_init(P, 1, size);
	if(avec.size()==size){
        for (int i = 0; i < size; ++i) {
        fmpz_set_si(fmpz_mat_entry(P, 0, i), avec[i]);
        }
    }
    else{
        for (int i = 0; i < size; ++i) {
        fmpz_set_si(fmpz_mat_entry(P, 0, i), 0);
    }   
}
	
}
void Encrypt::buildCI(){//encrypt cIndex and store in CI
    
    //CI.clear();
    std::cout << "--- start buildCI " << std::endl;
    
    try{
        CI.reserve(cIndex.size());

        for (const auto& community : cIndex) {

        try{
            EncIndex temp(bgn, aipe);

            temp.id=community.id;

            mpz_t core_mpz;
            mpz_init_set_ui(core_mpz, community.core);  // Convert core to mpz_t type
        
            bgn.encrypt(core_mpz,temp.core);         // Encrypt and store in temp.core
            mpz_clear(core_mpz);       

            fmpz_mat_t P;
            fmpz_mat_init(P, 1, ATTRIBUTE_NUM);
            vectorToMatrix(community.avec,ATTRIBUTE_NUM,P);
            aipe.encP(P,temp.avec);
            fmpz_mat_clear(P);

            for (int i = 0; i < community.evec.size(); ++i) {

                mpz_t i_mpz;
                element_t tt;
                element_init_same_as(tt, bgn.G1);
                mpz_init_set_ui(i_mpz, community.evec[i]);  
                bgn.encrypt(i_mpz,tt);            // Encrypt and assign to temp1
                mpz_clear(i_mpz);
                element_set(temp.evec[i],tt);
            }
            
            //std::cout<<"evec ok"<<std::endl;

            CI.push_back(std::move(temp));
            //std::cout<<community.id<<"社区CI完成"<<std::endl;
        
            }
        catch (const std::exception& ex) {
            std::cerr << "Error processing community " << community.id << ": " << ex.what() << std::endl;
        }
    }
    std::cout << "buildCI Completed!" << std::endl;

    }catch (const std::exception& ex) {
        std::cerr << "Fatal error in buildCI: " << ex.what() << std::endl;
        // Clean up any partial results if needed
        CI.clear();
        throw;
    }
}

void Encrypt::buildET(){//encrypt eTable and store in ET
     std::cout << "--- start buildET " << std::endl;

    for (int i=0;i<EDGES_NUM;i++) {

        mpz_t node_mpz;

        mpz_init_set_ui(node_mpz, eTable[i].startNode); 
        element_init_same_as(ET[i].startNode, bgn.G1); 
        bgn.encrypt(node_mpz,ET[i].startNode); 

        mpz_init_set_ui(node_mpz, eTable[i].endNode);
        element_init_same_as(ET[i].endNode, bgn.G1);  
        bgn.encrypt(node_mpz,ET[i].endNode); 

        mpz_clear(node_mpz); 

    }
     std::cout << "buildET Completed!" << std::endl;
}
std::vector<int> Encrypt::genSearchAttribute(){
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<int> attributes(ATTRIBUTE_NUM);
    std::iota(attributes.begin(), attributes.end(), 0); // Fill with values 0, 1, ..., ATTRIBUTE_NUM - 1

    std::shuffle(attributes.begin(), attributes.end(), gen);

    std::uniform_int_distribution<> dist(1, ATTRIBUTE_NUM);
    int numElements = dist(gen);

    return std::vector<int>(attributes.begin(), attributes.begin() + numElements);

}
void Encrypt::genToken(int theta, Token& st){//generate search token

    std::vector<int> qvec(ATTRIBUTE_NUM, 0);

    std::vector<int> set=genSearchAttribute();//generate random attribute set
    for (const auto& position : set) {
        qvec[position]=1;
    } 

    fmpz_mat_t Q;
    vectorToMatrix(qvec,ATTRIBUTE_NUM,Q);

    //std::cout<<"Q="<<std::endl;
    //fmpz_mat_print_pretty(Q);
    aipe.encQ(Q,1,st.qvec);

    mpz_t theta_mpz;
    mpz_init_set_ui(theta_mpz, theta);
    
    bgn.encrypt(theta_mpz,st.theta);

    mpz_clear(theta_mpz);
    fmpz_mat_clear(Q);

}
int Encrypt::getLargestScoreCommunityId(const std::vector<CanCommunity>& candidate) {
    if (candidate.empty()) {
        return 100000;
    }

    size_t maxIndex = 0;

    for (size_t i = 1; i < candidate.size(); i++) {

        if (mpz_cmp(candidate[i].score, candidate[maxIndex].score) > 0) {
            maxIndex = i;
        }
    }

    return candidate[maxIndex].id;
}
int Encrypt::Search(Token& st){//privacy-preserving community search algorithm, return the id of the searched community id

    std::vector<CanCommunity> candidate;//store the id of communities that meet the core number constraint


    for(auto& community : CI){

        if(bgn.comSimple(community.core,st.theta)){//core number filtering
            CanCommunity temp;
            temp.id=community.id;
            mpz_t score;
            mpz_init(score);

            aipe.secureIP(community.avec,st.qvec,score);// calculate attribute-driven score
            //gmp_printf("Score=: %Zd\n", score);
            mpz_set(temp.score,score);

            candidate.push_back(temp);
            mpz_clear(score);
        }
    }

    std::cout<<"the number of candidate communities="<<candidate.size()<<std::endl;
    for(const auto& community : candidate){
        std::cout<<"community id="<<community.id<<" ";
        gmp_printf("Score=: %Zd\n", community.score);
    }


    int resultid=getLargestScoreCommunityId(candidate);//id of resulting community

    std::cout<<"id of resulting community="<<resultid<<std::endl;
    return resultid;

}
std::vector<EncET> Encrypt::determineET(int id){//return the encrypted edges in the resulting community

    std::vector<EncET> encryptedEdges;//initialize encrypted edge vector
    if(id==100000){
        return encryptedEdges;//no matching community found
    }

    for(auto& community : CI){
        if(community.id==id){
            for(int i=0;i<EDGES_NUM;++i){
                EncET encEdge;
                element_init_same_as(encEdge.startNode, bgn.G1);
                element_init_same_as(encEdge.endNode, bgn.G1);

                // Homomorphic multiplication: evec[i] * ET[i]
                bgn.mul(community.evec[i], ET[i].startNode, encEdge.startNode);
                bgn.mul(community.evec[i], ET[i].endNode, encEdge.endNode);

                encryptedEdges.push_back(encEdge);
            }
            cout<<endl;
            break;
        }
    }
    return encryptedEdges;

}
int Encrypt::decryptEdges(std::vector<EncET> encryptedEdges){//just test
    std::vector<Edge> edges;
    if(encryptedEdges.empty()){
        std::cout<<"There is no matching community found in this search"<<std::endl;
        return 0;
    }
    cout<<"--- start decrypting "<<std::endl;

    for(int i=0;i<encryptedEdges.size();i++){
        mpz_t start, end;
        mpz_init(start);
        mpz_init(end);

        //bgn.decrypt(encryptedEdges[i].startNode, start);
        bgn.decrypt(ET[i].startNode, start);
       // cout<<"start node decrypted: "<<mpz_get_si(start)<<std::endl;
        //bgn.decrypt(encryptedEdges[i].endNode, end);
        bgn.decrypt(ET[i].endNode, end);
        cout<<"("<<mpz_get_si(start)<<", "<<mpz_get_si(end)<<")";

        if(mpz_cmp_ui(start, 0) != 0 && mpz_cmp_ui(end, 0) != 0){
            Edge temp;
            temp.startNode = mpz_get_si(start);
            temp.endNode = mpz_get_si(end);
            edges.push_back(temp);
        }

        mpz_clear(start);
        mpz_clear(end);
    }
    cout<<std::endl;

    /*std::cout<<"The number of edges in the resulting community=";
    if(edges.empty()){
        std::cout<<"(none)";
    } else {
        std::cout<<edges.size()<<" ";
    }*/
   // std::cout<<endl;

    return edges.size();

}

void Encrypt::printcIndex(){
	
	for (const auto& community : cIndex) {
        std::cout << community.id<<". core=" << community.core << std::endl;
        /*std::cout<<"avec=(";
        for (const auto& i: community.avec) {
            std::cout <<" "<< i;
        }
        std::cout<<")"<<std::endl;
        std::cout<<"evec=(";
        for (const auto& i: community.evec) {
            std::cout <<" "<< i;
        }
        std::cout<<")"<<std::endl;
        std::cout << std::endl;*/
    }
}
void Encrypt::printeTable(){	
	for (const auto& edge : eTable) {
        std::cout << "("<<edge.startNode<<"," << edge.endNode<<")";      
    }
}
void Encrypt::printCI(){
    
    for (const auto& community : CI) {
        std::cout<<"id="<<community.id<<" ";

        element_printf("core = %B\n", community.core);
        std::cout<<"avec="<<std::endl;
        //fmpz_mat_print_pretty(community.avec);

        std::cout<<std::endl;
        std::cout<<"evec="<<std::endl;

        for (int i=0;i<EDGES_NUM;i++) {
            element_printf(" %B\n", community.evec[i]);
        }
        std::cout<<"结束"<<std::endl;
    }
}

void Encrypt::printET(){
    
    for (const auto& edge : ET) {

        element_printf("%B ", edge.startNode);
        element_printf("%B\n", edge.endNode);

    }
}
    
    
int main(int argc, char** argv) {
	timespec beginT, endT;

	const std::string datasetname="Reed98";
	const std::string cIndexfile="./data/output/"+datasetname+"-"+std::to_string(ATTRIBUTE_NUM)+"-Community.out";
	const std::string eTablefile = "./data/output/"+datasetname+"-"+std::to_string(ATTRIBUTE_NUM)+"-Edge.out";

	BGN bgn;
    AIPE aipe(1024,ATTRIBUTE_NUM);
	Encrypt graph(bgn,aipe);

	graph.readcIndex(cIndexfile);
	graph.readeTable(eTablefile);

    std::cout<<std::endl;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
	graph.buildCI();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
    TimeCI+= timeCost(beginT, endT);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
    graph.buildET();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
    TimeET+= timeCost(beginT, endT);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(4, Max_Core);

    Token stoken(bgn,aipe);
    graph.genToken(dist(gen),stoken);
    std::cout<<"single search for testing decryption!!"<<std::endl;
    int result=graph.Search(stoken);
    if (result!=100000){
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int num=graph.decryptEdges(graph.determineET(result));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeDecrypt+= timeCost(beginT, endT);
    }else{
        std::cout<<"There is no matching community found in this search"<<std::endl;
    }

    std::cout<<"--- start search （The demo displays only 10 searches!）"<<std::endl;
    for(int i=0;i<10;i++){
        Token stoken(bgn,aipe);
        graph.genToken(dist(gen),stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int result=graph.Search(stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeSearch1+= timeCost(beginT, endT);
        if (result!=100000) {
            std::cout<<"The resulting community of 10-"<<i+1<<"-th search is "<<result<<std::endl;
        } else {
            std::cout<<"There is no matching community found in 10-"<<i+1<<"-th search"<<std::endl;
        }
    }
    //200次搜索
    /*for(int i=0;i<200;i++){
        std::random_device rd; 
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(5, Max_Core);

        Token stoken(bgn,aipe);

        graph.genToken(dist(gen),stoken);
        //std::cout<<"搜索陷门完成"<<std::endl;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int result=graph.Search(stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeSearch2+= timeCost(beginT, endT);
        //graph.decryptEdges(graph.determineET(result));//算解密时间
        std::cout<<"200-"<<i<<"-th search Completed！"<<std::endl;

    }

    //300搜索

    for(int i=0;i<300;i++){
        std::random_device rd; 
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(5, Max_Core);

        Token stoken(bgn,aipe);

        graph.genToken(dist(gen),stoken);
        //std::cout<<"搜索陷门完成"<<std::endl;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int result=graph.Search(stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeSearch3+= timeCost(beginT, endT);
        //graph.decryptEdges(graph.determineET(result));//算解密时间
        std::cout<<"300-"<<i<<"-th search Completed！"<<std::endl;

    }
    //400搜索

    for(int i=0;i<400;i++){
        std::random_device rd; 
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(5, Max_Core);

        Token stoken(bgn,aipe);

        graph.genToken(dist(gen),stoken);
        //std::cout<<"搜索陷门完成"<<std::endl;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int result=graph.Search(stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeSearch4+= timeCost(beginT, endT);
        //graph.decryptEdges(graph.determineET(result));//算解密时间
        std::cout<<"400-"<<i<<"-th search Completed！"<<std::endl;

    }
    //500搜索

    for(int i=0;i<500;i++){
        std::random_device rd; 
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(5, Max_Core);

        Token stoken(bgn,aipe);

        graph.genToken(dist(gen),stoken);
        //std::cout<<"搜索陷门完成"<<std::endl;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        int result=graph.Search(stoken);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        TimeSearch5+= timeCost(beginT, endT);
        //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &beginT);
        //graph.decryptEdges(graph.determineET(result));//算解密时间
        //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endT);
        //TimeEncrypt+= timeCost(beginT, endT);
        std::cout<<"500-"<<i<<"-th search Completed！"<<std::endl;
    }*/
    

    int cIndexsizeMB=graph.cIndex.size()*sizeof(Community)>>20;//MB
    int cIndexsizeKB=graph.cIndex.size()*sizeof(Community)>>10;//KB

    size_t totalSize = sizeof(fmpz) * 1 * (2*ATTRIBUTE_NUM+1)+sizeof(int)+sizeof(element_t)*(EDGES_NUM+1);
    size_t cIndexsizeB=graph.cIndex.size()*sizeof(int)*(2+ATTRIBUTE_NUM+EDGES_NUM);//B

    int eTablesizeMB=graph.eTable.size()*sizeof(Edge)>>20;//MB
    int eTablesizeKB=graph.eTable.size()*sizeof(Edge)>>10;//KB

    int CIsizeMB=graph.CI.size()*sizeof(EncIndex)>>20;//MB
    int CIsizeKB=graph.CI.size()*sizeof(EncIndex)>>10;//KB
    int CIsizeB=graph.CI.size()*totalSize;//B

    int ETsizeMB=EDGES_NUM*sizeof(EncET)>>20;//MB
    int ETsizeKB=EDGES_NUM*sizeof(EncET)>>10;//KB
    std::cout <<std::endl;
    std::cout <<std::endl;
    std::cout<<"----------------------------------------------"<<std::endl;
    std::cout <<"Dataset："<<datasetname<<std::endl;
    std::cout << "Vertex number：" << NODES_NUM << std::endl;
    std::cout << "Edge number：" << EDGES_NUM << std::endl;
    std::cout << "Attribute number：" << ATTRIBUTE_NUM << std::endl;
    std::cout << std::endl;
    std::cout << "time for builtCI (s)：" << TimeCI << std::endl;
    std::cout << "time for builtET (s)：" << TimeET << std::endl;
    std::cout << "time for 10 searches (s)：" << TimeSearch1 << std::endl;
    //std::cout << "time for 200 searches：" << TimeSearch2 << std::endl;
    //std::cout << "time for 300 searches：" << TimeSearch3 << std::endl;
    //std::cout << "time for 400 searches：" << TimeSearch4 << std::endl;
    //std::cout << "time for 500 searches：" << TimeSearch5 << std::endl;
    std::cout << "single decryption time (s)：" << TimeDecrypt<< std::endl;

    std::cout <<std::endl;
    std::cout << "size of cIndex (MB)：" << cIndexsizeMB << std::endl;
    std::cout << "size of CI (MB)：" << CIsizeMB << std::endl;
    std::cout << "size of eTable (MB)：" << eTablesizeMB << std::endl;
    std::cout << "size of ET (MB)：" << ETsizeMB << std::endl;
    std::cout <<std::endl;

    std::cout << "size of cIndex (KB)：" << cIndexsizeKB << std::endl;
    std::cout << "size of CI (KB)：" << CIsizeKB << std::endl;
    std::cout << "size of eTable (KB)：" << eTablesizeKB << std::endl; 
    std::cout << "size of ET (KB)：" << ETsizeKB << std::endl;
    //std::cout << "(B)：" << matSize << std::endl;
    //std::cout << "size of cIndex  (B)：" << cIndexsizeB << std::endl;
    //std::cout << "   CI 索引大小(B)：" << CIsizeB << std::endl;

    
	
	return(0);
}