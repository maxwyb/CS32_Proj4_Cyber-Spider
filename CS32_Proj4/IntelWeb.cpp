#include <iostream>
#include <vector>
#include <algorithm>
#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include "InteractionTuple.h"

using namespace std;

// *** Non-member functions *** //
bool cmp(const InteractionTuple &a, const InteractionTuple &b) {
    if (a.context != b.context) {
        bool retVal = (a.context < b.context) ? true : false;
        return retVal;
    } else if (a.from != b.from) {
        bool retVal = (a.from < b.from) ? true : false;
        return retVal;
    } else {
        bool retVal = (a.to < b.to) ? true : false;
        return retVal;
    }
}


// *** IntelWeb *** //
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
    
//    cerr << "String parse: " << aLog[0] << "; " << aLog[1] << "; " << aLog[2] << endl;
    
    bool success;
    success = m_forward.insert(aLog[1], aLog[2], aLog[0]);
    if (!success)
        return false;
    success = m_reverse.insert(aLog[2], aLog[1], aLog[0]);
    if (!success)
        return false;
    
    
    return true;
}


unsigned int IntelWeb::crawl(const vector<string>& indicators, unsigned int minPrevalenceToBeGood, vector<string>& badEntitiesFound, vector<InteractionTuple>& interactions) {
    
    
    unsigned int count = 0;
    badEntitiesFound.clear();
    for (int i = 0; i < indicators.size(); i++) {
        badEntitiesFound.push_back(indicators[i]);
    }
    
    
    for (int i = 0; i < badEntitiesFound.size(); i++) {
        bool indicatorAppeared = false;
        
        DiskMultiMap::Iterator it1 = m_forward.search(badEntitiesFound[i]);
        if (it1.isValid()) {
//            cerr << "A node with the particular key found in Forward Hash Table." << endl;
            indicatorAppeared = true;
            do {
                MultiMapTuple m = *it1;
                bool hasStoredEntry = false; // store the new bad entity if this has not been done
                for (int j = 0; j < badEntitiesFound.size(); j++) {
                    if (badEntitiesFound[j] == m.value) {
                        hasStoredEntry = true;
                        break;
                    }
                }
                if (!hasStoredEntry)
                    badEntitiesFound.push_back(m.value);
                
                InteractionTuple aLog = *new InteractionTuple;
                aLog.from = m.key;
                aLog.to = m.value;
                aLog.context = m.context;
                
                bool hasStoredInteraction = false;
                for (int j = 0; j < interactions.size(); j++) {
                    if (interactions[j].from == aLog.from && interactions[j].to == aLog.to && interactions[j].context == aLog.context) {
                        hasStoredInteraction = true;
                        cerr << "Detected that one InteractionTuple has been stored; ignore the current one." << endl;
                        break;
                    }
                }
                if (!hasStoredInteraction)
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
                bool hasStoredEntry = false;
                for (int j = 0; j < badEntitiesFound.size(); j++) {
                    if (badEntitiesFound[j] == m.value) {
                        hasStoredEntry = true;
                        break;
                    }
                }
                if (!hasStoredEntry)
                    badEntitiesFound.push_back(m.value);
                
                InteractionTuple aLog = *new InteractionTuple;
                aLog.from = m.value;
                aLog.to = m.key;
                aLog.context = m.context;
                
                bool hasStoredInteraction = false;
                for (int j = 0; j < interactions.size(); j++) {
                    if (interactions[j].from == aLog.from && interactions[j].to == aLog.to && interactions[j].context == aLog.context) {
                        hasStoredInteraction = true;
                        cerr << "Detected that one InteractionTuple has been stored; ignore the current one." << endl;
                        break;
                    }
                }
                if (!hasStoredInteraction)
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
    
    sort(badEntitiesFound.begin(), badEntitiesFound.end());
    sort(interactions.begin(), interactions.end(), cmp);
    
    return count;
    
}
