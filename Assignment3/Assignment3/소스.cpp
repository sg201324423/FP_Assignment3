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
#define DEGREE 510 // 3�̻����� �� ��, 510�̸� 4096Byte�� ������
#define MAX 30  // �޸� �����ϱ� ���� ����ũ�� 
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

	//������ hashPrefix�°� ��Ŷ�ѹ� �߰�
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




//�ؽ�����
void exactSearch(Directory directory, int key, int hashMapSize, string type){

	map<int, int>::iterator it;
	int bucketNum = directory.hash(key);


	map<int, int> hashTable = directory.buckets[bucketNum]->hashTable;
	it = hashTable.find(key);

	if (it != hashTable.end()) // ã���� ���
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


//�ε�������
void rangeSearch(Node*& root, float from, float to, string type) {

	


}


//db����????
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
		puts("Ű�� �������� �ʽ��ϴ�.\n");
		return;
	}
	// key ���� ���� Key �� �ִ��� Ȯ��
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
	InsertKey4 �Լ�
	������ �̷������ ��尡 �ε��� ��� �� �� ���

	Node *preNode : Ű ���ʿ� �߰��� ������
	Node *nextNode : Ű ���ʿ� �߰��� ������
	int addKey  : Ű ��ȣ
	Node *thisNode : ���ҵ� �ε��� ���
	*/
	Node *addNode;
	Node *addIndexNode;
	Node *tempPoint[DEGREE + 1];
	float tempKey[DEGREE];
	int i, j;
	// �ε����� ����Ʈ�� Ű�� ����
	for (i = 0; i < DEGREE; i++)
		tempPoint[i] = thisNode->node.indexNode.pointer[i];
	for (i = 0; i < DEGREE - 1; i++)
		tempKey[i] = thisNode->node.indexNode.key[i];
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE - 1; i++)
	{
		// ���� ���� Ű�� �����ϴ��� Ȯ�� 
		if (fabsf(tempKey[i] - addkey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((tempKey[i] > addkey) || (tempKey[i] == 0)) break;
	}
	// �߰��ؾ� �� ���� ������� ���� ��
	if (tempKey[i] != 0)
	{
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE; j > i; j--)
			tempPoint[j] = tempPoint[j - 1];
		for (j = DEGREE - 1; j > i; j--)
			tempKey[j] = tempKey[j - 1];
	}
	tempKey[i] = addkey;
	tempPoint[i] = preNode;
	tempPoint[i + 1] = nextNode;
	// ���ο� �ε��� ��� ����
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = INDEX;
	addNode->parent = thisNode->parent;
	memset((char *)thisNode, 0, sizeof(Node));
	thisNode->type = INDEX;
	thisNode->parent = addNode->parent;
	// �ݹݾ� ������ thisNode , addNode�� �Ҵ� ���� �ڽ� ��忡�� �θ��� �˸�
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

	// �θ��尡 ���ٸ� ����
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = tempKey[DEGREE / 2];
		// �θ� ��� ����
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// �θ� ��尡 �ִٸ� InsertKey3 �� ����
		InsertKey3(thisNode, addNode, tempKey[DEGREE / 2], thisNode->parent, root);
}
void InsertKey3(Node *preNode, Node *nextNode, float addkey, Node *thisNode, Node *root)
{
	/*
	InsertKey3 �Լ�
	�ε��� ��� �̰� ��忡 ������� ���� �� ���

	Node *preNode : Ű ���ʿ� �߰��� ������
	Node *nextNode : Ű ���ʿ� �߰��� ������
	int addKey  : Ű ��ȣ
	Node *thisNode : � �ε��� ��忡 �߰��� ������
	*/
	int i, j;
	//thisNode �� �ε��� ��� ���� Ȯ��
	if (thisNode->type != INDEX) return;
	// �ε��� ��尡 �����ִٸ� InsertKey4 �� ����
	if (thisNode->full == FULL)
	{
		InsertKey4(preNode, nextNode, addkey, thisNode, root);
		return;
	}
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE - 1; i++)
	{
		// ���� ���� Ű�� �����ϴ��� Ȯ�� 
		if (fabsf(thisNode->node.indexNode.key[i] - addkey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((thisNode->node.indexNode.key[i] > addkey) || (thisNode->node.indexNode.key[i] == 0)) break;
	}
	// �߰��ؾ� �� ���� ������� ���� ��
	if (thisNode->node.indexNode.key[i] != 0)
	{
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE - 1; j > i; j--)
			thisNode->node.indexNode.pointer[j] = thisNode->node.indexNode.pointer[j - 1];
		for (j = DEGREE - 2; j > i; j--)
			thisNode->node.indexNode.key[j] = thisNode->node.indexNode.key[j - 1];
	}
	thisNode->node.indexNode.key[i] = addkey;
	thisNode->node.indexNode.pointer[i] = preNode;
	thisNode->node.indexNode.pointer[i + 1] = nextNode;
	// �߰� �� �� ��尡 ���� ���ִ��� Ȯ��
	if (thisNode->node.indexNode.key[DEGREE - 2] != 0) thisNode->full = FULL;
}
void InsertKey2(float insertKey, int insertData, Node *thisNode, Node *root)
{
	/*
	InsertKey2 �Լ�
	��������̰� ���� �� �� ���

	int insertKey  : �߰��� Ű
	int insertData  : �߰��� ������
	Node *thisNode  : ���� �� ���
	*/
	int i;
	Node *addNode;
	Node *addIndexNode;
	// ���� ��尡 �� ���ִ��� Ȯ���Ѵ�.
	if (thisNode->full != FULL) return;
	// ���� ��尡 ���� ��尡 �ƴ϶�� ����
	if (thisNode->type != LEAF) return;
	// �ϳ��� ���� ��� ���� �� �ʱ�ȭ
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = LEAF;
	// ���� ����� ���� ������ �κ��� ���ο� ��忡 �����Ѵ�.
	InsertKey1(thisNode->node.leafNode.key[DEGREE - 1], thisNode->node.leafNode.data[DEGREE - 1], addNode, root);
	// �߰��� Ű�� �����͸� ����� FULL ���°� �ƴϹǷ� 0���� �ٲ۴�.
	thisNode->node.leafNode.key[DEGREE - 1] = 0;
	thisNode->node.leafNode.data[DEGREE - 1] = 0;
	thisNode->full = 0;
	// ���� ��忡 ���ο� Ű�� �߰� ��Ų��.
	InsertKey1(insertKey, insertData, thisNode, root);
	thisNode->full = FULL;
	// ���� ����� ������ �߰� ��忡 �����Ѵ�.
	for (i = DEGREE / 2 + 1; i < DEGREE; i++)
	{
		InsertKey1(thisNode->node.leafNode.key[i], thisNode->node.leafNode.data[i], addNode, root);

		thisNode->node.leafNode.key[i] = 0;
		thisNode->node.leafNode.data[i] = 0;
	}
	thisNode->full = 0;
	// ������ ����
	addNode->node.leafNode.next = thisNode->node.leafNode.next;
	thisNode->node.leafNode.next = addNode;
	addNode->parent = thisNode->parent;
	// �θ� ��尡 ���ٸ� ����
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = thisNode->node.leafNode.key[DEGREE / 2];
		// �θ� ��� ����
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// �θ� ��尡 �ִٸ� InsertKey3 �� ����
		InsertKey3(thisNode, addNode, thisNode->node.leafNode.key[DEGREE / 2], thisNode->parent, root);
}
void InsertKey1(float insertKey, int insertData, Node *thisNode, Node *root)
{
	/*
	InsertKey1 �Լ�
	��������̰� ��忡 ������� �־ �׳� �߰��� �� ���

	int insertKey : �߰��� Ű
	int insertData : �߰��� ������
	Node *thisNode : � ���� ��忡 �߰��� ������
	*/
	int i, j;
	// thisNode �� NULL �� �� ����
	if (thisNode == NULL)
	{
		return;
	}
	// ��尡 ���� �������� InsertKey2 ����
	if (thisNode->full == FULL)
	{
		InsertKey2(insertKey, insertData, thisNode, root);
		return;
	}
	// ���� ��尡 ������尡 �ƴ϶�� ����
	if (thisNode->type != LEAF) return;
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE; i++)
	{
		// ���� ���� Ű�� �����ϸ� ����
		if (fabsf(thisNode->node.leafNode.key[i] - insertKey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((thisNode->node.leafNode.key[i] > insertKey) || (thisNode->node.leafNode.key[i] == 0)) break;
	}
	// �߰��� ���� ����־� ���� ���� ��
	if (thisNode->node.leafNode.key[i] != 0)
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE - 1; j > i; j--)
		{
			thisNode->node.leafNode.key[j] = thisNode->node.leafNode.key[j - 1];
			thisNode->node.leafNode.data[j] = thisNode->node.leafNode.data[j - 1];
		}

	// Ű�� ������ �߰� 
	thisNode->node.leafNode.key[i] = insertKey;
	thisNode->node.leafNode.data[i] = insertData;
	// �߰� �� �� ��尡 ���� ���ִ��� Ȯ��
	if (thisNode->node.leafNode.key[DEGREE - 1] != 0) thisNode->full = FULL;
}
void InsertKey(float key, int data, Node *root)
{
	//Ű ���� 0�� �� �� ���� 0�� ����ִ� Ű�� ����ϱ� ����
	if (key == 0) return;
	InsertKey1(key, data, FindKey(key, 0, root), root);
}
void initStack()
{
	// ���� �ʱ�ȭ
	int i;
	for (i = 0; i < MAX; i++)
		Stack[i] = NULL;
	StackPoint = 0;
}
void initBPlusTree(Node*& node)
{
	/*
	3 �̸����� �ϰ� �Ǹ� ���԰�����
	��Ʈ�� �ƴ� �ε�������� ����Ʈ�� ������ 1���� �� ��찡 ����
	���� ������ ���� ����
	*/

	if (DEGREE < 3)
	{
		printf("������ 3�̸��� �ɼ� �����ϴ�.\n");
		exit(1);
	}

	// ��Ʈ ���� �� �ʱ�ȭ
	node = (Node *)malloc(sizeof(Node));
	memset((char *)node, 0, sizeof(Node));
	node->type = LEAF;
}
void addStack(Node *thisNode)
{
	// ������ ����á�ٸ� ����
	if (StackPoint == MAX)
	{
		printf("������ ����á���ϴ�. ���� ũ�⸦ �÷��� �ٽ� �õ��ϼ��� ~\n");
		return;
	}
	Stack[StackPoint++] = thisNode;
}
Node *getStack()
{
	// ������ ����ִٸ� ����
	return (StackPoint == 0) ? NULL : Stack[--StackPoint];
}
Node *FindKey(float AddKey, int inverse, Node *thisNode)
{
	/*
	node* FindKey(int AddKey)
	Ű�� �̹� �����ϴ��� Ȯ��
	inverse �� 0�� ��
	���� �����Ѵٸ� NULL �� ��ȯ
	���� �������� �ʴ´ٸ� ������ �˻��� ������带 ��ȯ
	inverse �� 0�� �ƴҶ� �ݴ�� ��ȯ
	*/
	int i;
	while (1)
	{
		// ��尡 ������ �ƴҶ�
		if (thisNode->type != LEAF)
		{
			// �߰��� key ������ ū ���� ã��
			for (i = 0; i < DEGREE - 1; i++)
				if (thisNode->node.indexNode.key[i] >= AddKey || thisNode->node.indexNode.key[i] == 0) break;

			// thisNode ��ü
			thisNode = (i == DEGREE - 1) ? thisNode->node.indexNode.pointer[DEGREE - 1] : thisNode = thisNode->node.indexNode.pointer[i];
		}
		else
		{
			// key ���� ���� Key �� �ִ��� Ȯ��
			for (i = 0; i < DEGREE; i++)
				if (fabsf(thisNode->node.leafNode.key[i] - AddKey) < EPS)
					return (inverse == 0) ? NULL : thisNode;

			return (inverse == 0) ? thisNode : NULL;
		}
	}
}
