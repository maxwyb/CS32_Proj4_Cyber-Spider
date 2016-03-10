#include <iostream>
#include <vector>
#include <algorithm>
#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include "InteractionTuple.h"

using namespace std;

// *** Non-member functions *** //
bool cmp(const InteractionTuple &a, const InteractionTuple &b) { // compare function used in sort for InteractionTuple
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


int IntelWeb::crawlMap(DiskMultiMap& map, int pos, const int mapDir, unsigned int minPrevalenceToBeGood, vector<string>& badEntitiesFound, vector<InteractionTuple>& interactions) {
    
    int count = 0;
    
    DiskMultiMap::Iterator it = map.search(badEntitiesFound[pos]);
    if (it.isValid()) {

        do {
            MultiMapTuple m = *it;
            bool toStoreEntry = true;
            for (int j = 0; j < badEntitiesFound.size(); j++) { // if it has been stored before
                if (badEntitiesFound[j] == m.value) {
                    toStoreEntry = false;
                    break;
                }
            }
            
            if (toStoreEntry) { // if it has high prevalence to be good
                int prevalence = 0;

                DiskMultiMap::Iterator preit1 = m_forward.search(m.value);
                while (preit1.isValid()) {
                    prevalence++;
                    ++preit1;
                }
                DiskMultiMap::Iterator preit2 = m_reverse.search(m.value);
                while (preit2.isValid()) {
                    prevalence++;
                    ++preit2;
                }
                
//                cerr << m.value << " Prevalence = " << prevalence << endl;
                if (prevalence >= minPrevalenceToBeGood) {
//                    cerr << m.value << "'s prevalence exceeds the threshold; determined to be good." << endl;
                    toStoreEntry = false; // means not to store this entity as bad
                }
            }
            
            if (toStoreEntry) {
//                cerr << m.value << " is going to be stored in bad entity." << endl;
                badEntitiesFound.push_back(m.value);
                
                count++;
            }
            
            InteractionTuple aLog = *new InteractionTuple;
            if (mapDir == 1) { // initializing based on DiskMultiMap's direction
                aLog.from = m.key;
                aLog.to = m.value;
            } else if (mapDir == 2) {
                aLog.from = m.value;
                aLog.to = m.key;
            }
            aLog.context = m.context;
            
            bool hasStoredInteraction = false;
            for (int j = 0; j < interactions.size(); j++) {
                if (interactions[j].from == aLog.from && interactions[j].to == aLog.to && interactions[j].context == aLog.context) {
                    hasStoredInteraction = true;
//                  cerr << "Detected that one InteractionTuple has been stored; ignore the current one." << endl;
                    break;
                }
            }
            if (!hasStoredInteraction) // store it if it has not been stored before
                interactions.push_back(aLog);
            
            ++it;
        } while (it.isValid());
    }
    
    return count;
}


unsigned int IntelWeb::crawl(const vector<string>& indicators, unsigned int minPrevalenceToBeGood, vector<string>& badEntitiesFound, vector<InteractionTuple>& interactions) {
    
    int count = 0;
    
    badEntitiesFound.clear();
    for (int i = 0; i < indicators.size(); i++) {
        badEntitiesFound.push_back(indicators[i]);
    }
    
    // remove indicators that do not appear in telemetry logs
    for (int i = 0; i < badEntitiesFound.size(); i++) {
        bool indicatorAppeared = false;
        
        DiskMultiMap::Iterator it1 = m_forward.search(badEntitiesFound[i]);
        DiskMultiMap::Iterator it2 = m_reverse.search(badEntitiesFound[i]);
        if (it1.isValid() || it2.isValid())
            indicatorAppeared = true;
        
        if (!indicatorAppeared) { // erase bad entities that do not appear in telemetry logs
            badEntitiesFound.erase(badEntitiesFound.begin() + i);
            i--;
        }
    }
    
    for (int i = 0; i < badEntitiesFound.size(); i++) {
        
        // log all bad values based on the current key, in the forward map
        count += crawlMap(m_forward, i, 1, minPrevalenceToBeGood, badEntitiesFound, interactions);
        
        // log all bad keys based on the current value, in the reverse map
        count += crawlMap(m_reverse, i, 2, minPrevalenceToBeGood, badEntitiesFound, interactions);
        
    }
    
    sort(badEntitiesFound.begin(), badEntitiesFound.end());
    sort(interactions.begin(), interactions.end(), cmp);
    
    return count;
    
}


bool IntelWeb::purge(const string& entity) {
    
    vector<MultiMapTuple> nodesToDelete;
    // traverse two MultiMaps to get all exact Nodes to delete
    DiskMultiMap::Iterator it1 = m_forward.search(entity);
    if (it1.isValid()) {
        do {
            MultiMapTuple storing;
            storing.key = (*it1).key;
            storing.value = (*it1).value;
            storing.context = (*it1).context;
            
            nodesToDelete.push_back(storing);
            ++it1;
        } while (it1.isValid());
    }
    DiskMultiMap::Iterator it2 = m_reverse.search(entity);
    if (it2.isValid()) {
        do {
            MultiMapTuple storing;
            storing.key = (*it2).value; // reverse the key and value when storing the Nodes to delete
            storing.value = (*it2).key;
            storing.context = (*it2).context;
            
            nodesToDelete.push_back(storing);
            ++it2;
        } while (it2.isValid());
    }
    
    int count = 0;
    for (int i = 0; i < nodesToDelete.size(); i++) {
        count += m_forward.erase(nodesToDelete[i].key, nodesToDelete[i].value, nodesToDelete[i].context);
        count += m_reverse.erase(nodesToDelete[i].value, nodesToDelete[i].key, nodesToDelete[i].context);
    }
    
    if (count == 0)
        return false;
    else
        return true;
}

