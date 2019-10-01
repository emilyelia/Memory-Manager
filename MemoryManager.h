#include <list>
#pragma once

#include <functional>
#include <memory>
#include <iostream>
#include <fstream>

struct Node
{
    int start;
    int length;
    char* memAddress;
    bool isHole;
};
typedef struct Node Node;

class MemoryManager
{
public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    ~MemoryManager();

    void initialize(size_t sizeInWords);
    void shutdown();

    void* allocate(size_t sizeInBytes);
    void free(void* address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);

    void* getBitmap();
    unsigned int getWordSize();
    void* getMemoryStart();
    unsigned int getMemoryLimit();
    void *getList();
    void printList();

private:
    char * memMap;
    unsigned sizeWord;
    size_t sizeInWords;
    std::list<Node> *mapList;
    std::function<int(int, void *)> alloc;
    Node newNode(int start,int length,char *memAddress,bool isHole);
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);
