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
	uint32_t  _ways_num;
	public:
	bool dirty;  
	bool get_dirty_bit(){return dirty;}
	cache_block()
	{
		dirty = false;
		initialized = false;
	}
	void add_cache_block(uint32_t tag, uint32_t  ways_num)
	{
		_ways_num = ways_num;
		if(!initialized){
			initialized = true;
		}
		cache_block::tag = tag;
		dirty = false;
	}
	uint32_t get_tag(){return tag;}
	bool is_initialized() {return initialized;}
	uint32_t get_age() // will substract 1 from age of all.
	{
		if(!initialized)
			return -1;
		else
			return age;
	}
	void update_age_of_not_accessed() // will substract 1 from age of all.
	{
		if(initialized){
			age--;
		}
	}
	void update_age_of_accessed() // will reset the age to newest
	{
		if(initialized){
			age = _ways_num;
		}
	}

	void remove_block()
	{
		initialized = false;
		tag = 0;
        dirty = false;
	}

	void print()
	{
		printf("	tag=%d, initialized=%d, age=%d, dirty=%d\n", tag, initialized, age, dirty);
	}
};

class way
{
	std::vector<cache_block> ways_list;
	uint32_t block_size;
	uint32_t tag_mask_bits;
	uint32_t block_bits_size;
	int ways_num;
	int blocks_num; // number of lines in way
	/// <- more variables for tag&block_index calc
	public:
	way(uint32_t number_of_blocks_in_way, uint32_t block_size, int ways_num){
		block_bits_size = log2(block_size);
		way::ways_num = ways_num;
		blocks_num = number_of_blocks_in_way;
		ways_list.resize(number_of_blocks_in_way);
		way::block_size = block_size;
		tag_mask_bits = log2(number_of_blocks_in_way) + block_bits_size;

	}
	
	bool is_dirty_block(uint32_t data_address)
	{
		return( ways_list[get_block_idx_from_addr(data_address)].get_dirty_bit());
	}

	int access_data_from_way(uint32_t data_address) // returns 0 if block occipied by another, 1 if empty, 2 if block found
	{
		cache_block b = ways_list[get_block_idx_from_addr(data_address)];
		if (b.is_initialized())
		{
			if (get_tag_from_addr(data_address) == b.get_tag())
				return 2;
			else
				return 0;
		}
		else{
			return 1;
		}
	}

	void set_dirty_bit(uint32_t addr, bool b){
		ways_list[get_block_idx_from_addr(addr)].dirty = b;
	}
	
	uint32_t get_age_of_block(uint32_t data_address) // will be called for all ways, if the data not found and need to understand who to evict
	{
		return ways_list[get_block_idx_from_addr(data_address)].get_age();
	}
	
	// sets the age of the new data to ways_num, and updates tag
	void add_new_data_to_way(uint32_t data_address) // write/overwrite the bock in this way
	{
		ways_list[get_block_idx_from_addr(data_address)].add_cache_block(get_tag_from_addr(data_address), ways_num);
	}

	void update_age_of_not_accessed(uint32_t data_address) // will be called for all theways that weren't accessed to get the block
	{
		ways_list[get_block_idx_from_addr(data_address)].update_age_of_not_accessed();
	}

	void update_age_of_accessed(uint32_t data_address) // will be called for all theways that weren't accessed to get the block
	{
		ways_list[get_block_idx_from_addr(data_address)].update_age_of_accessed();
	}

	uint32_t get_tag_from_addr(uint32_t data_address)
	{
		uint32_t tag = data_address >> tag_mask_bits;
		return tag;
	}
	uint32_t get_block_idx_from_addr(uint32_t data_address)
	{
		uint32_t block_idx = data_address >> block_bits_size;
		block_idx = block_idx % (int)(blocks_num);
		return block_idx;
	}

	uint32_t get_address_of_existing_block(uint32_t given_address) {
		// this function gets an address, goes to the block the address is mapped to, and
		// returns the address of the block which already exists in cache. It can be different! (different tag)
		unsigned block_idx = get_block_idx_from_addr(given_address);
		if (ways_list[block_idx].is_initialized() )
		{
			return (ways_list[block_idx].get_tag() << tag_mask_bits) + (block_idx << block_bits_size);
		}
		else
			return 0xFFFFFFFF;
	}

	void delete_block(uint32_t data_address)
	{
		int block_idx = get_block_idx_from_addr(data_address);
		if (ways_list[block_idx].is_initialized() && ways_list[block_idx].get_tag() == get_tag_from_addr(data_address))
		{
			//ways_list[block_idx].print();
			ways_list[block_idx].remove_block();
		}

	}

	void print()
	{
		printf("Way has %d blocks, each one %d bytes:\n", blocks_num, block_size);
		for(int i=0; i<blocks_num;i++)
		{
			printf("	block %d:\n", i);
			ways_list[i].print();
		}
	}
};



class cache
{
	private:
		uint32_t cache_size;
		std::vector<way> ways_list;
		uint32_t block_size_bytes;
		bool allocate; 
		uint32_t mem_cycles;
		uint32_t cache_cycles;
		cache * lower_cache;
		cache * higher_cache;
		uint32_t num_of_ways;
		int misses, access_times;

	public:

		cache(unsigned cache_size_bits,unsigned block_bits,bool allocate,unsigned mem_cycles,
			unsigned cache_cycles,uint32_t assoc_lvl,cache* pnt_lower)
		{
				higher_cache = NULL; // default
				cache::cache_size = pow(2,cache_size_bits);
				cache::cache_cycles = cache_cycles;
				cache::mem_cycles = mem_cycles;
				cache::allocate = allocate;
				cache::lower_cache = lower_cache;
				cache::block_size_bytes = pow(2,block_bits);
				misses=0;
				access_times=0;
				if (pnt_lower == NULL)
					cache::lower_cache = NULL;
				else
					cache::lower_cache = pnt_lower;
				
				num_of_ways = pow(2, assoc_lvl);
				int num_of_blocks_in_way = cache_size / (block_size_bytes * num_of_ways); 
				for(unsigned i=0 ; i < num_of_ways ; i++){
					way w = way(num_of_blocks_in_way,block_size_bytes, num_of_ways);	
					ways_list.push_back(w);
				}
		}
		
		int get_accesses_num() { return access_times ? access_times : -1 ;}
		int get_miss_num() { return misses ? misses : -1 ;}
		int get_misses_num() { return misses ? misses : -1 ;}
		double get_missRate() { return (double) access_times ? (double)misses/access_times : -1 ;}	
		void set_higher(cache* higher) {higher_cache = higher;}
		//search for data_address in current cache - doesn't check next lvl!
		uint32_t cache_read(uint32_t data_address)
		{
			uint32_t delay = 0;	
			access_times++;
			int way_idx_of_data = search_in_cache(data_address);
			if(way_idx_of_data == -1) // not found in the cache
			{
				//data not found - we search in lower lvls
				misses++; 
				//we are in L1, so we seach in L2	
				if (lower_cache!=NULL){
					//printf("read lower cache\n");
					delay = cache_cycles + (*lower_cache).cache_read(data_address); // if not found add delay to next level
				}
				else 
				{ //we are in L2, so we search in mem
					delay = cache_cycles + mem_cycles;
				}
				way_idx_of_data = add_new_block_to_cache(data_address);
			}
			else 
			{ 
				delay = cache_cycles;
			}
			if(way_idx_of_data != -1)
				update_ages(data_address, way_idx_of_data);  
			//print();			
			return delay;
		}
		
		
		uint32_t cache_write(uint32_t data_address)
		{
			//IF HIT - already exists in lower levels, assume values are updated in background
			int way_idx_of_data = search_in_cache(data_address);
			if(way_idx_of_data != -1){
				access_times++;
				ways_list[way_idx_of_data].set_dirty_bit(data_address, true);
				//printf("Miss rate: %f\n",get_missRate());
				//update ages in L1 
				update_ages(data_address, way_idx_of_data);
				//update ages in L2 (if command accessed from L1)
				if(lower_cache!=NULL){
					int way_idx_of_data_lower = (*lower_cache).search_in_cache(data_address);
					(*lower_cache).update_ages(data_address,way_idx_of_data_lower);
				}
				return cache_cycles;
			}
			//IF MISS
			else{
				//if write allocate, then execute read function
				if(allocate){
					int time = cache_read(data_address);
					int way_idx_of_data = search_in_cache(data_address);
					ways_list[way_idx_of_data].set_dirty_bit(data_address, true);
					return time;
				}	
				//if write no allocate
				else{
					misses++;
					if(lower_cache!=NULL){//we are in L1, write in L2
						access_times++;
						return (*lower_cache).cache_write(data_address);
					}
					else//we are in L2, write to mem
					{
						
						access_times++;
						return mem_cycles;
					}
				}	
			}
		
		}

		int search_in_cache(uint32_t data_address)
		{
			//run through all way to read the data
			for(unsigned i=0 ; i< num_of_ways ; i++){
				if(ways_list[i].access_data_from_way(data_address) == 2)//found data in one way
					return i ;
			}
			return -1;
		}

		int add_new_block_to_cache(uint32_t data_address) // returns the index of way that the data was added to 
		{
			//run through all ways to find empty spot
			bool got_added=false; // initialize to check if data got added 
			unsigned int updated_way;
			// if block exists - update it
			for(unsigned i=0 ; i< num_of_ways ; i++){
				if(ways_list[i].access_data_from_way(data_address) == 2) //found the block!
				{
					ways_list[i].add_new_data_to_way(data_address);
					//printf("block was added to way %d\n", i);
					got_added=true;
					updated_way = i;
					break; // if wrote to one spot, break the loop(avoid copies)
				}
			}			
			if(!got_added)
			{
				for(unsigned i=0 ; i< num_of_ways ; i++){
					if(ways_list[i].access_data_from_way(data_address) == 1)//found empty spot
					{
						ways_list[i].add_new_data_to_way(data_address);
						//printf("block was added to way %d\n", i);
						got_added=true;
						updated_way = i;
						break; // if wrote to one spot, break the loop(avoid copies)
					}
				}
			}
			//if no empty spot add to least recent spot
			if(!got_added)
			{
				//run through all way to find LRU spot
				for(unsigned i=0 ; i< num_of_ways ; i++){
					if(ways_list[i].get_age_of_block(data_address) == 1)//found LRU
					{
						// delete the evicted block from L1 to keep inclusive
						if(higher_cache)
						{
							//printf("eviction of block in upper cache!\n");
							uint32_t adress= ways_list[i].get_address_of_existing_block(data_address);
							if(adress!=0xFFFFFFFF)
								higher_cache->remove_if_exists(adress);
						}
						// update the evicted block in lower cache if dirty
						if(ways_list[i].is_dirty_block(data_address) && lower_cache != NULL)
						{
							uint32_t adress= ways_list[i].get_address_of_existing_block(data_address);
							if(adress!=0xFFFFFFFF)
								lower_cache->add_new_block_to_cache(adress); 
						}
						// overwrite the block with the new one
						ways_list[i].add_new_data_to_way(data_address);
						updated_way = i;
					}
				}	
			}		

			return updated_way;
		}

		void remove_if_exists(uint32_t data_address)
		{
			//run through all way to find the data
			for(unsigned i=0 ; i< num_of_ways ; i++){
				ways_list[i].delete_block(data_address);
			}
		}

		void update_ages(uint32_t data_address, int accesed_way_idx){
            int updated_block_age = ways_list[accesed_way_idx].get_age_of_block(data_address);
            ways_list[accesed_way_idx].update_age_of_accessed(data_address);
            for(unsigned i=0 ; i< num_of_ways ; i++){
                if(ways_list[i].get_age_of_block(data_address) >= updated_block_age && i != accesed_way_idx)//way needs to get updated
                    ways_list[i].update_age_of_not_accessed(data_address);
            }	
		}

		void print(){
			printf("Printing cache data\n");
			printf("cache_size=%d, block_size=%d, allocate=%d, mem_cycles=%d, cache_cycles=%d, lower_cache(pointer)=%p, num_of_ways=%d\n",cache_size,block_size_bytes, allocate,mem_cycles, cache_cycles,lower_cache,num_of_ways);
			printf("misses=%d, access_times=%d\n", misses, access_times);
			for(int i=0; i<num_of_ways; i++){
				printf("Way #%d:\n", i);
				ways_list[i].print();
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



	cache L2(L2Size,BSize,WrAlloc,MemCyc,L2Cyc,L2Assoc,NULL);
	cache L1(L1Size,BSize,WrAlloc,MemCyc,L1Cyc,L1Assoc,&L2);
	L2.set_higher(&L1);

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
		//cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;
		
		if(operation == 'r')
		{
			total_delay += L1.cache_read(num);
		}
		else
		{
			total_delay += L1.cache_write(num);
		}

		//L1.print();
		//L2.print();

	}

	double L1MissRate = L1.get_missRate();
	double L2MissRate = L2.get_missRate();
	double avgAccTime = 0;
	long double avgAccTime2 = 0;

	//L1.print();
	//L2.print();


	if (L1.get_accesses_num() != 0){
		avgAccTime = (double) total_delay / L1.get_accesses_num();
		avgAccTime2 = (long double) (L1.get_accesses_num()*L1Cyc + L2.get_accesses_num()*L2Cyc + L2.get_miss_num()*MemCyc ) / L1.get_accesses_num();
	}

	double finalAvgAccTime2 = (double)((int)(avgAccTime2 * 1000 + 0.5)) / 1000.0;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	//printf("AccTimeAvg=%.03f\n", avgAccTime);
	printf("AccTimeAvg=%.03f\n", finalAvgAccTime2);

	return 0;
}
