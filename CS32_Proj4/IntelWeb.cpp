#include <iostream>
#include <vector>
#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include "InteractionTuple.h"
#include <cassert> //debug
using namespace std;

IntelWeb::IntelWeb() : m_forward(), m_reverse() {
    m_fileLoaded = false;
}

IntelWeb::~IntelWeb() {
    close();
}

bool IntelWeb::createNew(const string& filePrefix, unsigned int maxDataItems) {
    
    if (m_fileLoaded) {
        close();
    }
    
    string forwardName = filePrefix + "-forward.dat";
    string reverseName = filePrefix + "-reverse.dat";
    
    const double loadFactor = 0.75;
    int nBuckets = maxDataItems / loadFactor;
    
    
    bool success;
    success = m_forward.createNew(forwardName, nBuckets);
    if (!success)
        return false;
    success = m_reverse.createNew(reverseName, nBuckets);
    if (!success)
        return false;
    
    m_fileLoaded = true;
    return true;
}

bool IntelWeb::openExisting(const string& filePrefix) {
    if (m_fileLoaded) {
        close();
    }
    
    string forwardName = filePrefix + "-forward.dat";
    string reverseName = filePrefix + "-reverse.dat";
    
    bool success;
    success = m_forward.openExisting(forwardName);
    if (!success)
        return false;
    success = m_reverse.openExisting(reverseName);
    if (!success)
        return false;
    
    m_fileLoaded = true;
    return true;
}

void IntelWeb::close() {
    if (m_fileLoaded) {
        m_forward.close();
        m_reverse.close();
    }
    m_fileLoaded = false;
}

bool IntelWeb::ingest(const string& telemetryFile) {
    
    // parse the telemetry line into key, value and context fields
    string aLog[3] = {"", "", ""};
    int j = 0;
    for (int i = 0; i < telemetryFile.length(); i++) {
        if (telemetryFile[i] == ' ') {
            j++;
            continue;
        }
        if (j > 2) {
            cout << "Invalid telemetry log line." << endl;
            return false;
        }
        aLog[j] += telemetryFile[i];
    }
    
    cerr << "String parse: " << aLog[0] << "; " << aLog[1] << "; " << aLog[2] << endl;
    
    bool success;
//    const string key = aLog[1];
//    const string value = aLog[2];
//    const string context = aLog[0];
    success = m_forward.insert(aLog[1], aLog[2], aLog[0]);
//    assert(aLog[1] == "c.exe" && aLog[2] == "www.attacker.com" && aLog[0] == "m0007");
//    success = m_forward.insert("c.exe", "www.attacker.com", "m0007");
//    cerr << "m_forward.insert(" << aLog[1] << ", " << aLog[2] << ", " << aLog[0] << ");" << endl;
    if (!success)
        return false;
    success = m_reverse.insert(aLog[2], aLog[1], aLog[0]);
    if (!success)
        return false;
    
//    //debug
//    DiskMultiMap::Iterator it = m_forward.search("c.exe");
//    if (it.isValid())
//        cout << "c.exe is stored in DiskMultiMap." << endl;
    
//    m_forward.insert("c.exe", "www.attacker.exe", "m0007");
    
    return true;
}

////debug
//void IntelWeb::aTestFunc() {
//    DiskMultiMap::Iterator it = m_forward.search("c.exe");
//    if (it.isValid())
//        cout << "c.exe is stored in DiskMultiMap." << endl;
//}

unsigned int IntelWeb::crawl(const vector<string>& indicators, unsigned int minPrevalenceToBeGood, vector<string>& badEntitiesFound, vector<InteractionTuple>& interactions) {
    
//    //debug
//    DiskMultiMap::Iterator it = m_forward.search("c.exe");
//    if (!it.isValid())
//        cout << "c.exe is suddenly lost from DiskMultiMap..." << endl;
    
    unsigned int count = 0;
    badEntitiesFound.clear();
    for (int i = 0; i < indicators.size(); i++) {
        badEntitiesFound.push_back(indicators[i]);
    }
    
//    //debugging
//    for (int i = 0; i < badEntitiesFound.size(); i++) {
//        cerr << "badEntitiesFound: " << badEntitiesFound[i] << endl;
//    }
//    cerr << "badEntitiesFound.size() = " << badEntitiesFound.size() << endl;
    
    for (int i = 0; i < badEntitiesFound.size(); i++) {
        bool indicatorAppeared = false;
        
        DiskMultiMap::Iterator it1 = m_forward.search(badEntitiesFound[i]);
        if (it1.isValid()) {
//            cerr << "A node with the particular key found in Forward Hash Table." << endl;
            indicatorAppeared = true;
            do {
                MultiMapTuple m = *it1;
                bool hasStored = false; // store the new bad entity if this has not been done
                for (int j = 0; j < badEntitiesFound.size(); j++) {
                    if (badEntitiesFound[j] == m.value) {
                        hasStored = true;
                        break;
                    }
                }
                if (!hasStored)
                    badEntitiesFound.push_back(m.value);
                
                InteractionTuple aLog = *new InteractionTuple;
                aLog.from = m.key;
                aLog.to = m.value;
                aLog.context = m.context;
                interactions.push_back(aLog);
                
                count++;
                ++it1;
            } while (it1.isValid());
        }
        
        DiskMultiMap::Iterator it2 = m_reverse.search(badEntitiesFound[i]);
        if (it2.isValid()) {
            indicatorAppeared = true;
            do {
                MultiMapTuple m = *it2;
                bool hasStored = false;
                for (int j = 0; j < badEntitiesFound.size(); j++) {
                    if (badEntitiesFound[j] == m.value) {
                        hasStored = true;
                        break;
                    }
                }
                if (!hasStored)
                    badEntitiesFound.push_back(m.value);
                
                InteractionTuple aLog = *new InteractionTuple;
                aLog.from = m.value;
                aLog.to = m.key;
                aLog.context = m.context;
                interactions.push_back(aLog);
                
                count++;
                ++it2;
            } while (it2.isValid());
        }
        
        if (!indicatorAppeared) {
            badEntitiesFound.erase(badEntitiesFound.begin() + i);
            i--;
        }
    }
    
    return count;

    
//    // make a first pass to find bad entities by searching indicators passed in by user
//    for (int i = 0; i < indicators.size(); i++) {
//        bool indicatorAppeared = false;
//        DiskMultiMap::Iterator it1 = m_forward.search(indicators[i]);
//        if (it1.isValid())
//        {
//            badEntitiesFound.push_back(indicators[i]);
//            indicatorAppeared = true;
//            do {
//                MultiMapTuple m = *it1;
////                cout << "The key is: " << m.key << endl;
////                cout << "The value is: " << m.value << endl;
////                cout << "The context is: " << m.context << endl;
//                badEntitiesFound.push_back(m.value);
//                ++it1;
//            } while (it1.isValid());
//        }
//        
//        DiskMultiMap::Iterator it2 = m_reverse.search(indicators[i]);
//        if (it2.isValid()) {
//            if (!indicatorAppeared)
//                badEntitiesFound.push_back(indicators[i]);
//            
//            do {
//                MultiMapTuple m = *it2;
//                badEntitiesFound.push_back(m.value);
//                ++it2;
//            } while (it2.isValid());
//        }
//    }
    
    // repeatedly find new bad entities based on existing ones, until nothing new is found
//    for (int i = 0; i < badEntitiesFound.size(); i++) {
//        DiskMultiMap::Iterator it1= m_forward.search(badEntitiesFound[i]);
//        if (it1.isValid())
//    }
    
}
