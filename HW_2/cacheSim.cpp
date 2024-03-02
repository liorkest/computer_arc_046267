#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdint.h>


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

uint32_t ADDR_SIZE = 32;


class way
{
	std::vector<cache_block> way_data;
	uint32_t block_size;
	uint32_t tag_size;
	uint32_t block_bits_size;
	int ways_num;
	int blocks_num; // number of lines in way
	/// <- more variables for tag&block_index calc
	public:
	way(uint32_t number_of_blocks, uint32_t block_size, int ways_num){
		block_bits_size = log2(number_of_blocks);
		tag_size = ADDR_SIZE - 2 - block_bits_size;
		blocks_num = number_of_blocks;
		way_data.resize(number_of_blocks);
		way::block_size = block_size;
	}
	
	int access_data_from_way(uint32_t data_address) // returns 0 if block occipied by another, 1 if empty, 2 if block found
	{
		cache_block b = way_data[get_block_idx_from_addr(data_address)];
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
		return way_data[get_block_idx_from_addr(data_address)].get_age();
	}
	
	// sets the age of the new data to ways_num, and updates tag
	void add_new_data_to_way(uint32_t data_address) // write/overwrite the bock in this way
	{
		way_data[get_block_idx_from_addr(data_address)].add_cache_block(get_tag_from_addr(data_address), ways_num);
	}

	void update_age_of_not_accessed(uint32_t data_address) // will be called for all theways that weren't accessed to get the block
	{
		way_data[get_block_idx_from_addr(data_address)].update_age_of_not_accessed();
	}

	uint32_t get_tag_from_addr(uint32_t data_address)
	{
		uint32_t tag_mask = (1u << tag_size) - 1  ;
		uint32_t tag = (data_address >> (2 + block_bits_size)) & tag_mask;
		return tag;
	}
	uint32_t get_block_idx_from_addr(uint32_t data_address)
	{
		uint32_t block_idx_mask = (1u << block_bits_size) - 1  ;
		uint32_t block_idx = (data_address >> 2 )& block_idx_mask;
		return block_idx;
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
			cache_block::tag = tag;
			cache_block::age = ways_num;	// it has the highest value
		}
	}
	uint32_t get_tag(){return tag;}
	bool is_initialized() {return is_initialized;}
	uint32_t get_age() // will substract 1 from age of all.
	{
		return age;
	}
	void update_age_of_not_accessed() // will substract 1 from age of all.
	{
		age--;
	}
};


class cache
{
	private:
		uint32_t cache_size;
		std::vector<way> way_data;
		uint32_t block_size;
		bool allocate; 
		uint32_t mem_cycles;
		uint32_t cache_cycles;
		cache * lower_cache;
		uint32_t assoc_lvl;
		uint32_t way_lines;
		int misses, access_times;

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
				misses=0;
				access_times=0;
				if (pnt_lower == NULL)
					cache::lower_cache == NULL;
				else
					cache::lower_cache == pnt_lower;
				cache::way_lines = cache_size / (block_size *assoc_lvl); 	
				for(unsigned i=0 ; i< assoc_lvl ; i++){
					way w = way(way_lines,block_size, assoc_lvl);	
					way_data.push_back(w);
				}

		}
		
		int get_accesses_num() {return access_times;}
		double get_missRate(){return (double) misses/access_times;}	
		
		//search for data_adress in current cache - doesn't check next lvl!
		uint32_t cache_read(uint32_t data_adress)
		{
			access_times++;
			uint32_t delay = 0;	
			bool data_found = false;
			/// SEARCH FOR DATA IN RELVENT CACHE ////
			//run through all way to read the data
			for(unsigned i=0 ; i< assoc_lvl ; i++){
				if(way_data[i].access_data_from_way(data_adress) == 2)//found data in some way
					data_found = true ;
			}
			//if found update delay for current cache
			if(data_found)
				delay = delay + cache_cycles;
				
			//data not found - we search in lower lvls
			if(!data_found ){
				misses++;
				//we are in L1, so we seach in L2	
				if(lower_cache!=NULL)
					delay = cache_cycles + (*lower_cache).cache_read(data_adress)   ; // if not found add delay to next level
				//we are in L2, so we search in mem
				if(lower_cache==NULL)
					delay = cache_cycles + mem_cycles;
			}
			//// ADD DATA TO RELEVENT CACHE ////////
			bool got_added=false; // initialize to check if data got added
			unsigned int updated_way;
			//if not found add data to cache, go in at every cache
			if(!data_found){
				//run through all ways to find empty spot
				for(unsigned i=0 ; i< assoc_lvl ; i++){
					if(way_data[i].access_data_from_way(data_adress) == 1)//found empty spot
					{
						way_data[i].add_new_data_to_way(data_adress);
						got_added=true;
						updated_way = i;
						break; // if wrote to one spot, break the loop(avoid copies)
					}
				}
				//if no empty spot add to least recent spot
				if(!got_added){
					//run through all way to find LRU spot
					for(unsigned i=0 ; i< assoc_lvl ; i++){
						if(way_data[i].get_age_of_block(data_adress) == 1)//found LRU
						{
							way_data[i].add_new_data_to_way(data_adress);
							got_added=true;
							updated_way = i;
						}
					}	
				}
			}
			///UPDATE AGE OF WAYS///
			for(unsigned i=0 ; i< assoc_lvl ; i++){
				if(i != updated_way)//way needs to get updated
					way_data[i].update_age_of_not_accessed(data_adress);
			}	

			return delay;
		}
		
		//NEED TO UPDATE!!!!!!!
		uint32_t cache_write(uint32_t data_adress){
			return 0;
		}
		
		
	
};


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

	cache L2(L2Size,BSize,WrAlloc,MemCyc,L2Cyc,L2Assoc,NULL);
	cache L1(L1Size,BSize,WrAlloc,MemCyc,L1Cyc,L1Assoc,&L2);
	int total_delay = 0;

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
		
		if(operation == 'r')
		{
			total_delay += L1.cache_read(num);
		}
		else
		{
			total_delay += L1.cache_write(num);
		}

	}

	double L1MissRate = L1.get_missRate();
	double L2MissRate = L2.get_missRate();
	double avgAccTime = total_delay / L1.get_accesses_num();


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
