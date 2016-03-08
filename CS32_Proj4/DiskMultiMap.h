#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

/*
bool write(const char* s, size_t length, BinaryFile::Offset toOffset);
bool write(const SomeType& x, BinaryFile::Offset toOffset);
 
bool read(char* s, size_t length, BinaryFile::Offset toOffset);
bool read(SomeType& x, BinaryFile::Offset toOffset);
*/

class DiskMultiMap
{
public:

	class Iterator
	{
	public:
		Iterator();
		// You may add additional constructors
        Iterator(BinaryFile& file, BinaryFile::Offset node, const std::string& key, const std::string& value, const std::string& context);
        
        bool isValid() const { return m_state; }
		Iterator& operator++();
		MultiMapTuple operator*();

	private:
		// Your private member declarations will go here
        BinaryFile* m_bf;
        
        bool m_state; // true for valid, false for invalid
        BinaryFile::Offset m_node;
        
        std::string m_key;
        std::string m_value;
        std::string m_context;
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	// Your private member declarations will go here
    BinaryFile bf;
    bool fileLoaded;
    int nBuckets;
    
    BinaryFile::Offset getFileSize();
    void updateFileSize(size_t increaseSize);
    
    struct Node {

        const char *key;
        const char *value;
        const char *context;
        
        BinaryFile::Offset next;
        
    };
};

#endif // DISKMULTIMAP_H_
