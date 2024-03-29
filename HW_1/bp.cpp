/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h" 
#include <stdio.h>
#include <vector>
#include <math.h>
#include<iterator> // for iterators
#include <bitset>
#include <iostream>
#include <cstdint>


/* Constants and enums definitions */
const int ADDR_SIZE = 32;
enum share_use_method { not_using_share = 0, using_share_lsb = 1, using_share_mid = 2 };
enum bimodal_state { SNT = 0, WNT = 1, WT = 2, ST = 3};

class bimodal_FSM
{
	/*
		This class represents a single bimodal FSM
	*/
	private:
		int _s;
		int _default;
	public:
		bimodal_FSM() {	
			_s = WNT; // just a default	
			printf("Deafult constructor!!!\n");
		}
		bimodal_FSM(int s) {
			_s = s;
			_default = s;
		}
		void change_state_NT() { _s--; if (_s < 0) _s = 0; }
		void change_state_T() { _s++; if (_s > 3) _s = 3; }
		bool get_decision() {  return (_s >= 2); }
		void reset_to_default() {
			_s = _default;
		}
		void print(){
			printf("	curr state = %d, ", _s);
		}
};

class history_record
{
	/*
		This class represents a single history register, which is linked to FSMs
	*/
	private:
		uint32_t history; // binary history 0101 = NT,T,NT,T lsb is most recent
		share_use_method _share_use;
		unsigned _historySize;
		bool  isGlobalTable;
	public:
		//initialize history record
		history_record(unsigned historySize, share_use_method share_use, bool is_GlobalTable)
		{
			isGlobalTable = is_GlobalTable;
			history = 0u;
			_share_use = share_use;
			_historySize = historySize;
		}

		void update_record(uint32_t pc, bool taken)
		{
			// update history
			history = history << 1; //remove MSB bit, add 0 at LSB
			uint32_t hist_bitmask = (1 << _historySize) - 1; // Create bitmask to extract historySize lower bits
			if(taken) //change LSB to '1' in case of branch taken
				history = history + 1u;
			history = history & hist_bitmask; //remove additonal bits.
		}

		unsigned int get_fsm_index(uint32_t pc) {
			uint32_t row_index ;
			uint32_t bitmask = (1 << _historySize) - 1; // Create bitmask to extract historySize lower bits
			
			if (_share_use == using_share_lsb && isGlobalTable) {
				uint32_t pc_share_bits = (pc >> 2); // extract the relevant bits for lsb share
				row_index = pc_share_bits ^ history; // Perform XOR between pc_share_bits and the history
			} else if (_share_use == using_share_mid  && isGlobalTable) {
				uint32_t pc_share_bits = (pc >> 16); // extract the relevant bits for mid share
				row_index = pc_share_bits ^ history; // Perform XOR between pc_share_bits and the history
			}
			else {
				row_index = history;// fsm_idx = find_btb_idx(uint32_t pc); // Assuming find_btb_idx is another function that computes an index
			}
			uint32_t fsm_index = row_index & bitmask ;

			return fsm_index; // Return only the historySize lower bits of the result
		}

		void print(){
			printf("	history value: %d\n", history);
		}

		void reset_hist() {history = 0u;}

};

class btb_record
{
	/* This is a data structure for a single record in the BTB */
	uint32_t tag; // tag associated with the branch instruction's address.
	unsigned tag_size; // tag field size in bits
	uint32_t dst_addr; // the target address of the branch instruction.
	bool isGlobalHist;
	bool isGlobalTable;
	bool valid;
	share_use_method _share_use;
	unsigned _historySize;
	static int hist_global_ptr_ref_count;
	history_record * history_record_ptr; // pointer to object which contains information about the branch history.
	std::vector<bimodal_FSM> * _bimodal_state_vector;
	static int bimodal_global_ptr_delete_count;

	public:
	//initialize btb rec
	btb_record(unsigned tag_size, bool isGlobalHist, bool isGlobalTable, unsigned history_size, share_use_method share_use, uint32_t fsmState) {
		btb_record::isGlobalTable = isGlobalTable;
		valid = false;
		_share_use = share_use;
		_historySize = history_size;
		btb_record::tag_size = tag_size;
		btb_record::isGlobalHist = isGlobalHist;
		if(!isGlobalTable){
			_bimodal_state_vector = init_fsm_vec(history_size, fsmState);
		}
		if(!isGlobalHist)
			btb_record::history_record_ptr = new history_record(history_size, share_use, isGlobalTable);
	}
	//initialize fsm vec
	std::vector<bimodal_FSM> static * init_fsm_vec(unsigned historySize, unsigned fsmState)
	{ // this is a helper function, that creates a binomal counters vector, in the size needed according to history size
		// returns a pointer to this vector
		// this function is called once (if global) or multiple times by each history record (if local)
		size_t bimodal_state_vector_size = pow(2, historySize);
		std::vector<bimodal_FSM> * vec = new std::vector<bimodal_FSM>();
		for (unsigned int i=0; i<bimodal_state_vector_size; i++)
		{
			bimodal_FSM b(fsmState);
			vec->push_back(b);
		}
		//printf("expected: %d \n real std::vector<bimodal_FSM> size = %d\n", (int)bimodal_state_vector_size, (int)vec->size());
		return vec;
	}

	void reset_record(uint32_t pc)
	{
		if (!isGlobalHist) // if local history, reset the history register
			history_record_ptr->reset_hist();
		//reset all vector in fsm table
		if(!isGlobalTable){
			for (unsigned int i=0; i < _bimodal_state_vector->size(); i++)
				(*_bimodal_state_vector)[i].reset_to_default();
		}

	}
	void set_hist_ptr(history_record * history_record_ptr) {btb_record::history_record_ptr = history_record_ptr; }
	void set_table_ptr(std::vector<bimodal_FSM> * table_ptr) {btb_record::_bimodal_state_vector = table_ptr; }
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t curr_cmd_tag)
	{
		if (valid) // record is already initialized
		{
			if (tag == curr_cmd_tag) 
			{
				update_table(taken, pc);
				history_record_ptr->update_record(pc, taken);
			}
			else // overwrite the current record!
			{
				tag = curr_cmd_tag;
				reset_record(pc);
				update_table(taken, pc);
				history_record_ptr->update_record(pc, taken);
			}
		} 
		else // totally new record :)
		{
			tag = curr_cmd_tag;
			valid = true;
			update_table(taken, pc);
			history_record_ptr->update_record(pc, taken);
		}
		dst_addr = targetPc;
	}

	void update_table(bool taken, uint32_t pc){
		// update FSM
		uint32_t fsm_idx = history_record_ptr->get_fsm_index(pc);
		if (taken)
			(*_bimodal_state_vector)[fsm_idx].change_state_T();
		else 
			(*_bimodal_state_vector)[fsm_idx].change_state_NT();
	}

	bool predict(uint32_t pc, uint32_t * dst, uint32_t tag_index){
		if (!history_record_ptr) { // Check if the pointer is valid
			printf("No history ptr!\n");
			return false;
		}
		if(!valid  || tag_index != tag ){
			//printf("notag or no valid\n");
			*dst = pc + 4;
			return false;
		}
		uint32_t fsm_idx = history_record_ptr->get_fsm_index(pc);
		//in case of local hist, local fsm
		if (!_bimodal_state_vector) // Check if the FSM vector pointer is valid
			return false;
		bool prediction = (*_bimodal_state_vector)[fsm_idx].get_decision();
		// in case of Taken update according to table, else update as pc+4
		if (prediction)
			*dst = dst_addr ;
		else
			*dst = pc + 4;
		return prediction;
	}
	//print function
	void print(){
		printf("	valid: %d, tag: %x, dst: %x\n", valid, tag, dst_addr);
		history_record_ptr->print();
		printf("	FSM table:\n:");
		for(unsigned int i=0;i<_bimodal_state_vector->size(); i++){
			(*_bimodal_state_vector)[i].print();
		}
		printf("\n");
	}
	void free_mem() {
		if (!isGlobalTable)
			delete _bimodal_state_vector;
		else if(bimodal_global_ptr_delete_count == 0)// global FSM vector
		{
			delete _bimodal_state_vector;
			bimodal_global_ptr_delete_count = 1;
		}
		if(!isGlobalHist)
			delete history_record_ptr;
		if (isGlobalHist && hist_global_ptr_ref_count == 0) {
			delete history_record_ptr;
			btb_record::hist_global_ptr_ref_count = 1;
		}
	}
};
int btb_record::bimodal_global_ptr_delete_count = 0;
int btb_record::hist_global_ptr_ref_count = 0;
class branch_predictor
{
	private:
		std::vector<btb_record> BTB_table;  
		unsigned btbSize, historySize, tagSize;
		bool isGlobalHist, isGlobalTable;
		share_use_method share_use;
		uint32_t find_tag(uint32_t pc);
		int flush_num, branch_num, total_size;
	public:
		branch_predictor()	{}
		int init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
		bool BP_predict(uint32_t pc, uint32_t *dst);
		void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		void BP_GetStats(SIM_stats *curStats);
		uint32_t find_btb_idx(uint32_t pc);
		void BP_Print_BTB_data(){
			
			printf ("\n~BTB DATA:~\n");
			for(unsigned i; i < btbSize; i++){
				printf ("	BTB record number %d:\n", i);
				BTB_table[i].print();
			}
		}
		
};

int branch_predictor::init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	branch_num = 0;
	// Parameter validation
    if (btbSize != 1 && btbSize != 2 && btbSize != 4 && btbSize != 8 && btbSize != 16 && btbSize != 32) {
        printf("Error: Invalid BTB size. BTB size must be 1, 2, 4, 8, 16, or 32.\n");
        return -1;
    }
    if (historySize < 1 || historySize > 8) {
        printf("Error: Invalid history size. History size must be between 1 and 8 (inclusive).\n");
        return -1;
    }
	unsigned maxTagSize = 30 - log2(btbSize);
	if (tagSize > maxTagSize) {
		printf("Error: Invalid tag size. Tag size cannot exceed 30 - log2(btbSize).\n");
		return -1;
	}
	 if (fsmState > 3 || fsmState < 0) {
        printf("Error: Invalid FSM state. FSM state must be between 0 and 3.\n");
        return -1;
    }

	branch_predictor::btbSize = btbSize;
	share_use = (share_use_method)Shared;
	branch_predictor::historySize = historySize;
	branch_predictor::tagSize = tagSize;
	branch_predictor::isGlobalHist = isGlobalHist;
	branch_predictor::isGlobalTable = isGlobalTable;
		
	// create global registers if needed
	std::vector<bimodal_FSM> * bimodal_state_vector_global;
	if (isGlobalTable) {
		// one FSM table for all history registers
		bimodal_state_vector_global = btb_record::init_fsm_vec(historySize, fsmState);
	}
	history_record *history_record_ptr_global;
	if (isGlobalHist) { 
		// one history for all
		history_record_ptr_global = new history_record(historySize, share_use, isGlobalTable);
	}

	// generate the BTB table
	for (unsigned int i=0; i<btbSize; i++) 
	{
		btb_record new_record(tagSize, isGlobalHist, isGlobalTable, historySize, share_use, fsmState);

		if (isGlobalHist)  // one history for all
			new_record.set_hist_ptr(history_record_ptr_global);
		if (isGlobalTable)  // one table for all
			new_record.set_table_ptr(bimodal_state_vector_global);
		BTB_table.push_back(new_record);
	}

	return 0;
}

//function recives pc, returns the BTB idx in that adress, Tested
uint32_t branch_predictor::find_btb_idx(uint32_t pc)
{
	//find btb index using mask
    unsigned int num_btb_bits =  log2(btbSize);
	if(num_btb_bits == 0){
		return 0;
	}
	uint32_t btb_mask = (1u << num_btb_bits) - 1;	// Create a mask with 1s in the relevant bits
	uint32_t btbBits = (pc >> 2) & btb_mask; // Apply the mask to pc after shifting to skip the 2 least significant bits
	uint32_t btb_index = btbBits;
	return btb_index;
}

//function recives pc, returns the BTB idx in that adress
uint32_t branch_predictor::find_tag(uint32_t pc){
	//find tag index using mask
	if(tagSize==0)
		return 0;
	unsigned int num_btb_bits =  log2(btbSize);
	uint32_t tag_mask = (1u << tagSize) - 1  ;
	uint32_t tag_bits = (pc >> (2+ num_btb_bits)) & tag_mask;
	int32_t tag_index = tag_bits; // Convert into numbers
	return tag_index;
}

bool branch_predictor::BP_predict(uint32_t pc, uint32_t *dst)
{

	branch_num++;

	uint32_t btb_index = find_btb_idx(pc);
	uint32_t tag_index = find_tag(pc);

	if (btb_index > btbSize) // Check if btb_idx is within bounds of BTB_table
	{
		printf("BTB index out of bounds");
		return false;
	}

	return BTB_table[btb_index].predict(pc, dst, tag_index);
}

void branch_predictor::BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{


	if( ((targetPc != pred_dst) && taken) || ((pred_dst != pc + 4) && !taken) ) {
		flush_num++;
	} 
	//printf("targetPc: 0x%x ,pred_dst: 0x%x, taken: %d " , targetPc,pred_dst,taken);

	int index = find_btb_idx(pc);
	BTB_table[index].update(pc, targetPc, taken, find_tag(pc));

}

void branch_predictor::BP_GetStats(SIM_stats *curStats)
{
	curStats->br_num = branch_num;
	curStats->flush_num = flush_num;
	int total_size;
	if (isGlobalHist)
	{
		if (isGlobalTable)
			total_size = btbSize * (tagSize + 31 /*ADDR_SIZE*/) + historySize + 2 * pow(2, historySize);
		else // global history, local table
			total_size = historySize + btbSize * (tagSize + 31 /*ADDR_SIZE*/) + btbSize * 2 * pow(2, historySize);
	}
	else
	{	
			if (isGlobalTable)
				total_size = btbSize * (tagSize + 31 /*ADDR_SIZE*/+ historySize )  + 2 * pow(2, historySize);
			else	 // local hist local table     
				total_size = btbSize * (tagSize + 31 /*ADDR_SIZE*/ + historySize) +  btbSize *2 * pow(2, historySize);
	}
	curStats->size = total_size;

	for(unsigned  i = 0; i < btbSize; i++){
		BTB_table[i].free_mem();
	}
}



// Functions implementation for the given interface:
branch_predictor bp;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return bp.init( btbSize,  historySize,  tagSize,  fsmState,isGlobalHist,  isGlobalTable,  Shared);
}


bool BP_predict(uint32_t pc, uint32_t *dst){
	bool prediction = bp.BP_predict(pc, dst);
	return prediction;
	
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	bp.BP_update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	bp.BP_GetStats(curStats);
}


