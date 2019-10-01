
#include "MemoryManager.h"

using namespace std;

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
{
    sizeWord = wordSize;
    alloc = allocator;
    memMap = nullptr;
}

MemoryManager::~MemoryManager()
{
    this->shutdown();
}

void MemoryManager::setAllocator(std::function<int(int, void *)> allocator)
{
    alloc = allocator;
}

void MemoryManager::shutdown()
{
    if (memMap != nullptr)
        delete[] memMap;
    this->mapList->clear();
    delete mapList;
    memMap = nullptr;
}
Node MemoryManager::newNode(int start, int length, char* memAddress, bool isHole)
{
    Node nodeNew;
    nodeNew.start = start;
    nodeNew.length = length;
    nodeNew.memAddress = memAddress;
    nodeNew.isHole = isHole;
    //std::cout << start << " " << length << " " << memAddress << " " << isHole << std::endl;
    return nodeNew;
}

void  MemoryManager::initialize(size_t sizeInWords)
{
    if (memMap != nullptr)
    {
        delete[] memMap;
    }
    //multiply value passed times valuse in theconstructor
    size_t toInitalize = sizeInWords*sizeWord;
    memMap = new char[toInitalize];
    mapList = new std::list<Node>();
    this->sizeInWords = sizeInWords;
    mapList->push_front(newNode(0, sizeInWords, (char *)this->memMap, true));
}

void* MemoryManager::allocate(size_t sizeInBytes)
{
    if (sizeInBytes <= 0)
    {
        return nullptr;
    }
    size_t wordSize = getWordSize();
    size_t lengthReq = sizeInBytes / wordSize;
    if (sizeInBytes % wordSize != 0)
        lengthReq += 1;
    
    int startOffset = alloc(lengthReq, mapList);
    if (startOffset == -1)
        return nullptr;
    
    void* start = getMemoryStart();
    
    std::list<Node>::iterator it = mapList->begin(),nxt;
    while (it != mapList->end())
    {
        if (it->start == startOffset)
        {
            if (lengthReq != it->length)
            {
                nxt = ++it;
                --it;
                mapList->insert(nxt, newNode(
                                             it->start + lengthReq,
                                             it->length - lengthReq,
                                             it->memAddress + (wordSize*lengthReq),
                                             true));
                it->length = lengthReq;
            }
            
            it->isHole = false;
            return (it->memAddress);
        }
        
        ++it;
    }
    return nullptr;
}

void MemoryManager::free(void* address)
{
    void* start = getMemoryStart();
    std::list<Node>::iterator it = mapList->begin(),og;
    while(it != mapList->end()){
        //cout << it->memAddress << " " << address << endl;
        if (it->memAddress == address)
        {
            og = it;
            if (it != mapList->begin())
            {
                --it;
                if (it->isHole)
                {
                    it->length = it->length + og->length;
                    mapList->erase(og);
                }
                else
                {
                    ++it;
                }
            }
            
            og = ++it;
            --it;
            if (og != mapList->end() && og->isHole)
            {
                it->length = it->length + og->length;
                mapList->erase(og);
            }
            
            it->isHole = true;
            break;
        }
        ++it;
    }
    
}

unsigned MemoryManager::getWordSize()
{
    return this->sizeWord;
}

void *MemoryManager::getMemoryStart()
{
    return this->memMap;
}

unsigned int MemoryManager::getMemoryLimit()
{
    return (this->sizeWord)*(this->sizeInWords);
}

int MemoryManager::dumpMemoryMap(char *filename)
{
    ofstream myfile;
    myfile.open(filename);
    if(myfile.is_open())
    {
        int cnt=0,idx=0;
        for(auto it = mapList->begin(); it != mapList->end(); ++it)
        {
            if (it->isHole)
                cnt++;
        }
        for(auto it = mapList->begin(); it != mapList->end(); ++it)
        {
            if (it->isHole)
            {
                myfile << "[" << it->start << ", " << it->length << "]";
                idx++;
                if (idx != cnt)
                    myfile << " - ";
            }
        }
        myfile.close();
        return 0;
    }
    return 1;
    
}

void* MemoryManager::getBitmap()
{
    uint8_t* bitMap;
    int bitMapLen = (sizeInWords/8);
    if(sizeInWords % 8 != 0)
    {
        bitMapLen++;
    }
    bitMap = new uint8_t[bitMapLen + 2];
    
    bitMap[0] = bitMapLen % 128;
    bitMap[1] = bitMapLen / 128;
    int idx = 2,cnt = 0, tp = 0;
    for(auto it = mapList->begin(); it != mapList->end(); ++it)
    {
        for(int i=0; i< it->length; i++)
        {
            if (cnt == 8)
            {
                bitMap[idx++] = (uint8_t)tp;
                //cout << "tp: " << tp << endl;
                tp = 0;
                cnt = 0;
            }
            if (!it->isHole)
                tp = (tp | 1 << (cnt));
            //cout << cnt << " " << it->isHole << " " << idx << " " << tp << endl;
            cnt++;
        }
    }
    
    return bitMap;
}

void* MemoryManager::getList()
{
    uint16_t* bitList;
    int idx = 1,holeCnt=0;
    for(auto it = mapList->begin(); it != mapList->end(); ++it)
    {
        if(it->isHole)
        {
            holeCnt++;
        }
    }
    bitList = new uint16_t[holeCnt + 1];
    for(auto it = mapList->begin(); it != mapList->end(); ++it)
    {
        if(it->isHole)
        {
            bitList[idx++] = it->start;
            bitList[idx++] = it->length;
        }
    }
    bitList[0] = holeCnt*4;
    return bitList;
}

void MemoryManager::printList()
{
    cout << "Size of List: " << mapList->size() << endl;
    for(auto it = mapList->begin(); it != mapList->end(); ++it)
    {
        std::cout << it->start << " " << it->length << "  " << it->isHole << " " << it->memAddress << endl;
    }
}

int bestFit(int sizeInWords, void *list)
{
    std::list<Node>* listPtr = reinterpret_cast<std::list<Node>* >(list);
    auto it = listPtr->begin();
    int minDiff = -1,mnStartOffset = -1;
    
    while (it != listPtr->end())
    {
        int holeDiff = it->length - sizeInWords;
        if (it->isHole)
        {
            if (holeDiff >=0 && (minDiff == -1 || holeDiff < minDiff) )
            {
                minDiff = holeDiff;
                mnStartOffset = it->start;
            }
        }
        
        ++it;
    }
    
    return mnStartOffset;
}

int worstFit(int sizeInWords, void *list)
{
    std::list<Node>* listPtr = reinterpret_cast<std::list<Node>* >(list);
    std::list<Node>::iterator it = listPtr->begin();
    int maxDiff = -1,maxStartOffset = -1;
    
    while (it != listPtr->end())
    {
        if (it->isHole)
        {
            int holeDiff = it->length - sizeInWords;
            if (holeDiff >=0 && (maxStartOffset == -1 || holeDiff > maxDiff) )
            {
                maxDiff = holeDiff;
                maxStartOffset = it->start;
            }
        }
        
        ++it;
    }
    
    return maxStartOffset;
}

/*
 int main()
 {
 int wordSize = 4;
 MemoryManager memoryManager(wordSize, bestFit);
 memoryManager.initialize(26);
 memoryManager.printList();
 void* testA = memoryManager.allocate(wordSize * 10);
 cout << testA << endl;
 memoryManager.printList();
 void* testB = memoryManager.allocate(wordSize * 2);
 memoryManager.printList();
 void* testC = memoryManager.allocate(wordSize * 2);
 memoryManager.printList();
 void* testD = memoryManager.allocate(wordSize * 6);
 cout << testA << endl;
 memoryManager.printList();
 memoryManager.free(testA);
 memoryManager.printList();
 memoryManager.free(testC);
 memoryManager.printList();
 //uint8_t* bitmap = (uint8_t*) memoryManager.getBitmap();
 //uint8_t* list = (uint8_t*) memoryManager.getList();
 //prettyPrintBitmap(bitmap);
 //prettyPrintList(list);
 
 memoryManager.free(testB);
 memoryManager.printList();
 memoryManager.free(testD);
 memoryManager.printList();
 //delete bitmap;
 //delete list;
 return 0;
 }
 */
