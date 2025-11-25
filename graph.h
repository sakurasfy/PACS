#ifndef _GRAPH_H_
#define _GRAPH_H_
#include <vector>
#include <iostream>
#include <fstream>

#define NODES_NUM 963
#define EDGES_NUM 18813
#define ATTRIBUTE_NUM 60
using namespace std;

struct Edge {
	int startNode;
	int endNode;
};
struct Vertex{
	int id;
	int ver_core;
	int number;//节点所在连通分量编号
	std::vector<int> attr;//节点的属性集合
};
struct Community1{
	int id;//社区标识
	int com_core;//社区核值
	std::vector<int> attribute_vec;//社区属性向量
	vector<int> edge_vec;//社区边向量

};
struct Community2{//基础社区索引
	//int id;//社区标识
	int com_core;//社区核值
	vector<int> ver;//社区节点
	vector<Edge> edg;//社区里的边

};

class Graph{

private:
	std::vector<int> bitwiseAdd(const std::vector<int>& x, const std::vector<int>& y);
	void addnoise();
public:
	std::vector<Edge>graphIndex[NODES_NUM];//原始图索引
	Vertex verIndex[NODES_NUM];//存放每个节点的核值
	Vertex degreeIndex[NODES_NUM];//存放每个节点的度数以及属性集合
	int omega;//最大核值
	std::vector<Community2> comIndex;//存放所有社区
	std::vector<Community1> cIndex;//未加密社区索引
	std::vector<Edge> eTable;//未加密边表，由InitGraph初始化
	Graph();
	~Graph();

	void InitGraph(const char* graphfile);//初始化函数，从文件中读取
	void DFS(std::vector<Vertex> verBYcore[],std::vector<Vertex>& visited,int size,int v,int count);//深度优先搜索，用来求连通分量 visited用来标记节点是否被访问
	std::vector<Community2> Com_Community(std::vector<Vertex> k,int x);
	void Com_Core();//求解节点核值
	void print_Graph();
	void print_eTable();
	void print_Index(const Vertex index[],int size);
	void print_Index2(const std::vector<Vertex> index[],int size);
	void print_comIndex(const std::vector<Community2> index);
	void ComOmega();
	void InitverBYcore(vector<Vertex>* verBYcore);
	void Com_Number(vector<Vertex> verBYcore[],int size);
	bool isMember(const std::vector<int>& vec, int element);
	std::vector<Edge> DetermineEdge(vector<int> vertex);
	std::vector<int> setEdgeVector(std::vector<Edge> edges);
	void Com_Community(std::vector<Vertex> verBYcore[],int size);
	void setAttributeVector(std::vector<int> vertex);
	std::vector<int> setTovector(int vertex);
	std::vector<int> Com_ComAttrVector(std::vector<int> vertex);
	void buildcIndex();
	void print_cIndex();
	void outputcIndex(const std::string& filename);
	void outputeTable(const std::string& filename);
};
#endif
