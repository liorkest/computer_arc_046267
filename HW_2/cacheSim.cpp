#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	/*
	
	add code here
	
	
	
	*/


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

class way
{
	std::vector<cache_block> cache_data;
	uint32_t block_size;
	/// <- more variables for tag&block_index calc
	public:
	way(uint32_t number_of_blocks, uint32_t block_size){
		cache_data.resize(number_of_blocks);
		way::block_size = block_size;
	}
	
	int access_data_from_way(uint32_t data_address) // returns 0 if block occipied by another, 1 if empty, 2 if block found
	{
		cache_block b = cache_data[get_block_index_from_addr(data_address)];
		if (get_tag_from_addr(data_address) == b.get_tag())
		{
			/// <- block found, update it's age
			return 2;
		}
		else if (b.is_initialized())
		{
			return 0;
		}
		else{
			return 1;
		}
	}
	
	uint32_t get_age_of_block(uint32_t data_address) // will be called for all ways, if the data not found and need to understand who to evict
	{

	}

	bool add_new_data_to_way(uint32_t data_address){
		
	}

	void update_age_of_not_accessed(uint32_t data_address) // will be called for all theways that weren't accessed to get the block
	{
		cache_data[get_block_index_from_addr(data_address)].update_age_of_not_accessed();
	}

	uint32_t get_tag_from_addr(uint32_t data_address)
	{
		
	}
	uint32_t get_block_index_from_addr(uint32_t data_address)
	{
		
	}
};

class cache_block
{
	uint32_t tag;
	bool initialized;
	uint32_t age; // for eviction policy
	bool dirty;   /////// <- PROBABLY NOT NEEDED!!!!
	public:
	cache_block()
	{
		initialized = false;
	}
	void add_cache_block(uint32_t tag, uint32_t  ways_num)
	{
		if(!initialized) {
			initialized = true;
			cache_block::tag = tag;
			cache_block::age = ways_num;	// it has the highest value
		}
		else // already contains data
		{
			//// ???
		}
	}
	uint32_t get_tag(){return tag;}
	bool is_initialized() {return is_initialized;}
	void update_age_of_not_accessed() // will substract 1 from age of all.
	{
		
	}
};


class cache
{
	private:
		uint32_t cache_size;
		std::vector<way> cache_data;
		uint32_t block_size;
		bool allocate; 
		uint32_t mem_cycles;
		uint32_t cache_cycles;
		cache* lower_cache;
		uint32_t assoc_lvl;
		uint32_t way_lines;


	public:

		cache(unsigned cache_size,unsigned block_size,bool allocate,unsigned mem_cycles,
			unsigned cache_cycles,uint32_t assoc_lvl,cache* pnt_lower){
				cache::cache_size = cache_size;
				cache::cache_cycles = cache_cycles;
				cache::mem_cycles = mem_cycles;
				cache::allocate = allocate;
				cache::lower_cache = lower_cache;
				cache::block_size = block_size;
				cache::assoc_lvl = assoc_lvl;
				if (pnt_lower == NULL)
					cache::lower_cache == NULL;
				else
					cache::lower_cache == pnt_lower;
				cache::way_lines = cache_size / (block_size *assoc_lvl); 	
				for(unsigned i=0 ; i< way_lines ; i++){
					way w = way(way_lines,block_size);	
					cache_data.push_back(w);
				}

		}
			
		//search for data_adress in current cache - doesn't check next lvl!
		bool cache_read(uint32_t data_adress)
		{
		bool data_found = false;
		//run through all way to read the data
		for(unsigned i=0 ; i< way_lines ; i++){
			if(cache_data[i].access_data_from_way(data_adress))
				data_found = true ;
		}
		return data_found;
		}
		
		
		// returns 0 if block occipied by another, 1 if empty, 2 if block found


		
		bool cache_write(uint32_t data_adress){
		//find the block
		uint32_t block_line = find_block(data_adress);
		//search if the block already in current cache lvl
		bool found = cache_read(data_adress);
		//if not found write that block in cache
		if(!found){
			


		}



		}


		uint32_t find_block(uint32_t data_adress);

};
