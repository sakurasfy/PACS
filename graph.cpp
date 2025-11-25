#include <fstream>//读
#include <iostream>//写
#include "graph.h"
#include <sys/time.h>
#include<ctime>
#include<random>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <random>  
#include <chrono>   

Graph::Graph() {}
Graph::~Graph() {
	for (int i = 0; i < NODES_NUM; ++i) {
        graphIndex[i].clear(); 
    }
    comIndex.clear(); 
    cIndex.clear();   
    eTable.clear();   
}

void Graph::InitGraph(const char* graphfile) {
	int s,t;
	std::ifstream infile(graphfile);
    if (!infile.is_open()) {
        std::cout << "Failed to open file!" << std::endl;
        return;
    }
	while (infile >> s >> t) {
        // initialize graphIndex
        Edge edge1 = {s, t};
        Edge edge2 = {t, s};
        graphIndex[s].push_back(edge1);
        graphIndex[t].push_back(edge2);

        // initialize eTable
        if (s < t) {
            eTable.push_back(edge1);
        } else {
            eTable.push_back(edge2);
        }
    }
	
	for(int i=0;i<NODES_NUM;i++){
		degreeIndex[i].id=i;
		degreeIndex[i].ver_core=graphIndex[i].size();
	}

	//randomly shuffle the elements in eTable
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(eTable.begin(), eTable.end(), std::default_random_engine(seed));
}
void Graph::ComOmega(){
	omega=0;
	for(int i=0;i<NODES_NUM;i++){
		if(verIndex[i].ver_core>omega)
			omega=verIndex[i].ver_core;
	}
	std::cout<<"omega="<<omega<<std::endl;
}
void Graph::Com_Core(){//求解节点核值

	std::vector<Vertex> temp;
	for(int i=0;i<NODES_NUM;i++){
		temp.push_back(degreeIndex[i]);
	}

	while(!temp.empty()){
		//cout<<"size="<<degree.size()<<endl;
		std::sort(temp.begin(),temp.end(),[](const Vertex& a, const Vertex& b){
        	return a.ver_core < b.ver_core;
    	});
		Vertex minnode=temp.front();	
		//cout<<minnode.node<<"出队"<<endl;
		verIndex[minnode.id].id=minnode.id;
		verIndex[minnode.id].ver_core=minnode.ver_core;//set core value
		for(const auto& edge : graphIndex[minnode.id]){//visit neighbors
			
			int end=edge.endNode;
			auto it = std::find_if(temp.begin(), temp.end(), [end](const Vertex& v) {
                return v.id == end;
            });
            if (it != temp.end() && it->ver_core > minnode.ver_core) {
                it->ver_core -= 1;
            }						
		}
		temp.erase(temp.begin());
	
	}
}
void Graph::print_Index(const Vertex index[],int size){

	for (int i=0;i<size;i++) {
            std::cout << "(" << index[i].id << ", core/degree= " << index[i].ver_core << ") 属性集合=";
        	for (const auto& attribute : index[i].attr) {
            	std::cout << attribute << " " ;
        }
        std::cout<<std::endl;
        }
}
void Graph::InitverBYcore(vector<Vertex>* verBYcore){
	Vertex temp;

	for(int i=0;i<omega;i++){
		for(int j=0;j<NODES_NUM;j++){
			if(verIndex[j].ver_core>=i+1){
				temp.id=j;
				temp.ver_core=0;
				temp.number=0;
				verBYcore[i].push_back(temp);
			}
		}
	}
}
void Graph::DFS(std::vector<Vertex> verBYcore[],std::vector<Vertex>& visited,int size,int v,int count){//深度优先搜索，用来求连通分量 visited用来标记节点是否被访问
	//cout<<"vertex"<<v<<endl;
	auto it=std::find_if(verBYcore[size].begin(),verBYcore[size].end(),[v](const Vertex& vertex){
        return vertex.id == v;
    });
    if(it!=verBYcore[size].end()){
    	it->number=count;
    }

	auto iter=std::find_if(visited.begin(),visited.end(),[v](const Vertex& vertex){
        return vertex.id == v;
    });
	if(iter!=visited.end()){
		iter->ver_core=1;//1表示访问
		//cout<<"iter"<<iter->degree<<endl;
		int i=0;
		while(i<graphIndex[v].size()){
			int neighbor_id = graphIndex[v][i].endNode;
            auto neighbor_it = std::find_if(visited.begin(), visited.end(), [neighbor_id](const Vertex& vertex) {
                return vertex.id == neighbor_id;
            });

            if (neighbor_it != visited.end() && !neighbor_it->ver_core) {
                DFS(verBYcore,visited,size, neighbor_it->id, count);
            }
			i++;
			
		}
		
	}
	//cout<<"结束"<<endl;
}
void Graph::Com_Number(vector<Vertex> verBYcore[],int size){//对verBYcore[i]中的每个节点计算其连通分量number
	int count;
	for(int i=0;i<size;i++){//
		count=0;
		std::vector<Vertex> visited=verBYcore[i];

		for(int j=0;j<visited.size();j++){
			auto it=find_if(visited.begin(),visited.end(),[&](const Vertex& vertex) {
                return vertex.id == visited[j].id;
            });

			if(it!=visited.end()&& it->ver_core == 0){
					DFS(verBYcore,visited,i,it->id,count+1);
					count++;

			}
		}
	}
}
bool Graph::isMember(const std::vector<int>& vec, int element) {
    return std::find(vec.begin(), vec.end(), element) != vec.end();
}
std::vector<Edge> Graph::DetermineEdge(vector<int> vertex){//determine the edge set of the community which consists of "vertex"
	std::vector<Edge> edges;
	for (const auto& node : vertex) {//determine neighbors in graphIndex
		for (const auto& neighbor : graphIndex[node]) {
			if(isMember(vertex,neighbor.endNode)){
				int smallNode = std::min(node, neighbor.endNode);
                int largeNode = std::max(node, neighbor.endNode);
                Edge edge={smallNode,largeNode};
        		auto exists = std::find_if(edges.begin(), edges.end(), [&](const Edge& e) {
                    return e.startNode == edge.startNode && e.endNode == edge.endNode;
                });

                // Only add if the edge does not already exist
                if (exists == edges.end()) {
                    edges.push_back(edge);
                }
			}
		}
    }
    return edges;
}
std::vector<int> Graph::setEdgeVector(std::vector<Edge> edges){//transform community edges to vector corresponding to eTable "edges"
	vector<int> edge_vec;
	for(int i=0;i<EDGES_NUM;i++){//initialize
		edge_vec.push_back(0);
	}

	for (const auto& edge : edges) {
		auto it= std::find_if(eTable.begin(), eTable.end(), [&](const Edge& e) {
                    return e.startNode == edge.startNode && e.endNode == edge.endNode;
                });
		if (it!=edges.end()) {
			int index = std::distance(eTable.begin(), it);
            edge_vec[index]=1;//the position of edge vector is set to 1
        }

	}

	//verify
	int sum=0;
	for (const auto& it: edge_vec) {
		sum=sum+it;
	}
	if(sum==edges.size()){
		std::cout<<"边向量设置成功"<<std::endl;
	}
	return edge_vec;

}
void Graph::Com_Community(std::vector<Vertex> verBYcore[],int size){//partition the vertices in layer k+1 into communities based on their "degree"

	for(int i=0;i<size;i++){

		int max=0;//连通分量最大值，分几个树节点the max number of connected components, the number of tree nodes
		if (!verBYcore[i].empty()) {
            // Find the Vertex with the maximum `number` in verBYcore[i]
            auto max_vertex = std::max_element(verBYcore[i].begin(), verBYcore[i].end(), 
                [](const Vertex& a, const Vertex& b) {
                    return a.number < b.number;
                });
            max=max_vertex->number;
        }
        //std::cout<<i<<"-level has "<<max<<" connected components."<<std::endl;
	
		for(int j=0;j<max;j++){

			Community2 temp;	
			std::vector<int> vertex;
			for (const auto& node : verBYcore[i]) {
           		if (node.number == j+1) {
                	vertex.push_back(node.id);
            	}
        	}

			temp.com_core=i+1;
			temp.ver=vertex;
			vector<Edge>edge=DetermineEdge(vertex);
			temp.edg=edge;
			comIndex.push_back(temp);

			//test success
			/*std::vector<int> test=setEdgeVector(edge);
			std::cout << "边向量=(" ;
			for (const auto& it : test) {
            	std::cout<< it << " "; 
        	}
        	std::cout << ")" ;*/
			
		}
	}	
}

void Graph::print_Graph(){

	for(int i=0;i<NODES_NUM;i++){
		std::cout << "graph[" << i << "] = ";
        for (const auto& edge : graphIndex[i]) {
            std::cout << "(" << edge.startNode << ", " << edge.endNode << ") ";
        }
        std::cout << std::endl;
	}
}
void Graph::print_Index2(const std::vector<Vertex> index[],int size){

	for(int i=0;i<size;i++){
		std::cout << "core=" << i+1 << " ";
        for (const auto& vertex : index[i]) {
            std::cout << "(" << vertex.id << ", " << vertex.ver_core<<", "<<vertex.number << ") ";
        }
        std::cout << std::endl;
	}
}
void Graph::print_comIndex(const std::vector<Community2> index){

	for (const auto& community : index) {
        std::cout << "core=" << community.com_core << " (";
        for (const auto& vertex_id : community.ver) {
            std::cout << vertex_id << " ";
        }
        std::cout << ")" << std::endl;
        for (const auto& edge: community.edg) {
            std::cout << "("<<edge.startNode<<","<<edge.endNode<<") ";
        }
        std::cout << std::endl;
    }
}
void Graph::print_eTable(){
	//std::cout<<eTable.size()<<std::endl;
	for (const auto& edge : eTable) {
            std::cout << "(" << edge.startNode << ", " << edge.endNode << ") "<<std::endl;
        }
        std::cout << std::endl;
}

void Graph::setAttributeVector(std::vector<int> vertex){//对由vertex构成的社区进行属性分配以及设置属性向量
	
	std::vector<int> w;//随机五个不重复属性
	for(int y=0;y<5;y++){
		int a=rand()%ATTRIBUTE_NUM;
		if (std::find(w.begin(), w.end(), a) == w.end()) { // Avoid duplicates
            w.push_back(a);
        }
	}

	//选择该社区内百分之八十的节点
	int size=static_cast<int>(0.8*vertex.size());
	std::vector<int> temp(vertex);
	std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(temp.begin(), temp.end(), g);
    temp.resize(size);
	//std::cout<<"size="<<temp.size()<<std::endl;
	//分配w
	for (const auto& node: temp) {
		auto it = std::find_if(degreeIndex, degreeIndex + NODES_NUM, [node](const Vertex& v) {
        	return v.id == node;
    	});
    	if (it != degreeIndex + NODES_NUM) {
			for (const auto& attribute : w) {
                if (std::find((*it).attr.begin(), (*it).attr.end(), attribute) == (*it).attr.end()) {
                    (*it).attr.push_back(attribute);
                }
            }
  		}
  	}
}
void Graph::addnoise(){
	//add noise for each vertex in the graph// for degreeIndex
    for (auto& node: degreeIndex) {
    	int noise_number=1+(rand()%5);//随机一到五个属性
    	//std::cout<<"noise="<<noise_number<<std::endl;
    	for(int i=0;i<noise_number;i++){
            int noise_attr = rand() % ATTRIBUTE_NUM;
            //std::cout<<"noise="<<noise_attr<<std::endl;
            // Ensure the noise attribute is unique within the node's attribute set
            if (std::find(node.attr.begin(), node.attr.end(), noise_attr) == node.attr.end()) {
                node.attr.push_back(noise_attr);
              
            }
        }
    }
}
std::vector<int> Graph::setTovector(int vertex){//convert attribute set to attribute vector
	vector<int> set;
	vector<int> resultvector;
	//find the attribute set of the vertex
	auto it = std::find_if(degreeIndex, degreeIndex + NODES_NUM, [vertex](const Vertex& v) {
        	return v.id == vertex;
    	});
    if (it != degreeIndex + NODES_NUM) {
    	set=it->attr;
    }

    //initialization
    for(int i=0;i<ATTRIBUTE_NUM;i++){
    	resultvector.push_back(0);
    }

    //set to 1
    for (const auto& position : set) {
            resultvector[position]=1;
    }

    //verify
    int sum=0;
    std::cout<<vertex<<"the attribute vector of vertex=";
    for (const auto& i : resultvector) {
    	std::cout<<i<<" ";
            sum=sum+i;
    }
    std::cout<<std::endl;
    if(sum==set.size())
    {
    	std::cout<<"the attribute vector conversion succeeded"<<std::endl;
    }

    return resultvector;
}
std::vector<int> Graph::bitwiseAdd(const std::vector<int>& x, const std::vector<int>& y) {
     size_t n = x.size();
    std::vector<int> result(n, 0);
    if(x.size()==y.size()){

        for (size_t i = 0; i < n; ++i) {        
            result[i] = x[i] + y[i] ;
        }
    }
    return result;
        
}
std::vector<int> Graph::Com_ComAttrVector(std::vector<int> vertex){//calculate the attribute vector of the community consisting of "vertex"
	
	std::vector<int> attr_vec;
	
	for(int i=0;i<ATTRIBUTE_NUM;i++){
    	attr_vec.push_back(0);
    }

    for (int i = 0; i < vertex.size(); i++) {
            std::vector<int> attrVector = setTovector(vertex[i]);

            attr_vec = bitwiseAdd(attr_vec, attrVector);
        }

        //Preprocess to take integer
    for(auto& i : attr_vec){
    	i=i*i/vertex.size();
    }

    return attr_vec;
}
void Graph::buildcIndex(){//build cIndex from conIndex, mainly to calculate community attribute vector and edge vector
	int id=0;
	addnoise();//add random attribute noise to each vertex in degreeIndex
	for (const auto& community : comIndex) {
		Community1 temp;
		temp.id=id;
		temp.com_core=community.com_core;
		temp.edge_vec=setEdgeVector(community.edg);
		setAttributeVector(community.ver);//assign attributes
		temp.attribute_vec=Com_ComAttrVector(community.ver);
		cIndex.push_back(temp);
		id++;

	}

}
void Graph::print_cIndex(){

	for (const auto& community : cIndex) {
        std::cout << "id=" << community.id <<" core="<<community.com_core<<std::endl;
        std::cout<<"属性向量"<<community.attribute_vec.size()<<"=( ";
        for (const auto& attribute : community.attribute_vec) {
            std::cout << attribute << " ";
        }
        std::cout << ")" << std::endl;
        //std::cout<<"边向量"<<community.edge_vec.size()<<"=( ";
        //for (const auto& edge: community.edge_vec) {
           // std::cout << edge<<" ";
        //}
        //std::cout << ")"<<std::endl;
    }
}
void Graph::outputcIndex(const std::string& filename) {//output cIndex to file
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }

    // Write each Community1 object in cIndex to the file
    for (const auto& community : cIndex) {
        // Write id and com_core
        outFile  << community.id <<" "<< community.com_core;

        for (int attribute : community.attribute_vec) {
            outFile <<" "<< attribute;
        }

        for (int edge : community.edge_vec) {
            outFile <<" "<<edge;
        }
        outFile << "\n";
    }

    outFile.close();
    std::cout << "Index written to " << filename << " successfully." << std::endl;
}
void Graph::outputeTable(const std::string& filename) {//output eTable to file
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }

    // Write each Community1 object in cIndex to the file
    for (const auto& edge : eTable) {
        // Write id and com_core
        outFile  << edge.startNode <<" "<< edge.endNode<<"\n";
    }

    outFile.close();
    std::cout << "Index written to " << filename << " successfully." << std::endl;
}
int main(int argc, char** argv) {

	const std::string datasetname = "Reed98";
	const std::string infilestr = "./data/input/"+datasetname+".in";
	const std::string outfile1str = "./data/output/"+datasetname+"-60-Community.out";
	const std::string outfile2str = "./data/output/"+datasetname+"-60-Edge.out";
	
	const char* infile= infilestr.c_str();
	const char* outfile1 = outfile1str.c_str();
	const char* outfile2 = outfile2str.c_str();

	Graph graph;

	graph.InitGraph(infile);
	graph.print_Graph();
	graph.Com_Core();
	graph.ComOmega();
	std::cout<<"-----------节点度数-----------"<<std::endl;
	graph.print_Index(graph.degreeIndex,NODES_NUM);
	std::cout<<"-----------节点核值-----------"<<std::endl;
	//graph.print_Index(graph.verIndex,NODES_NUM);


	
	std::vector<Vertex> verBYcore[graph.omega];//不同核值的节点
	graph.InitverBYcore(verBYcore);
	graph.Com_Number(verBYcore,graph.omega);
	std::cout<<"-----------节点核值-----------"<<std::endl;
	graph.print_Index2(verBYcore,graph.omega);

	graph.Com_Community(verBYcore,graph.omega);
	std::cout<<"-----------社区索引-----------"<<std::endl;
	graph.print_comIndex(graph.comIndex);

	std::cout<<"-----------随机化后的边表-----------"<<std::endl;
	graph.print_eTable();


	std::cout<<"-----------cIndex--------------"<<std::endl;
	graph.buildcIndex();
	graph.print_cIndex();

	std::cout<<"-----------节点属性集合-----------"<<std::endl;
	//graph.print_Index(graph.degreeIndex,NODES_NUM);

	graph.outputcIndex(outfile1); 
	graph.outputeTable(outfile2);
	

return(0);
}