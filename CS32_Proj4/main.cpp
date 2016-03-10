#include <iostream>
#include <vector>
#include "DiskMultiMap.h"
#include "IntelWeb.h"
#include <cassert>
using namespace std;

int main() {
    
    // *** testing DiskMultiMap *** //
    DiskMultiMap x;
//    assert(x.openExisting("myhashtable.dat"));
    x.createNew("myhashtable.dat",100);
    assert(x.insert("hmm.exe", "pfft.exe", "m52902"));
    assert(x.insert("hmm.exe", "pfft.exe", "m52902"));
    assert(x.insert("hmm.exe", "pfft.exe", "m10001"));
    assert(x.insert("blah.exe", "bletch.exe", "m0003"));
    assert(x.insert("hmm.exe", "smallberg.exe", "m10002"));
    assert(x.insert("hmm.exe", "nachenberg.exe", "AlphaGo"));
    {
        DiskMultiMap::Iterator it = x.search("hmm.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "hmm.exe not found." << endl;
        }
    }
    {
        DiskMultiMap::Iterator it = x.search("blah.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "blah.exe not found." << endl;
        }
    }
    
    cout << "----------" << endl;
    assert(x.erase("hmm.exe", "pfft.exe", "m52902") == 2);
    x.erase("hmm.exe", "smallberg.exe", "m10002");
    x.erase("blah.exe", "bletch.exe", "m0003");
    {
        DiskMultiMap::Iterator it = x.search("hmm.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "hmm.exe not found." << endl;
        }
    }
    {
        DiskMultiMap::Iterator it = x.search("blah.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "blah.exe not found." << endl;
        }
    }
    
    cout << "----------" << endl;
    x.insert("trojan.exe", "www.0day.com", "DeepBlue");
    x.insert("trojan.exe", "CodeDayLA.org", "m20160101");
    {
        DiskMultiMap::Iterator it = x.search("trojan.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "trojan.exe not found." << endl;
        }
    }
    {
        DiskMultiMap::Iterator it = x.search("hmm.exe");
        if (it.isValid())
        {
            do {
                MultiMapTuple m = *it; // get the association
                cout << "(" << m.key << ", " << m.value << ", " << m.context << ")" << endl;
                ++it; // advance iterator to the next matching item
            } while (it.isValid());
        } else {
            cout << "hmm.exe not found." << endl;
        }
    }
    
    assert(x.erase("not-there", "something-else", "ah-oh") == 0);
    
    // *** test IntelWeb *** //
    cout << "----------" << endl;
    IntelWeb aWeb;
    aWeb.createNew("test-data", 100);

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
    
    {
        vector<string> indicators;
        indicators.push_back("c.exe");
        vector<string> badEntitiesFound;
        vector<InteractionTuple> interactions;
    
        assert(aWeb.crawl(indicators, 5, badEntitiesFound, interactions) == 4);
    
        cout << endl << "---Bad Entities: ---" << endl;
        for (int i = 0; i < badEntitiesFound.size(); i++) {
            cout << badEntitiesFound[i] << endl;
        }
        cout << endl << "---Interactions: ---" << endl;
        for (int i = 0; i < interactions.size(); i++) {
            cout << "(" << interactions[i].from << ", " << interactions[i].to << ", " << interactions[i].context << ")" << endl;
        }
        cout << endl;
    }
    
    cout << "----------" << endl;
    aWeb.purge("q.exe");
    {
        vector<string> indicators;
        indicators.push_back("c.exe");
        vector<string> badEntitiesFound;
        vector<InteractionTuple> interactions;
        
        assert(aWeb.crawl(indicators, 5, badEntitiesFound, interactions) == 3);
        
        cout << endl << "---Bad Entities: ---" << endl;
        for (int i = 0; i < badEntitiesFound.size(); i++) {
            cout << badEntitiesFound[i] << endl;
        }
        cout << endl << "---Interactions: ---" << endl;
        for (int i = 0; i < interactions.size(); i++) {
            cout << "(" << interactions[i].from << ", " << interactions[i].to << ", " << interactions[i].context << ")" << endl;
        }
        cout << endl;
    }
    
    return 0;
    
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
    
}