#include <iostream>
#include <string>
#include <vector>
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
    
    // write the "pointer" to the first Delete struct
    BinaryFile::Offset firstDeletePos = -1;
    success = bf.write(firstDeletePos, size);
    if (!success)
        return false;
    size += sizeof(BinaryFile::Offset);
    
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
    if (key.length() > 120 || value.length() > 120 || context.length() > 120)
        return false;
    
    bool success;
    Node aNode = *new Node;
    strcpy(aNode.key, key.c_str());
    strcpy(aNode.value, value.c_str());
    strcpy(aNode.context, context.c_str());
    aNode.next = -1;
    
    // get the storing position of the new Node, by finding if there are any places ready to be overwritten. If not, store it at the end of the file
    BinaryFile::Offset storingPos = getFileSize();
    
    const BinaryFile::Offset deleteHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int);
    BinaryFile::Offset firstDeletePos;
    bf.read(firstDeletePos, deleteHead);
    
    if (firstDeletePos != -1) { // some Deletion are previously stored in the file
        
        Deletion storedDeletion;
        BinaryFile::Offset deletePos = firstDeletePos;
        BinaryFile::Offset nextDeletePos;
        
        bf.read(storedDeletion, deletePos);
        nextDeletePos = storedDeletion.next;
        
        bool foundPlace = false;
        if (storedDeletion.canBeOverwritten) {
            storingPos = storedDeletion.pos;
            foundPlace = true;
            
            storedDeletion.canBeOverwritten = false;
            bf.write(storedDeletion, deletePos);
        }
        
        while (nextDeletePos != -1 && !foundPlace) { // traverse to the last Deletion in the file
            deletePos = nextDeletePos;
            bf.read(storedDeletion, deletePos);
            nextDeletePos = storedDeletion.next;
            
            if (storedDeletion.canBeOverwritten) {
                storingPos = storedDeletion.pos;
                foundPlace = true;
                
                storedDeletion.canBeOverwritten = false;
                bf.write(storedDeletion, deletePos);
                break;
            }
        }
    }
    
    // write the new Node to binary file, and update file size if needed
    success = bf.write(aNode, storingPos);
    if (!success)
        return false;
    if (storingPos == getFileSize())
        updateFileSize(sizeof(aNode));
    

    // update the -next- Offset of the last Node currently stored in the target bucket
    // calculate hash value of key
    hash<string> hashFunc;
    size_t keyHash = hashFunc(key);
    int bucketPos = keyHash % nBuckets;
    
    // getting the head "pointer" to the first Node in the target bucket
    const BinaryFile::Offset bucketHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int) + sizeof(BinaryFile::Offset) + sizeof(BinaryFile::Offset) * bucketPos;
    BinaryFile::Offset firstNodePos;
    bf.read(firstNodePos, bucketHead);
    
    if (firstNodePos == -1) { // currently the target bucket is empty
        
        firstNodePos = storingPos;
        success = bf.write(firstNodePos, bucketHead);
        if (!success)
            return false;
        
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
        }
        
        // update the -next- Offset of the currently stored Node
        storedNode.next = storingPos;
        success = bf.write(storedNode, nodePos);
        if (!success)
            return false;
    }
    
    return true;
}


DiskMultiMap::Iterator DiskMultiMap::search(const string& key) {
    
    hash<string> hashFunc;
    size_t keyHash = hashFunc(key);
    int bucketPos = keyHash % nBuckets;
    
    const BinaryFile::Offset bucketHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int) + sizeof(BinaryFile::Offset) + sizeof(BinaryFile::Offset) * bucketPos;
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


void DiskMultiMap::searchForOffset(const string& key, const string& value, const string& context, vector<BinaryFile::Offset>& offsets) {
    
    offsets.clear();
    
    hash<string> hashFunc;
    size_t keyHash = hashFunc(key);
    int bucketPos = keyHash % nBuckets;
    
    const BinaryFile::Offset bucketHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int) + sizeof(BinaryFile::Offset) + sizeof(BinaryFile::Offset) * bucketPos;
    BinaryFile::Offset firstNodePos;
    bf.read(firstNodePos, bucketHead);
    
    if (firstNodePos == -1) { // no Node is the target bucket, so there is nothing to delete
        return;
        
    } else {
        
        Node storedNode;
        BinaryFile::Offset nodePos = firstNodePos;
        BinaryFile::Offset nextNodePos;
        
        // examine the first Node in the target bucket
        bf.read(storedNode, nodePos);
        nextNodePos = storedNode.next;
        
        if (storedNode.key == key) {
            if (storedNode.value == value && storedNode.context == context) {
                offsets.push_back(nodePos);
                
                // direct the -next- Offset of the previous Node to the next Node
                // the deleted Node MUST BE the first Node in the bucket
                firstNodePos = storedNode.next;
                bf.write(firstNodePos, bucketHead);
                    
            }
        }
        
        // traverse later Nodes in the target bucket
        while (nextNodePos != -1) {
            nodePos = nextNodePos;
            bf.read(storedNode, nodePos);
            nextNodePos = storedNode.next;
            
            if (storedNode.key == key) {
                if (storedNode.value == value && storedNode.context == context) {
                    offsets.push_back(nodePos);
                    // direct the -next- Offset of the previous Node to the next Node
                    
                    // the deleted Node CAN BE the fist Node in the bucket after the previous deletion, if any
                    if (nodePos == firstNodePos) {
                        
                        firstNodePos = storedNode.next;
                        bf.write(firstNodePos, bucketHead);
                        
                    } else {  // it can also be the second or later Node in the bucket after the previous deletion, if any
                        Node temp_storedNode;
                        BinaryFile::Offset temp_nodePos = firstNodePos;
                        BinaryFile::Offset temp_nextNodePos;
                    
                        bf.read(temp_storedNode, temp_nodePos);
                        temp_nextNodePos = temp_storedNode.next;
                        bool processed = false;
                        if (nodePos == temp_storedNode.next) { // if the first Node in the bucket is the previous Node to the deleted Node
                            temp_storedNode.next = storedNode.next;
                            bf.write(temp_storedNode, temp_nodePos);
                            processed = true;
                        }
                    
                        while (temp_nextNodePos != -1 && !processed) { // traverse to later Nodes in the bucket
                            temp_nodePos = temp_nextNodePos;
                            bf.read(temp_storedNode, temp_nodePos);
                            temp_nextNodePos = temp_storedNode.next;
                            
                            if (nodePos == temp_storedNode.next) {
                                temp_storedNode.next = storedNode.next;
                                bf.write(temp_storedNode, temp_nodePos);
                                break;
                            }
                        }
                    }

                }
            }
        }
        return;
    }
    
}


bool DiskMultiMap::deleteNode(vector<BinaryFile::Offset> offsets) {
    
    if (offsets.empty())
        return true;
    
    BinaryFile::Offset filesize = getFileSize();
    bool success;
    
    const BinaryFile::Offset deleteHead = sizeof(BinaryFile::Offset) + sizeof(unsigned int);
    BinaryFile::Offset firstDeletePos;
    success = bf.read(firstDeletePos, deleteHead);
    if (!success)
        return false;
    
    if (firstDeletePos == -1) { // no Deletion previously stored in the file
        bf.write(filesize, deleteHead);
        
    } else { // some Deletion are previously stored in the file
        
        Deletion storedDeletion;
        BinaryFile::Offset deletePos = firstDeletePos;
        BinaryFile::Offset nextDeletePos;
        
        bf.read(storedDeletion, deletePos);
        nextDeletePos = storedDeletion.next;
        
        while (nextDeletePos != -1) { // traverse to the last Deletion in the file
            deletePos = nextDeletePos;
            bf.read(storedDeletion, deletePos);
            nextDeletePos = storedDeletion.next;
        }
        
        storedDeletion.next = filesize; // update the last Deletion already stored in the file
        bf.write(storedDeletion, deletePos);
        
    }
    
    // store new Deletions to the file
    for (int i = 0; i < offsets.size() - 1; i++) { // there are more than one Node to delete
        Deletion aDelete;
        aDelete.pos = offsets[i];
        aDelete.canBeOverwritten = true;
//        aDelete.next = offsets[i+1];
        aDelete.next = filesize + sizeof(Deletion);
        
        bf.write(aDelete, filesize);
        filesize += sizeof(Deletion);
        updateFileSize(sizeof(Deletion));
    }
    Deletion aDelete;
    aDelete.pos = offsets[offsets.size()-1];
    aDelete.canBeOverwritten = true;
    aDelete.next = -1;
    
    bf.write(aDelete, filesize);
    filesize += sizeof(Deletion);
    updateFileSize(sizeof(Deletion));
    
    return true;
}

int DiskMultiMap::erase(const string& key, const string& value, const string& context) {
    
    vector<BinaryFile::Offset> offsets;
    
    searchForOffset(key, value, context, offsets);
    deleteNode(offsets);
    
    return int(offsets.size());
    
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

