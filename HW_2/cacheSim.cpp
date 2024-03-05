#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <math.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

uint32_t ADDR_SIZE = 32;

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
	bool is_initialized() {return initialized;}
	uint32_t get_age() // will substract 1 from age of all.
	{
		return age;
	}
	void update_age_of_not_accessed() // will substract 1 from age of all.
	{
		if(initialized){
			age--;
		}
	}

	void print()
	{
		printf("	tag=%d, initialized=%d, age=%d, dirty=%d\n", tag, initialized, age, dirty);
	}
};
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
		way::ways_num = ways_num;
		printf("in way init, block_bits_size: %dblock_bits_size , tag_size: %d, blocks_num: %d \n",block_bits_size,tag_size,blocks_num);
		tag_size = ADDR_SIZE - 2 - block_bits_size;
		blocks_num = number_of_blocks;
		way_data.resize(number_of_blocks);
		way::block_size = block_size;
	}
	
	int access_data_from_way(uint32_t data_address) // returns 0 if block occipied by another, 1 if empty, 2 if block found
	{
		printf("access_data 1\n");
		uint32_t way_idx= get_block_idx_from_addr(data_address);
		printf("way idx: %d\n",way_idx);
		cache_block b = way_data[way_idx];
		printf("acces_data 2\n");

		if (get_tag_from_addr(data_address) == b.get_tag())
		{
					printf("access_data 2.1\n");

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
		printf("block_idx: %d\n", block_idx);
		return block_idx;
	}

	void print()
	{
		printf("Way has %d blocks, each one %d bytes:\n", blocks_num, block_size);
		for(int i=0; i<blocks_num;i++)
		{
			printf("	block %d:\n", i);
			way_data[i].print();
		}
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
		uint32_t num_of_ways;
		int misses, access_times;

	public:

		cache(unsigned cache_size,unsigned block_size,bool allocate,unsigned mem_cycles,
			unsigned cache_cycles,uint32_t assoc_lvl,cache* pnt_lower){
				cache::cache_size = pow(2,cache_size);
				cache::cache_cycles = cache_cycles;
				cache::mem_cycles = mem_cycles;
				cache::allocate = allocate;
				cache::lower_cache = lower_cache;
				cache::block_size = pow(2,block_size);
				cache::assoc_lvl = assoc_lvl;
				misses=0;
				access_times=0;
				if (pnt_lower == NULL)
					cache::lower_cache = NULL;
				else
					cache::lower_cache = pnt_lower;
				
				num_of_ways = pow(2, assoc_lvl);
				uint32_t num_of_blocks_in_way = cache::cache_size / (cache::block_size * cache::num_of_ways); 
				printf("num_of_blocks_in_way %d\n",num_of_blocks_in_way);	
				for(unsigned i=0 ; i < cache::num_of_ways ; i++){
					way w = way(num_of_blocks_in_way,cache::block_size, cache::num_of_ways);	
					way_data.push_back(w);
				}
			print();
		}
		
		int get_accesses_num() { return access_times ? access_times : -1 ;}
		double get_missRate() { return (double) access_times ? misses/access_times : -1 ;}	
		
		//search for data_address in current cache - doesn't check next lvl!
		uint32_t cache_read(uint32_t data_address)
		{
			uint32_t delay = 0;	
			access_times++;//only if cache excists or everytime??????????
			/*
			if(assoc_lvl==0){
				if(lower_cache!=NULL)
					return mem_cycles;
				else
					return cache_cycles + (*lower_cache).cache_read(data_address); // with delay of current cache???????????
			}
			else
			*/
			
			bool data_found = false;
			/// SEARCH FOR DATA IN RELVENT CACHE ////
			//run through all way to read the data
			for(unsigned i=0 ; i< num_of_ways ; i++){
				if(way_data[i].access_data_from_way(data_address) == 2)//found data in some way
					data_found = true ;
			}
			//if found update delay for current cache
			if(data_found)
				delay = delay + cache_cycles;
				
			//data not found - we search in lower lvls
			if(!data_found ){
				misses++;
				//we are in L1, so we seach in L2	
				if(lower_cache!=NULL){
					printf("before recursion");
					delay = cache_cycles + (*lower_cache).cache_read(data_address)   ; // if not found add delay to next level
					printf("after recursion");

				}
				//we are in L2, so we search in mem
				if(lower_cache==NULL)
					delay = cache_cycles + mem_cycles;
			}
			//// ADD DATA TO RELEVENT CACHE ////////
			bool got_added=false; // initialize to check if data got added
			unsigned int updated_way;
			//if not found add data to cache, go in at every cache
			
			if(!data_found ){
				//run through all ways to find empty spot
				for(unsigned i=0 ; i< num_of_ways ; i++){
					if(way_data[i].access_data_from_way(data_address) == 1)//found empty spot
					{
						way_data[i].add_new_data_to_way(data_address);
						printf("block was added to way %d\n", i);
						got_added=true;
						updated_way = i;
						break; // if wrote to one spot, break the loop(avoid copies)
					}
				}
				//if no empty spot add to least recent spot
				if(!got_added){
					//run through all way to find LRU spot
					for(unsigned i=0 ; i< num_of_ways ; i++){
						if(way_data[i].get_age_of_block(data_address) == 1)//found LRU
						{
							way_data[i].add_new_data_to_way(data_address);
							got_added=true;
							updated_way = i;
						}
					}	
				}
			}
			///UPDATE AGE OF WAYS///
			for(unsigned i=0 ; i< num_of_ways ; i++){
				if(i != updated_way)//way needs to get updated
					way_data[i].update_age_of_not_accessed(data_address);
			}	
			
			return delay;
		}
		
		//NEED TO UPDATE!!!!!!!
		uint32_t cache_write(uint32_t data_address){
			return 0;
		}

		void print(){
			printf("Printing cache data\n");
			printf("cache_size=%d, block_size=%d, allocate=%d, mem_cycles=%d, cache_cycles=%d, lower_cache(pointer)=%x, assoc_lvl=%d, num_of_ways=%d\n",cache_size,block_size, allocate,mem_cycles, cache_cycles,lower_cache,assoc_lvl,num_of_ways);
			printf("misses=%d, access_times=%d\n", misses, access_times);
			for(int i=0; i<num_of_ways; i++){
				printf("Way #%d:\n", i);
				way_data[i].print();
			}
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



	printf("~~~L2~~~\n");
	cache L2(L2Size,BSize,WrAlloc,MemCyc,L2Cyc,L2Assoc,NULL);
	printf("~~~L1~~~\n");
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
	printf("after initilazation of caches");

	double L1MissRate = L1.get_missRate();
	double L2MissRate = L2.get_missRate();
	double avgAccTime = 0;

	L1.print();
	L2.print();


	if (L1.get_accesses_num() != 0)
		avgAccTime = total_delay / L1.get_accesses_num();


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);
		
	return 0;
}
