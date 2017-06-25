#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <string.h>
#include <conio.h>
#include "Tokenizer.h"

#pragma warning(disable:4996)

using namespace std;

#define BLOCKSIZE 4096
#define DEGREE 510 // 3이상으로 할 것, 510이면 4096Byte로 설정됨
#define MAX 30  // 메모리 해제하기 위한 스택크기 
#define LEAF 1
#define INDEX 2
#define FULL 1
#define EPS 0.000001
#define INIT_GLOB_DEPTH 0
#define SHOW_DUPLICATE_BUCKETS 0

typedef struct _students
{
	char name[20];
	unsigned int studentID;
	float score;
	unsigned int advisorID;
}Students;

typedef struct _professors
{
	char name[20];
	unsigned int professorID;
	int salary;
}Professors;


typedef struct student_block
{
	Students records[BLOCKSIZE / sizeof(Students)];
}StudentBlock;

typedef struct professor_block
{
	Professors records[BLOCKSIZE / sizeof(Professors)];
}ProfessorBlock;


typedef struct _hashMap {
	int key;
	int tableNum;

}HashMap;


int studentCnt, professorCnt;

StudentBlock *readStudentBlocks, *writeStudentBlocks;
ProfessorBlock *readProfessorBlocks, *writeProfessorBlocks;

class Bucket {

public:
	int hashPrefix, size;
	map<int, int> hashTable;

	Bucket(int hashPrefix, int size) {
		this->hashPrefix = hashPrefix;
		this->size = size;
	}

	int insert(int _key, int _blockNum) {
		map<int, int>::iterator it;
		it = hashTable.find(_key);
		if (it != hashTable.end()) return -1;
		else if (isFull()) 
			return 0;
		else {
			hashTable[_key] = _blockNum;
			return 1;
		}
	}

	bool search(int key) {
		map<int, int>::iterator it;
		it = hashTable.find(key);
		if (it != hashTable.end())
			return true;
		else    return false;
	}

	int isFull(void) {
		if (hashTable.size() == size)
			return 1;
		else
			return 0;
	}

	int isEmpty(void) {
		if (hashTable.size() == 0)
			return 1;
		else
			return 0;
	}

	int gethashTableize() {

		return hashTable.size();
	}

	int getHashPrefix(void) {
		return hashPrefix;
	}

	int increaseHashPrefix(void) {
		hashPrefix++;
		return hashPrefix;
	}

	int decreaseHashPrefix(void) {
		hashPrefix--;
		return hashPrefix;
	}

	map<int, int> copy(void) {
		map<int, int> temp(hashTable.begin(), hashTable.end());
		return temp;
	}

	void clear(void) {
		hashTable.clear();
	}

	//insert values into Student.hash as binary format

	int writeHashFile(FILE *& fout) {

		map<int, int>::iterator it;
		HashMap* hashMap = new HashMap[hashTable.size()];
		int i = 0;
		

		for (it = hashTable.begin(); it != hashTable.end(); it++) {

			hashMap[i].key = it->first;
			hashMap[i].tableNum = it->second;

			i++;
		}

		fwrite((void*)hashMap, sizeof(HashMap), this->gethashTableize(), fout);
		return 0;
	}

};


class Directory {
public:
	int hashPrefix;
	vector<Bucket*> buckets;

	//증가된 hashPrefix맞게 버킷넘버 추가
	int pairIndex(int bucketNum, int hashPrefix) {
		return bucketNum ^ (1 << (hashPrefix - 1));
	}

	void grow(void) {
		for (int i = 0; i < 1 << hashPrefix ; i++)
			buckets.push_back(buckets[i]);
		hashPrefix++;
	}

	void shrink(void) {
		int flag = 1, i;

		for (i = 0; i < buckets.size(); i++) {
			if (buckets[i]->getHashPrefix() == hashPrefix) {
				flag = 0;
				return;
			}
		}

		hashPrefix--;
		for (i = 0; i < 1 << hashPrefix; i++)
			buckets.pop_back();
	}

	void split(int bucketNum) {
		int local_depth, pair_index, index_diff, dir_size, i;
		map<int, int> temp;
		map<int, int>::iterator it;

		local_depth = buckets[bucketNum]->increaseHashPrefix();
		if (local_depth > hashPrefix)
			grow();
		pair_index = pairIndex(bucketNum, local_depth);
		buckets[pair_index] = new Bucket(local_depth, BLOCKSIZE);
		temp = buckets[bucketNum]->copy();
		buckets[bucketNum]->clear();
		index_diff = 1 << local_depth;
		dir_size = 1 << hashPrefix;
		for (i = pair_index - index_diff; i >= 0; i -= index_diff)
			buckets[i] = buckets[pair_index];
		for (i = pair_index + index_diff; i < dir_size; i += index_diff)
			buckets[i] = buckets[pair_index];
		for (it = temp.begin(); it != temp.end(); it++)
			insert((*it).first, (*it).second, 1);
	}

	void merge(int bucketNum) {
		int local_hashPrefix, extendedBucketNum, index_diff, dir_size, i;

		local_hashPrefix = buckets[bucketNum]->getHashPrefix();
		extendedBucketNum = pairIndex(bucketNum, local_hashPrefix);
		index_diff = 1 << local_hashPrefix;
		dir_size = 1 << hashPrefix;

		if (buckets[extendedBucketNum]->getHashPrefix() == local_hashPrefix) {

			buckets[extendedBucketNum]->decreaseHashPrefix();
			delete(buckets[bucketNum]);
			buckets[bucketNum] = buckets[extendedBucketNum];
			for (i = bucketNum - index_diff; i >= 0; i -= index_diff)
				buckets[i] = buckets[extendedBucketNum];
			for (i = bucketNum + index_diff; i < dir_size; i += index_diff)
				buckets[i] = buckets[extendedBucketNum];
		}
	}

	string bucket_id(int n) {
		int d;
		string s;
		d = buckets[n]->getHashPrefix();
		s = "";
		while (n > 0 && d > 0) {
			s = (n % 2 == 0 ? "0" : "1") + s;
			n /= 2;
			d--;
		}
		while (d > 0) {
			s = "0" + s;
			d--;
		}
		return s;
	}


	Directory(int size) {
		this->hashPrefix = 0;
		for (int i = 0; i < 1 << this->hashPrefix; i++) {
			buckets.push_back(new Bucket(this->hashPrefix, BLOCKSIZE / size));
		}
	}

	int hash(int n) {
		return n & ((1 << hashPrefix) -1);
	}

	void insert(int key, int bucketNum, bool reinserted) {

		int cmpIdx = 0;

		if (hashPrefix == 0) {

		}
		else {

			for (int i = 0; i < hashPrefix; i++)
				cmpIdx += pow(2, i);

		}
		int status = buckets[(key & cmpIdx)]->insert(key, bucketNum);


		if (status == 0) {
			split(bucketNum);
			insert(key, bucketNum, reinserted);
		}
	}

	bool search(int key) {
		int bucketNum = hash(key);
		return buckets[bucketNum]->search(key);
	}

	void display(bool duplicates) {

	}

	int writeHashFile(FILE *& fout, bool duplicates) {
		fseek(fout, 0, SEEK_SET);
		for (int i = 0; i < buckets.size(); i++)
			buckets[i]->writeHashFile(fout);
		return 0;
	}

};



typedef struct LEAF_NODE
{
	float key[DEGREE];
	int data[DEGREE];
	struct NODE *next;
}LeafNode;

typedef struct INDEX_NODE
{
	float key[DEGREE - 1];
	struct NODE *pointer[DEGREE];
}IndexNode;

typedef struct NODE
{
	int type;
	int full;
	struct NODE *parent;
	union
	{
		LeafNode leafNode;
		IndexNode indexNode;
	}node;
}Node;

typedef struct
{
	int key;
	int tableNum;
}IndexMap;


int numOfStudentRecords = BLOCKSIZE / sizeof(Students);
int numOfProfessorRecords = BLOCKSIZE / sizeof(Professors);



Node * studentRoot;
Node * professorRoot;
Node * Stack[MAX];
int StackPoint;

void initBPlusTree(Node*& node);

void initStack();
Node * getStack();
void addStack(Node *thisNode);
// void Free(Node *thisNode);
Node * FindKey(float AddKey, int inverse, Node *thisNode);

void InsertKey(float key, int data, Node *root);
void InsertKey1(float insertKey, int insertData, Node *thisNode, Node *root);
void InsertKey2(float insertKey, int insertData, Node *thisNode, Node *root);
void InsertKey3(Node *preNode, Node *nextNode, float addkey, Node *thisNode, Node *root);
void InsertKey4(Node *preNode, Node *nextNode, float addkey, Node *thisNode, Node *root);

void DeleteKey(float deleteKey);
void DeleteKey1(Node *thisNode);
void DeleteKey2(Node *thisNode);

void SelectKey(float selectKey);

void traverse(Node*& root);
void traverse(Node*& p, int depth);



void readStudentHashFile(HashMap*& readStudentHashMap, int count) {

	FILE *readHash = fopen("Students.hash", "rb");
	fseek(readHash, 0, SEEK_SET);
	fread((void*)readStudentHashMap, sizeof(HashMap), count, readHash);

	//print out readHashMap
	for (int j = 0; j < count; j++) {
		cout << readStudentHashMap[j].key << " " << readStudentHashMap[j].tableNum << endl;
	}

}

void readProfessorHashFile(HashMap*& readProfessorHashMap, int count) {

	FILE *readHash = fopen("Professors.hash", "rb");
	fseek(readHash, 0, SEEK_SET);
	fread((void*)readProfessorHashMap, sizeof(HashMap), count, readHash);

	//print out readHashMap
	for (int j = 0; j < count; j++) {
		cout << readProfessorHashMap[j].key << " " << readProfessorHashMap[j].tableNum << endl;
	}

}

void readStudentIndexFile(Node*& score, int count) {

	FILE *readIdx = fopen("Students_score.idx", "rb");
	fseek(readIdx, 0, SEEK_SET);
	fread((void*)score, sizeof(Node), count, readIdx);
	/*
	for (int j = 0; j < count; j++) {

	//cout << score->node.leafNode.key[j] << endl;
	}*/
	traverse(score);

}

void readProfessorIndexFile(Node*& salary, int count) {

	FILE *readIdx = fopen("Professor_salary.idx", "rb");
	fseek(readIdx, 0, SEEK_SET);
	fread((void*)salary, sizeof(Node), count, readIdx);
	/*
	for (int j = 0; j < count; j++) {
	cout << salary->node.leafNode.key[j] << endl;
	}*/
	traverse(salary);

}



void insertStudentDB(StudentBlock*& blocks, int count) {
	//insert hashTable into DB as binary format
	FILE * DBFile = fopen("Students.DB", "wb");
	fseek(DBFile, 0, SEEK_SET);
	int j = fwrite((char*)blocks, sizeof(StudentBlock), count / numOfStudentRecords + 1, DBFile);
}

void insertProfessorDB(ProfessorBlock*& blocks, int count) {

	//insert hashTable into DB as binary format
	FILE * DBFile = fopen("Professors.DB", "wb");
	fseek(DBFile, 0, SEEK_SET);
	int k = fwrite((char*)blocks, sizeof(ProfessorBlock), count / numOfProfessorRecords + 2, DBFile);
}






int getStudentData(StudentBlock*& blocks, string input_str) {

	//Get input data from .csv
	ifstream input_data(input_str.c_str());
	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	tokenizer.setDelimiter(","); //parsing Delimiter = ","
	getline(input_data, buf);
	tokenizer.setString(buf);
	int count = atoi(tokenizer.next().c_str()); //the num of Students

	blocks = new StudentBlock[count / numOfStudentRecords + 1];

	//initialize blocks
	for (int j = 0; j < count / numOfStudentRecords + 1; j++) {
		for (int i = 0; i < numOfStudentRecords; i++) {
			strcpy(blocks[j].records[i].name, "");
			blocks[j].records[i].studentID = 0;
			blocks[j].records[i].score = 0;
			blocks[j].records[i].advisorID = 0;
		}
	}

	//read inputFile and then put value into blocks
	for (int j = 0; j < count / numOfStudentRecords + 1; j++) {
		for (int i = 0; i < numOfStudentRecords; i++) {

			string temp;
			getline(input_data, buf);
			tokenizer.setString(buf);
			if (input_data.eof()) break;

			strncpy(blocks[j].records[i].name, tokenizer.next().c_str(), sizeof(blocks[j].records[i].name));
			blocks[j].records[i].studentID = atoi(tokenizer.next().c_str());
			blocks[j].records[i].score = (float)atof(tokenizer.next().c_str());
			blocks[j].records[i].advisorID = atoi(tokenizer.next().c_str());
		}
	}

	return count;
}

int getProfessorData(ProfessorBlock*& blocks, string input_str) {

	//Get input data from .csv
	ifstream input_data(input_str.c_str());
	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	tokenizer.setDelimiter(","); //parsing Delimiter = ","
	getline(input_data, buf);
	tokenizer.setString(buf);
	int count = atoi(tokenizer.next().c_str()); //the num of Professors

	blocks = new ProfessorBlock[count / numOfProfessorRecords + 1];

	//initialize blocks
	for (int j = 0; j < count / numOfProfessorRecords + 1; j++) {
		for (int i = 0; i < numOfProfessorRecords; i++) {
			strcpy(blocks[j].records[i].name, "");
			blocks[j].records[i].professorID = 0;
			blocks[j].records[i].salary = 0;
		}
	}

	//read inputFile and then put value into blocks
	for (int j = 0; j < count / numOfProfessorRecords + 1; j++) {
		for (int i = 0; i < numOfProfessorRecords; i++) {

			string temp;
			getline(input_data, buf);
			tokenizer.setString(buf);
			if (input_data.eof()) break;

			strncpy(blocks[j].records[i].name, tokenizer.next().c_str(), sizeof(blocks[j].records[i].name));
			blocks[j].records[i].professorID = atoi(tokenizer.next().c_str());
			blocks[j].records[i].salary = atoi(tokenizer.next().c_str());
		}
	}

	return count;
}

int writeStudentIndexFile(FILE *& fout)
{
	fseek(fout, 0, SEEK_SET);
	Node * p = studentRoot;

	if (p->type == INDEX)
		for (; p->node.indexNode.pointer[0] != NULL; p = p->node.indexNode.pointer[0]);

	while (1)
	{
		fwrite((void*)p, sizeof(Node), 1, fout);

		if (p->node.leafNode.next != NULL)
			p = p->node.leafNode.next;
		else
			break;
	}
	return 0;
}

int writeProfessorIndexFile(FILE *& fout)
{
	fseek(fout, 0, SEEK_SET);
	Node * p = professorRoot;

	if (p->type == INDEX)
		for (; p->node.indexNode.pointer[0] != NULL; p = p->node.indexNode.pointer[0]);

	while (1)
	{
		fwrite((void*)p, sizeof(Node), 1, fout);

		if (p->node.leafNode.next != NULL)
			p = p->node.leafNode.next;
		else
			break;
	}
	return 0;
}




//해시파일
void exactSearch(Directory directory, int key, int hashMapSize, string type){

	map<int, int>::iterator it;
	int bucketNum = directory.hash(key);


	map<int, int> hashTable = directory.buckets[bucketNum]->hashTable;
	it = hashTable.find(key);

	if (it != hashTable.end()) // 찾았을 경우
	{

		printf("%d %d\n", it->first, it->second);
		FILE * fin;
		if (type == "Students") {


			for (int j = 0; j < studentCnt / numOfStudentRecords + 1; j++) {
				for (int i = 0; i < numOfStudentRecords; i++) {

					if (readStudentBlocks[j].records[i].studentID == it->first) {
						cout << readStudentBlocks[j].records[i].name << " " << readStudentBlocks[j].records[i].studentID << " "
							<< readStudentBlocks[j].records[i].score << " " << readStudentBlocks[j].records[i].advisorID << endl;
					}

				}
			}

		}
		else if (type == "Professors") {

			for (int j = 0; j < professorCnt / numOfProfessorRecords + 1; j++) {
				for (int i = 0; i < numOfProfessorRecords; i++) {

					if (readProfessorBlocks[j].records[i].professorID == it->first) {
						cout << readProfessorBlocks[j].records[i].name << " " << readProfessorBlocks[j].records[i].professorID << " "
							<< readProfessorBlocks[j].records[i].salary << endl;
					}

				}
			}

		}
	}

}


//인덱스파일
void rangeSearch(Node*& root, float from, float to, string type) {

	


}


//db파일????
void join() {



}


int getQueryData(string*& querySet) {

	ifstream input_data("query.input");
	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	tokenizer.setDelimiter(","); //parsing Delimiter = ","
	getline(input_data, buf);
	tokenizer.setString(buf);
	int count = atoi(tokenizer.next().c_str()); //the num of Students
	querySet = new string[count];
	for (int i = 0; i < count; i++) {
			if (input_data.eof()) break;
			getline(input_data, buf);
			querySet[i] = buf;
	}

	return count;
}



int main()
{
	int num;
	initBPlusTree(studentRoot);
	initBPlusTree(professorRoot);
	initStack();

	studentCnt = getStudentData(writeStudentBlocks, "student_data.csv");
	professorCnt = getProfessorData(writeProfessorBlocks, "prof_data.csv");
	insertStudentDB(writeStudentBlocks, studentCnt);
	insertProfessorDB(writeProfessorBlocks, professorCnt);

	string* querySet;
	int queryCnt = getQueryData(querySet);

	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	tokenizer.setDelimiter(", "); //parsing Delimiter = ","
	

	Directory studentDirectory(sizeof(Students)), professorDirectory(sizeof(Professors)); //hash directory initialization

	FILE *readStudentDB = fopen("Students.DB", "rb");
	fseek(readStudentDB, 0, SEEK_SET);
	readStudentBlocks = new StudentBlock[studentCnt / numOfStudentRecords + 1];

	fread((char*)readStudentBlocks, sizeof(StudentBlock), studentCnt / numOfStudentRecords + 1, readStudentDB);

	FILE *readProfessorDB = fopen("Professors.DB", "rb");
	fseek(readProfessorDB, 0, SEEK_SET);
	readProfessorBlocks = new ProfessorBlock[professorCnt / numOfProfessorRecords + 2];

	fread((char*)readProfessorBlocks, sizeof(ProfessorBlock), professorCnt / numOfProfessorRecords + 2, readProfessorDB);
	

	//studentID is key of Hash
	//insert key value into hash table
	for (int j = 0; j < studentCnt/numOfStudentRecords + 1; j++) {
		for (int i = 0; i < numOfStudentRecords; i++) {
			int hashRes = studentDirectory.hash(readStudentBlocks[j].records[i].studentID);
			studentDirectory.insert(readStudentBlocks[j].records[i].studentID, hashRes, 0);
		}
	}




	//professorID is key of Hash
	//insert key value into hash table
	for (int j = 0; j < professorCnt / numOfProfessorRecords + 1; j++) {
		for (int i = 0; i < numOfProfessorRecords; i++) {
			professorDirectory.insert(readProfessorBlocks[j].records[i].professorID, professorDirectory.hash(readProfessorBlocks[j].records[i].professorID), 0);
		}
	}


	//make Students.hash
	FILE *studentHashFile = fopen("Students.hash", "wb");
	if (studentDirectory.writeHashFile(studentHashFile, SHOW_DUPLICATE_BUCKETS) == -1)
		cout << "students.hash file error." << endl;

	

	//make Professor.hash
	FILE *professorHashFile = fopen("Professors.hash", "wb");
	if (professorDirectory.writeHashFile(professorHashFile, SHOW_DUPLICATE_BUCKETS) == -1)
		cout << "professor.hash file error." << endl;


	//score is the key of B+tree
	//insert <key, value> into index
	for (int j = 0; j < studentCnt / numOfStudentRecords + 1; j++) {
		for (int i = 0; i < numOfStudentRecords; i++) {
			InsertKey(readStudentBlocks[j].records[i].score, j, studentRoot);
		}
	}



	//salary is the key of B+tree
	//insert <key, value> into index
	for (int j = 0; j < professorCnt / numOfProfessorRecords + 2; j++) {
		for (int i = 0; i < numOfProfessorRecords; i++) {
			InsertKey(readProfessorBlocks[j].records[i].salary, j, professorRoot);
		}
	}



	//make Students.idx
	FILE *studentIndexFile = fopen("Students_score.idx", "wb");
	if (writeStudentIndexFile(studentIndexFile) == -1)
		cout << "Students.idx file error." << endl;



	//make Professors.idx
	FILE *professorIndexFile = fopen("Professor_salary.idx", "wb");
	if (writeProfessorIndexFile(professorIndexFile) == -1)
		cout << "Professor.idx file error." << endl;

	HashMap* readStudentHashMap = new HashMap[studentCnt];
	HashMap* readProfessorHashMap = new HashMap[professorCnt];
	Node* readStudentScoreIdx = new Node[DEGREE];
	Node* readProfessorSalaryIdx = new Node[DEGREE];
/*
	
	while (1) {
		cout << "Select your operation\n 1.Show Students.hash\n 2.Show all the leaves of Students_score.idx\n 3.Show Stuents.DB\n"
			<< " 4.Show Professors.hash\n 5.Show all the leaves of professor_salary.idx\n 6.Show Professor.DB\n 7.Read Query File\n 8.exit..\n>>>>>>";
		cin >> num;
		switch (num) {

		case 1:
			//read values from Students.hash
			readStudentHashFile(readStudentHashMap, studentCnt);
			break;
		case 2:
			readStudentIndexFile(readStudentScoreIdx, DEGREE);
			break;
		case 3:
			for (int j = 0; j < studentCnt / numOfStudentRecords + 1; j++) {
				for (int i = 0; i < numOfStudentRecords; i++) {
					if (strcmp(readStudentBlocks[j].records[i].name, ""))
						cout << readStudentBlocks[j].records[i].name << " " << readStudentBlocks[j].records[i].studentID << " "
						<< readStudentBlocks[j].records[i].score << " " << readStudentBlocks[j].records[i].advisorID << endl;
				}
			}
			break;

		case 4:
			//read values from Students.hash
			readProfessorHashFile(readProfessorHashMap, professorCnt);
			break;

		case 5:
			readProfessorIndexFile(readProfessorSalaryIdx, DEGREE);
			break;
		case 6:
			for (int j = 0; j < professorCnt / numOfProfessorRecords + 1; j++) {
				for (int i = 0; i < numOfProfessorRecords; i++) {
					if (strcmp(readProfessorBlocks[j].records[i].name, ""))
						cout << readProfessorBlocks[j].records[i].name << " " << readProfessorBlocks[j].records[i].professorID << " "
						<< readProfessorBlocks[j].records[i].salary << endl;
				}
			}
			break;
		case 7:*/
			for (int i = 0; i < queryCnt; i++) {
				tokenizer.setString(querySet[i]);
				if (tokenizer.next() == "Search-Exact") {
					string type = tokenizer.next();
					tokenizer.next();
					if (type == "Students") {
						exactSearch(studentDirectory, atoi(tokenizer.next().c_str()), studentCnt, "Students");
					}
					else if (type == "Professors") {
						exactSearch(professorDirectory, atoi(tokenizer.next().c_str()), professorCnt, "Professors");
					}
				}
				else if (tokenizer.next() == "Search-Range") {
					string type = tokenizer.next();
					tokenizer.next();
					if (type == "Students") {
						rangeSearch(studentRoot, atoi(tokenizer.next().c_str()), atoi(tokenizer.next().c_str()), "Students");
					}
					else if (type == "Professors") {
						rangeSearch(professorRoot, atoi(tokenizer.next().c_str()), atoi(tokenizer.next().c_str()), "Professors");
					}
				}
				else if (tokenizer.next() == "Join") {

				}
			}
		/*	break;
		case 8:
			return 0;
			break;
		default:
			cout << "Not valid operation number!\n";
		}

		cout << endl;
	}
	*/
	system("pause");
	return 0;
}

void traverse(Node*& root)
{
	traverse(root, 0);
}


void traverse(Node*& p, int depth)
{
	int i;

	printf("depth %d : ", depth);
	if (p->type == LEAF)
	{
		for (i = 0; i < DEGREE; i++)
		{
			if (p->node.indexNode.key[i] != 0)
				printf("%.6f ", p->node.leafNode.key[i]);
		}
		printf("\n");
	}
	else
	{
		for (i = 0; i < DEGREE - 1; i++)
		{
			if (p->node.indexNode.key[i] != 0)
				printf("%.6f ", p->node.indexNode.key[i]);
		}
		printf("\n");
		for (i = 0; i < DEGREE - 1; i++)
		{
			if (p->node.indexNode.pointer[i] != NULL)
			{
				traverse(p->node.indexNode.pointer[i], depth + 1);
			}
		}
		if (p->node.indexNode.pointer[i] != NULL)
		{
			traverse(p->node.indexNode.pointer[i], depth + 1);
		}
		printf("\n");
	}
}

void SelectKey(float selectKey)
{
	Node *thisNode;
	int i;
	if ((thisNode = FindKey(selectKey, 1, studentRoot)) == NULL)
	{
		printf("Key = %.1f\n", selectKey);
		puts("키가 존재하지 않습니다.\n");
		return;
	}
	// key 값과 같은 Key 가 있는지 확인
	for (i = 0; i < DEGREE; i++)
		if (fabsf(thisNode->node.leafNode.key[i] - selectKey) < EPS)
		{
			printf("Key = %.1f\n", selectKey);
			printf("Data = %d\n", thisNode->node.leafNode.data[i]);
		}
}
void InsertKey4(Node *preNode, Node *nextNode, float addkey, Node *thisNode, Node *root)
{
	/*
	InsertKey4 함수
	분할이 이루어지는 노드가 인덱스 노드 일 때 사용

	Node *preNode : 키 앞쪽에 추가할 포인터
	Node *nextNode : 키 뒤쪽에 추가할 포인터
	int addKey  : 키 번호
	Node *thisNode : 분할될 인덱스 노드
	*/
	Node *addNode;
	Node *addIndexNode;
	Node *tempPoint[DEGREE + 1];
	float tempKey[DEGREE];
	int i, j;
	// 인덱스의 포인트와 키를 복사
	for (i = 0; i < DEGREE; i++)
		tempPoint[i] = thisNode->node.indexNode.pointer[i];
	for (i = 0; i < DEGREE - 1; i++)
		tempKey[i] = thisNode->node.indexNode.key[i];
	// 추가할 key 값보다 큰 값을 찾음
	for (i = 0; i < DEGREE - 1; i++)
	{
		// 만약 같은 키가 존재하는지 확인 
		if (fabsf(tempKey[i] - addkey) < EPS) return;
		// 현재 가리키고 있는 Key 가 추가할 Key 보다 크거나 0인지 확인
		if ((tempKey[i] > addkey) || (tempKey[i] == 0)) break;
	}
	// 추가해야 할 곳이 비어있지 않을 때
	if (tempKey[i] != 0)
	{
		// 추가할 자리를 비우기위해 오른쪽으로 쉬프트
		for (j = DEGREE; j > i; j--)
			tempPoint[j] = tempPoint[j - 1];
		for (j = DEGREE - 1; j > i; j--)
			tempKey[j] = tempKey[j - 1];
	}
	tempKey[i] = addkey;
	tempPoint[i] = preNode;
	tempPoint[i + 1] = nextNode;
	// 새로운 인덱스 노드 생성
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = INDEX;
	addNode->parent = thisNode->parent;
	memset((char *)thisNode, 0, sizeof(Node));
	thisNode->type = INDEX;
	thisNode->parent = addNode->parent;
	// 반반씩 나누어 thisNode , addNode에 할당 또한 자식 노드에게 부모노드 알림
	for (i = 0; i < DEGREE / 2; i++)
	{
		thisNode->node.indexNode.pointer[i] = tempPoint[i];
		thisNode->node.indexNode.key[i] = tempKey[i];
		thisNode->node.indexNode.pointer[i]->parent = thisNode;
	}
	thisNode->node.indexNode.pointer[i] = tempPoint[i];
	thisNode->node.indexNode.pointer[i]->parent = thisNode;
	i++;
	for (j = 0; i < DEGREE; j++, i++)
	{
		addNode->node.indexNode.pointer[j] = tempPoint[i];
		addNode->node.indexNode.key[j] = tempKey[i];
		addNode->node.indexNode.pointer[j]->parent = addNode;
	}
	addNode->node.indexNode.pointer[j] = tempPoint[i];
	addNode->node.indexNode.pointer[j]->parent = addNode;

	// 부모노드가 없다면 생성
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = tempKey[DEGREE / 2];
		// 부모 노드 연결
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// 부모 노드가 있다면 InsertKey3 를 실행
		InsertKey3(thisNode, addNode, tempKey[DEGREE / 2], thisNode->parent, root);
}
void InsertKey3(Node *preNode, Node *nextNode, float addkey, Node *thisNode, Node *root)
{
	/*
	InsertKey3 함수
	인덱스 노드 이고 노드에 빈공간이 있을 때 사용

	Node *preNode : 키 앞쪽에 추가할 포인터
	Node *nextNode : 키 뒤쪽에 추가할 포인터
	int addKey  : 키 번호
	Node *thisNode : 어떤 인덱스 노드에 추가할 것인지
	*/
	int i, j;
	//thisNode 가 인덱스 노드 인지 확인
	if (thisNode->type != INDEX) return;
	// 인덱스 노드가 꽉차있다면 InsertKey4 를 실행
	if (thisNode->full == FULL)
	{
		InsertKey4(preNode, nextNode, addkey, thisNode, root);
		return;
	}
	// 추가할 key 값보다 큰 값을 찾음
	for (i = 0; i < DEGREE - 1; i++)
	{
		// 만약 같은 키가 존재하는지 확인 
		if (fabsf(thisNode->node.indexNode.key[i] - addkey) < EPS) return;
		// 현재 가리키고 있는 Key 가 추가할 Key 보다 크거나 0인지 확인
		if ((thisNode->node.indexNode.key[i] > addkey) || (thisNode->node.indexNode.key[i] == 0)) break;
	}
	// 추가해야 할 곳이 비어있지 않을 때
	if (thisNode->node.indexNode.key[i] != 0)
	{
		// 추가할 자리를 비우기위해 오른쪽으로 쉬프트
		for (j = DEGREE - 1; j > i; j--)
			thisNode->node.indexNode.pointer[j] = thisNode->node.indexNode.pointer[j - 1];
		for (j = DEGREE - 2; j > i; j--)
			thisNode->node.indexNode.key[j] = thisNode->node.indexNode.key[j - 1];
	}
	thisNode->node.indexNode.key[i] = addkey;
	thisNode->node.indexNode.pointer[i] = preNode;
	thisNode->node.indexNode.pointer[i + 1] = nextNode;
	// 추가 한 뒤 노드가 가득 차있는지 확인
	if (thisNode->node.indexNode.key[DEGREE - 2] != 0) thisNode->full = FULL;
}
void InsertKey2(float insertKey, int insertData, Node *thisNode, Node *root)
{
	/*
	InsertKey2 함수
	리프노드이고 분할 할 때 사용

	int insertKey  : 추가할 키
	int insertData  : 추가할 데이터
	Node *thisNode  : 분할 될 노드
	*/
	int i;
	Node *addNode;
	Node *addIndexNode;
	// 현재 노드가 꽉 차있는지 확인한다.
	if (thisNode->full != FULL) return;
	// 현재 노드가 리프 노드가 아니라면 종료
	if (thisNode->type != LEAF) return;
	// 하나의 리프 노드 생성 및 초기화
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = LEAF;
	// 현재 노드의 제일 마지막 부분을 새로운 노드에 삽입한다.
	InsertKey1(thisNode->node.leafNode.key[DEGREE - 1], thisNode->node.leafNode.data[DEGREE - 1], addNode, root);
	// 추가한 키와 데이터를 지우고 FULL 상태가 아니므로 0으로 바꾼다.
	thisNode->node.leafNode.key[DEGREE - 1] = 0;
	thisNode->node.leafNode.data[DEGREE - 1] = 0;
	thisNode->full = 0;
	// 현재 노드에 새로운 키를 추가 시킨다.
	InsertKey1(insertKey, insertData, thisNode, root);
	thisNode->full = FULL;
	// 현재 노드의 절반을 추가 노드에 삽입한다.
	for (i = DEGREE / 2 + 1; i < DEGREE; i++)
	{
		InsertKey1(thisNode->node.leafNode.key[i], thisNode->node.leafNode.data[i], addNode, root);

		thisNode->node.leafNode.key[i] = 0;
		thisNode->node.leafNode.data[i] = 0;
	}
	thisNode->full = 0;
	// 포인터 연결
	addNode->node.leafNode.next = thisNode->node.leafNode.next;
	thisNode->node.leafNode.next = addNode;
	addNode->parent = thisNode->parent;
	// 부모 노드가 없다면 생성
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = thisNode->node.leafNode.key[DEGREE / 2];
		// 부모 노드 연결
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// 부모 노드가 있다면 InsertKey3 를 실행
		InsertKey3(thisNode, addNode, thisNode->node.leafNode.key[DEGREE / 2], thisNode->parent, root);
}
void InsertKey1(float insertKey, int insertData, Node *thisNode, Node *root)
{
	/*
	InsertKey1 함수
	리프노드이고 노드에 빈공간이 있어서 그냥 추가할 때 사용

	int insertKey : 추가할 키
	int insertData : 추가할 데이터
	Node *thisNode : 어떤 리프 노드에 추가할 것인지
	*/
	int i, j;
	// thisNode 가 NULL 일 때 종료
	if (thisNode == NULL)
	{
		return;
	}
	// 노드가 가득 차있으면 InsertKey2 실행
	if (thisNode->full == FULL)
	{
		InsertKey2(insertKey, insertData, thisNode, root);
		return;
	}
	// 만약 노드가 리프노드가 아니라면 종료
	if (thisNode->type != LEAF) return;
	// 추가할 key 값보다 큰 값을 찾음
	for (i = 0; i < DEGREE; i++)
	{
		// 만약 같은 키가 존재하면 종료
		if (fabsf(thisNode->node.leafNode.key[i] - insertKey) < EPS) return;
		// 현재 가리키고 있는 Key 가 추가할 Key 보다 크거나 0인지 확인
		if ((thisNode->node.leafNode.key[i] > insertKey) || (thisNode->node.leafNode.key[i] == 0)) break;
	}
	// 추가할 곳이 비어있어 있지 않을 때
	if (thisNode->node.leafNode.key[i] != 0)
		// 추가할 자리를 비우기위해 오른쪽으로 쉬프트
		for (j = DEGREE - 1; j > i; j--)
		{
			thisNode->node.leafNode.key[j] = thisNode->node.leafNode.key[j - 1];
			thisNode->node.leafNode.data[j] = thisNode->node.leafNode.data[j - 1];
		}

	// 키와 데이터 추가 
	thisNode->node.leafNode.key[i] = insertKey;
	thisNode->node.leafNode.data[i] = insertData;
	// 추가 한 뒤 노드가 가득 차있는지 확인
	if (thisNode->node.leafNode.key[DEGREE - 1] != 0) thisNode->full = FULL;
}
void InsertKey(float key, int data, Node *root)
{
	//키 값은 0이 될 수 없음 0은 비어있는 키로 사용하기 때문
	if (key == 0) return;
	InsertKey1(key, data, FindKey(key, 0, root), root);
}
void initStack()
{
	// 스택 초기화
	int i;
	for (i = 0; i < MAX; i++)
		Stack[i] = NULL;
	StackPoint = 0;
}
void initBPlusTree(Node*& node)
{
	/*
	3 미만으로 하게 되면 삽입과정중
	루트가 아닌 인덱스노드의 서브트리 개수가 1개가 될 경우가 있음
	따라서 조건이 맞지 않음
	*/

	if (DEGREE < 3)
	{
		printf("차수가 3미만이 될수 없습니다.\n");
		exit(1);
	}

	// 루트 생성 및 초기화
	node = (Node *)malloc(sizeof(Node));
	memset((char *)node, 0, sizeof(Node));
	node->type = LEAF;
}
void addStack(Node *thisNode)
{
	// 스택이 가득찼다면 종료
	if (StackPoint == MAX)
	{
		printf("스택이 가득찼습니다. 스택 크기를 늘려서 다시 시도하세요 ~\n");
		return;
	}
	Stack[StackPoint++] = thisNode;
}
Node *getStack()
{
	// 스택이 비어있다면 종료
	return (StackPoint == 0) ? NULL : Stack[--StackPoint];
}
Node *FindKey(float AddKey, int inverse, Node *thisNode)
{
	/*
	node* FindKey(int AddKey)
	키가 이미 존재하는지 확인
	inverse 가 0일 때
	만약 존재한다면 NULL 을 반환
	만약 존재하지 않는다면 마지막 검사한 리프노드를 반환
	inverse 가 0이 아닐때 반대로 반환
	*/
	int i;
	while (1)
	{
		// 노드가 리프가 아닐때
		if (thisNode->type != LEAF)
		{
			// 추가할 key 값보다 큰 값을 찾음
			for (i = 0; i < DEGREE - 1; i++)
				if (thisNode->node.indexNode.key[i] >= AddKey || thisNode->node.indexNode.key[i] == 0) break;

			// thisNode 교체
			thisNode = (i == DEGREE - 1) ? thisNode->node.indexNode.pointer[DEGREE - 1] : thisNode = thisNode->node.indexNode.pointer[i];
		}
		else
		{
			// key 값과 같은 Key 가 있는지 확인
			for (i = 0; i < DEGREE; i++)
				if (fabsf(thisNode->node.leafNode.key[i] - AddKey) < EPS)
					return (inverse == 0) ? NULL : thisNode;

			return (inverse == 0) ? thisNode : NULL;
		}
	}
}
