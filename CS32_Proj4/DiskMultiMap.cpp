#include <iostream>
#include <string>
#include <functional>
#include "DiskMultiMap.h"
#include "BinaryFile.h"
//#include "MultiMapTuple.h"
using namespace std;

class MultiMapTuple;

DiskMultiMap::DiskMultiMap() {
    fileLoaded = false;
    nBuckets = -1;
}

DiskMultiMap::~DiskMultiMap() {
    DiskMultiMap::close();
}


bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets) {

    if (fileLoaded) {
        close();
    }
    
    bool success = bf.createNew(filename);
    if (!success)
        return false;
    
    // keep total size of the file, and write it at the beginning of the file
    BinaryFile::Offset size = sizeof(BinaryFile::Offset);
    success = bf.write(size, 0);
    if (!success)
        return false;
    
    // write the total number of buckets
    success = bf.write(numBuckets, size);
    if (!success)
        return false;
    size += sizeof(unsigned int);
    
    // initialize "pointers" to each bucket
    for (int i = 0; i < numBuckets; i++) {
        BinaryFile::Offset bucketOffset = -1;
        success = bf.write(bucketOffset, size);
        if (!success)
            return false;
        size += sizeof(BinaryFile::Offset);
    }
    
    // update file size
    success = bf.write(size, 0);
    if (!success)
        return false;
    
    fileLoaded = true;
    nBuckets = numBuckets;
    return true;
}


bool DiskMultiMap::openExisting(const std::string& filename) {
    if (fileLoaded) {
        close();
    }
    
    bool success = bf.openExisting(filename);
    if (!success)
        return false;
    
    fileLoaded = true;
    return true;
}


void DiskMultiMap::close() {
    if (!fileLoaded)
        return;
    
    bf.close();
    fileLoaded = false;
}

BinaryFile::Offset DiskMultiMap::getFileSize() {
    BinaryFile::Offset currentSize;
    bf.read(currentSize, 0);
    return currentSize;
}

bool DiskMultiMap::updateFileSize(size_t increaseSize) {
    bool success;
    BinaryFile::Offset currentSize;
    success = bf.read(currentSize, 0);
    if (!success)
        return false;
    
    currentSize += increaseSize;
    success = bf.write(currentSize, 0);
    if (!success)
        return false;
    
    return true;
}

bool DiskMultiMap::insert(const string& key, const string& value, const string& context) {
    
    if (!fileLoaded)
        return false;
    bool success;
    
    Node aNode = *new Node;
    strcpy(aNode.key, key.c_str());
    strcpy(aNode.value, value.c_str());
    strcpy(aNode.context, context.c_str());
    aNode.next = -1;
    
    // calculate hash value of key
    hash<string> hashFunc;
    size_t keyHash = hashFunc(key);
    int bucketPos = keyHash % nBuckets;
    
//    // write the new Node to binary file
//    BinaryFile::Offset filesize = getFileSize();
//    bf.write(aNode, filesize);
    
    
    // getting the head "pointer" to the first Node in the target bucket
    const BinaryFile::Offset bucketHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int) + sizeof(BinaryFile::Offset) * bucketPos;
    BinaryFile::Offset firstNodePos;
    bf.read(firstNodePos, bucketHead);
    
    if (firstNodePos == -1) { // currently the target bucket is empty
        
        firstNodePos = getFileSize();
        success = bf.write(firstNodePos, bucketHead);
        if (!success)
            return false;
//        cerr << "Insert a new Node in an empty bucket: Offset of the first Node updated." << endl;
        
    } else {
        
        Node storedNode;
        BinaryFile::Offset nodePos = firstNodePos;
        BinaryFile::Offset nextNodePos;
        
        // read the fist Node in the bucket
        bf.read(storedNode, nodePos);
        nextNodePos = storedNode.next;
        
        while (nextNodePos != -1) { // iterate to the last Node in the target bucket
            nodePos = nextNodePos;
            bf.read(storedNode, nodePos);
            nextNodePos = storedNode.next;
            
//            cerr << "Traversed a Node in the bucket to find the end of Nodes to insert()." << endl;
        }
        
        storedNode.next = getFileSize();
        success = bf.write(storedNode, nodePos);
        if (!success)
            return false;
//        cerr << "Inserted a new Node after previous Nodes: -next- Offset of the last Node updated." << endl;
        
    }
    
    // write the new Node to binary file
    BinaryFile::Offset filesize = getFileSize();
    success = bf.write(aNode, filesize);
    if (!success)
        return false;
    
    updateFileSize(sizeof(aNode));
    return true;
}


DiskMultiMap::Iterator DiskMultiMap::search(const string& key) {
    
    hash<string> hashFunc;
    size_t keyHash = hashFunc(key);
    int bucketPos = keyHash % nBuckets;
    
    const BinaryFile::Offset bucketHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int) + sizeof(BinaryFile::Offset) * bucketPos;
    BinaryFile::Offset firstNodePos;
    bf.read(firstNodePos, bucketHead);
    
    if (firstNodePos == -1) {
        
        Iterator it = *new Iterator();
        return it;
        
    } else {
        
        Node storedNode;
        BinaryFile::Offset nodePos = firstNodePos;
        BinaryFile::Offset nextNodePos;
        
        // examine the first Node in the target bucket
        bf.read(storedNode, nodePos);
        nextNodePos = storedNode.next;
        
        if (storedNode.key == key) {
            string key(storedNode.key);
            string value(storedNode.value);
            string context(storedNode.context);
            Iterator it = *new Iterator(bf, nodePos, key, value, context);
            
            return it;
        }
        
        // traverse later Nodes in the target bucket
        while (nextNodePos != -1) {
            nodePos = nextNodePos;
            bf.read(storedNode, nodePos);
            nextNodePos = storedNode.next;
            
            if (storedNode.key == key) {
                string key(storedNode.key);
                string value(storedNode.value);
                string context(storedNode.context);
                Iterator it = *new Iterator(bf, nodePos, key, value, context);
                
                return it;
            }
        }
        
        // no Node in the target bucket is found with the particular key; return invalid iterator
        Iterator it = *new Iterator();
        return it;
    }
    
}


// *** DiskMultiMap::Iterator *** //

DiskMultiMap::Iterator::Iterator() {
    m_state = false;
}


DiskMultiMap::Iterator::Iterator(BinaryFile& file, BinaryFile::Offset node, const string& key, const string& value, const string& context) {
    m_state = true;
    
    m_bf = &file;
    
    m_node = node;
    m_key = key;
    m_value = value;
    m_context = context;
}


DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++() {
    Node storedNode;
    BinaryFile::Offset nodePos;
    BinaryFile::Offset nextNodePos;
    
    m_bf->read(storedNode, m_node);
    nodePos = m_node;
    nextNodePos = storedNode.next;
    
    while (nextNodePos != -1) {
        nodePos = nextNodePos;
        m_node = nodePos; // update the iterator
        m_bf->read(storedNode, nodePos);
        nextNodePos = storedNode.next;
        
        if (storedNode.key == m_key) {
            string newValue(storedNode.value);
            string newContext(storedNode.context);
            
            m_value = newValue;
            m_context = newContext;
            
            return *this;
        }
    }
    
    m_state = false;
    return *this;
}


MultiMapTuple DiskMultiMap::Iterator::operator*() {
    MultiMapTuple aMapTuple = *new MultiMapTuple;
    aMapTuple.key = m_key;
    aMapTuple.value = m_value;
    aMapTuple.context = m_context;
    
    return aMapTuple;
}

