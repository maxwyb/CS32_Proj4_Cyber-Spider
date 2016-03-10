#include <iostream>
#include <vector>
#include "DiskMultiMap.h"
#include "IntelWeb.h"
#include <cassert>
using namespace std;

int main() {
    
    cout << "Running successful." << endl;
    
    // *** testing DiskMultiMap *** //
    DiskMultiMap x;
//    assert(x.openExisting("myhashtable.dat"));
    x.createNew("myhashtable.dat",100); // empty, with 100 buckets
    assert(x.insert("hmm.exe", "pfft.exe", "m52902"));
    assert(x.insert("hmm.exe", "pfft.exe", "m52902"));
    assert(x.insert("hmm.exe", "pfft.exe", "m10001"));
    assert(x.insert("blah.exe", "bletch.exe", "m0003"));
    
    // track an existing disk file
    /*
    BinaryFile::Offset pos = 0; // keep track of the current position when reading the file
    
    BinaryFile::Offset filesize;
    assert(x.bf.read(filesize, 0));
    pos += sizeof(filesize);
    
    unsigned int numOfBuckets;
    assert(x.bf.read(numOfBuckets, pos));
    pos += sizeof(numOfBuckets);
    
    BinaryFile::Offset headNodePos[1000];
    for (int i = 0; i < numOfBuckets; i++) {
        assert(x.bf.read(headNodePos[i], pos));
        pos += sizeof(BinaryFile::Offset);
    }
    
    DiskMultiMap::Node firstNode; // first Node stored in file
    assert(x.bf.read(firstNode, pos));
    pos += sizeof(firstNode);
    
    hash<string> hashFunc;
    int hash1 = hashFunc("hmm.exe") % 100;
    int hash2 = hashFunc("blah.exe") % 100;
    
    DiskMultiMap::Node storedNode11;
    assert(x.bf.read(storedNode11, headNodePos[hash1]));
    
    DiskMultiMap::Node storedNode12;
    assert(x.bf.read(storedNode12, storedNode11.next));
    
    DiskMultiMap::Node storedNode13;
    assert(x.bf.read(storedNode13, storedNode12.next));
    
    DiskMultiMap::Node storedNode2;
    assert(x.bf.read(storedNode2, headNodePos[hash2]));
    
    cout << endl;
    */
    
    DiskMultiMap::Iterator it = x.search("hmm.exe");
    if (it.isValid())
    {
        cout << "I found at least 1 item with a key of hmm.exe\n";
        do {
            MultiMapTuple m = *it; // get the association
            cout << "The key is: " << m.key << endl;
            cout << "The value is: " << m.value << endl;
            cout << "The context is: " << m.context << endl;
            cout << endl;
            ++it; // advance iterator to the next matching item
        } while (it.isValid());
    }
    
    DiskMultiMap::Iterator it2 = x.search("blah.exe");
    if (it2.isValid())
    {
        cout << "I found at least 1 item with a key of blah.exe\n";
        do {
            MultiMapTuple m = *it2; // get the association
            cout << "The key is: " << m.key << endl;
            cout << "The value is: " << m.value << endl;
            cout << "The context is: " << m.context << endl;
            cout << endl;
            ++it2; // advance iterator to the next matching item
        } while (it2.isValid());
    }
    
    DiskMultiMap::Iterator it3 = x.search("Carey-the-best.exe");
    if (!it3.isValid())
        cout << "Carey-the-best.exe not found." << endl;
    
    
    // *** test IntelWeb *** //
    bool success;
    IntelWeb aWeb;
    success = aWeb.createNew("test-data", 100);
    if (!success)
        return -1;
    aWeb.ingest("m0007 c.exe www.attacker.com");
    aWeb.ingest("m0109 c.exe b.exe");
    aWeb.ingest("m0562 a.exe b.exe");
    aWeb.ingest("m0562 c.exe www.attacker.com");
    aWeb.ingest("m1174 q.exe www.attacker.com");
    aWeb.ingest("m3455 c.exe www.google.com");
    aWeb.ingest("m3455 www.google.com a.exe");
    
    aWeb.ingest("m3455 www.google.com smallberg.exe");
    aWeb.ingest("m3455 www.google.com nachenberg.exe");
    aWeb.ingest("m3455 www.google.com foo.dll");
//    //debug 1
//    DiskMultiMap::Iterator it10 = aWeb.m_forward.search("c.exe");
//    assert(!it10.isValid());
//    
//    //debug 2
//    DiskMultiMap y;
//    y.createNew("debug-data.dat",100);
//    y.insert("c.exe", "www.attacker.exe", "m0007");
//    y.insert("c.exe", "b.exe", "m0109");
//    y.insert("a.exe", "b.exe", "m0562");
//    y.insert("c.exe", "www.attacker.com", "m0562");
//    y.insert("q.exe", "www.attacker.com", "m1174");
//    DiskMultiMap::Iterator it11 = y.search("c.exe");
//    assert(it11.isValid());
//    do {
//        MultiMapTuple m = *it11;
//        cout << "The key is: " << m.key << endl;
//        cout << "The value is: " << m.value << endl;
//        cout << "The context is: " << m.context << endl;
//        cout << endl;
//        ++it11; // advance iterator to the next matching item
//    } while (it11.isValid());
//    //end debug 2
    
    vector<string> indicators;
    indicators.push_back("c.exe");
    vector<string> badEntitiesFound;
    vector<InteractionTuple> interactions;
    
    int retVal = aWeb.crawl(indicators, 5, badEntitiesFound, interactions);
    
    cout << endl << "Bad Entities: " << endl;
    for (int i = 0; i < badEntitiesFound.size(); i++) {
        cout << badEntitiesFound[i] << endl;
    }
    cout << endl;
    for (int i = 0; i < interactions.size(); i++) {
        cout << "(" << interactions[i].from << ", " << interactions[i].to << ", " << interactions[i].context << ")" << endl;
    }
    cout << endl;
    
//    cerr << "badEntitiesFound.size() = " << badEntitiesFound.size() << endl;
    
//    DiskMultiMap::Iterator it4 = y.search("c.exe");
//    if (it4.isValid()) {
//        do {
//            MultiMapTuple m = *it4;
//            cout << "The key is: " << m.key << endl;
//            cout << "The value is: " << m.value << endl;
//            cout << "The context is: " << m.context << endl;
//            cout << endl;
//            ++it4;
//        } while (it4.isValid());
//    }
    
    return 0;
}