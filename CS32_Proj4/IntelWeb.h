#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include <string>
#include <vector>
#include "DiskMultiMap.h"

class IntelWeb
{
public:
        IntelWeb();
        ~IntelWeb();
        bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
        bool openExisting(const std::string& filePrefix);
        void close();
        bool ingest(const std::string& telemetryFile);
        unsigned int crawl(const std::vector<std::string>& indicators,
                	   unsigned int minPrevalenceToBeGood,
                	   std::vector<std::string>& badEntitiesFound,
                	   std::vector<InteractionTuple>& interactions
                	  );
        bool purge(const std::string& entity);
    
    //debug
    

private:
	// Your private member declarations will go here
    DiskMultiMap m_forward;
    DiskMultiMap m_reverse;
    
    bool m_fileLoaded;
    
    // discover possible valid bad entities based on one known bad entity, in one DiskMutliMap
    int crawlMap(DiskMultiMap& map, int pos, const int mapDir,unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& interactions); // mapDir = 1 for forward map, mapDir = 2 for reverse map
    
};

#endif // INTELWEB_H_
                
