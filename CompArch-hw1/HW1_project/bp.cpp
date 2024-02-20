/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h" 
#include <stdio.h>
#include <vector>
#include <math.h>
#include<iterator> // for iterators

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
		}
		bimodal_FSM(unsigned int s) {
			_s = (int)s;
			_default = (int)s;
		}
		void set_state(bimodal_state s){
			_s = (int)s;
		}
		void change_state_NT() { _s--; if (_s < 0) _s = 0; }
		void change_state_T() { _s++; if (_s > 3) _s = 3; }
		bool get_decision() { return (_s >= 2); }
		void reset_to_default() {
			_s = _default;
		}
};

class history_record
{
	/*
		This class represents a single history register, which is linked to FSMs
	*/
	private:
		uint16_t history; // binary history 0101 = NT,T,NT,T lsb is most recent
		std::vector<bimodal_FSM> * _bimodal_state_vector; // a pointer to a global or private FSM vector
		share_use_method _share_use;
		bool _isGlobalTable;
	public:
		unsigned int get_fsm_index(uint32_t pc) 
		{
			if(_share_use == using_share_lsb)
			{
				uint32_t pc_share_bits = (pc >> 2) ; // extract the relevent bits for lsb share

				uint16_t row_index = pc_share_bits ^ history; // Perform XOR between pc_share_bit and the history
				return row_index;
			}
			else if(_share_use == using_share_mid)
			{
				uint32_t pc_share_bits = (pc >> 16) ; // extract the relevent bits for lsb share
				uint16_t row_index = pc_share_bits ^ history; // Perform XOR between pc_share_bit and the history
				return row_index;
			}
			else 
			{
				if (_isGlobalTable)
				{
					return 0; //only one value in fsm vector
				}
				else
					//fsm_idx = find_btb_idx(uint32_t pc);
					return history; 
			}
		}

		history_record(unsigned historySize, unsigned fsmState, share_use_method share_use, bool isGlobalTable) // in case of local bimodal counters
		{
			history = 0x0;
			_share_use = share_use;
			_isGlobalTable = isGlobalTable;
			_bimodal_state_vector = init_fsm_vec(historySize, fsmState);
		}
		history_record(unsigned historySize, unsigned fsmState, std::vector<bimodal_FSM> *global_bimodal_state_vector, share_use_method share_use, bool isGlobalTable) // in case of global bimodal counters
		{
			history = 0x0;
			_share_use = share_use;
			_isGlobalTable = isGlobalTable;
			_bimodal_state_vector = global_bimodal_state_vector;
		}

		std::vector<bimodal_FSM> static * init_fsm_vec(unsigned historySize, unsigned fsmState)
		{ // this is a helper function, that creates a binomal counters vector, in the size needed according to history size
		  // returns a pointer to this vector
		  // this function is called once (if global) or multiple times by each history record (if local)
			size_t bimodal_state_vector_size = pow(2, historySize);
			std::vector<bimodal_FSM> *vec = new std::vector<bimodal_FSM>(bimodal_state_vector_size);
			for (unsigned int i=0; i<bimodal_state_vector_size; i++)
			{
				bimodal_FSM fsm(fsmState);
				vec->push_back(fsm);
			}
			return vec;
		}

		bool predict (uint32_t pc)
		{
			uint32_t fsm_idx = get_fsm_index(pc);
						
			//in case of local hist, local fsm
			if (!_bimodal_state_vector) // Check if the FSM vector pointer is valid
				return false;
			return (*_bimodal_state_vector)[fsm_idx].get_decision();		
		}

		void update_record(uint32_t pc, bool taken)
		{
			uint32_t fsm_idx =  get_fsm_index(pc);
			// update FSM
			if (taken)
				(*_bimodal_state_vector)[fsm_idx].change_state_T();
			else
				(*_bimodal_state_vector)[fsm_idx].change_state_NT();
			// update history
			history = history << 1;
			if(taken)
				history = history + 1u;
		}

};


struct btb_record
{
	/* This is a data structure for a single record in the BTB */
	uint32_t tag; // tag associated with the branch instruction's address.
	unsigned tag_size; // tag field size in bits
	uint32_t dst_addr; // the target address of the branch instruction.
	history_record * history_record_ptr; // pointer to object which contains information about the branch history.
};

class branch_predictor
{
	private:
		std::vector<btb_record> BTB_table;  
		short **bimodal_counters; 
		unsigned btbSize, historySize, tagSize;
		bool isGlobalHist, isGlobalTable;
		share_use_method share_use;
		uint32_t find_btb_idx(uint32_t pc);
		uint32_t find_tag_idx(uint32_t pc);
		int flush_num, branch_num, total_size;
	public:
		branch_predictor()	{}
		int init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
		bool BP_predict(uint32_t pc, uint32_t *dst);
		void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		void BP_GetStats(SIM_stats *curStats);
};

int branch_predictor::init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	branch_num = 0;
	// Parameter validation
    if (btbSize != 1 && btbSize != 2 && btbSize != 4 && btbSize != 8 && btbSize != 16 && btbSize != 32) {
        printf("Error: Invalid BTB size. BTB size must be 1, 2, 4, 8, 16, or 32.\n");
        return 0;
    }
    if (historySize < 1 || historySize > 8) {
        printf("Error: Invalid history size. History size must be between 1 and 8 (inclusive).\n");
        return 0;
    }
	unsigned maxTagSize = 30 - log2(btbSize);
	if (tagSize > maxTagSize) {
		printf("Error: Invalid tag size. Tag size cannot exceed 30 - log2(btbSize).\n");
		return 0;
	}
	 if (fsmState > 3 || fsmState < 0) {
        printf("Error: Invalid FSM state. FSM state must be between 0 and 3.\n");
        return 0;
    }

	branch_predictor::btbSize = btbSize;
	share_use = (share_use_method)Shared;
	branch_predictor::historySize = historySize;
	branch_predictor::tagSize = tagSize;
	branch_predictor::isGlobalHist = isGlobalHist;
	branch_predictor::isGlobalTable = isGlobalTable;
	
	// BTB init:
	BTB_table = std::vector<btb_record>();
	
	// create global registers if needed
	std::vector<bimodal_FSM> * bimodal_state_vector_global;
	if (isGlobalTable) {
		// one FSM table for all history registers
		bimodal_state_vector_global = history_record::init_fsm_vec(historySize, fsmState);
	}
	history_record *history_record_ptr_global;
	if (isGlobalHist) { 
		// one history for all
		if (isGlobalTable)
			history_record_ptr_global = new history_record(historySize, fsmState, bimodal_state_vector_global, share_use, isGlobalTable);
		else
			history_record_ptr_global = new history_record(historySize, fsmState, share_use, isGlobalTable);
	}

	// generate the BTB table
	for (unsigned int i=0; i<btbSize; i++) 
	{
		btb_record new_record;
		new_record.tag_size = tagSize;
		new_record.tag = 0x0;
		new_record.dst_addr = 0x0;
		if (isGlobalHist) { // one history for all
			new_record.history_record_ptr = history_record_ptr_global;
		} else { // seperate history for each record
			if (isGlobalTable)
				new_record.history_record_ptr = new history_record(historySize, fsmState, bimodal_state_vector_global, share_use, isGlobalTable);
			else
				new_record.history_record_ptr = new history_record(historySize, fsmState, share_use, isGlobalTable);
		}

		BTB_table.push_back(new_record);
	}

	printf("Finished initialization of BTB. length = %d\n", BTB_table.size());
	return 0;
}

//function recives pc, returns the BTB idx in that adress, Tested
uint32_t branch_predictor::find_btb_idx(uint32_t pc){
	//find btb index using mask
    unsigned int num_btb_bits = (btbSize == 1) ? 1 : log2(btbSize);
	uint32_t btb_mask = (1u << num_btb_bits) - 1;	// Create a mask with 1s in the relevant bits
	uint32_t btbBits = (pc >> 2) & btb_mask; // Apply the mask to pc after shifting to skip the 2 least significant bits
	uint32_t btb_index = btbBits;
	return btb_index;
}

//function recives pc, returns the BTB idx in that adress
uint32_t branch_predictor::find_tag_idx(uint32_t pc){
	//find tag index using mask
	 unsigned int num_btb_bits = (btbSize == 1) ? 1 : log2(btbSize);
	uint32_t tag_mask = ((1u << tagSize) - 1)  ;
	uint32_t tag_bits = (pc >> (2+ num_btb_bits)) & tag_mask;
	int32_t tag_index = tag_bits; // Convert into numbers
	return tag_index;
}





bool branch_predictor::BP_predict(uint32_t pc, uint32_t *dst)
{
	branch_num++;
	
	uint32_t btb_index = find_btb_idx(pc);
	uint32_t tag_index = find_tag_idx(pc);

	
	if (btb_index >= btbSize) // Check if btb_idx is within bounds of BTB_table
	{
		printf("BTB index out of bounds");
		return false;
	}
	if (BTB_table[btb_index].tag != tag_index)  //In case the branch is unrecognized 
	{
		*dst = pc + 4;
		return false;
	}

	history_record* record = BTB_table[btb_index].history_record_ptr;
	if (!record) { // Check if the pointer is valid
		printf("No history ptr!\n");
		return false;
	}
	bool prediction = record->predict(pc);
	// in case of Taken update according to table, else update as pc+4
	if (prediction)
		*dst = BTB_table[btb_index].dst_addr ;
	else
		*dst = pc + 4;

	return prediction;
}

void branch_predictor::BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	int index = find_btb_idx(pc);
	printf("PC: 0x%x, Target PC: 0x%x, Predicted Dest Addr: 0x%x, Taken: %s, Flush Num: %d\n", pc, targetPc, pred_dst, taken ? "True" : "False", flush_num);
	if( ((targetPc != pred_dst) && taken) || ((pred_dst != pc + 4) && !taken) ) {
		flush_num++;
	} 
	BTB_table[index].dst_addr = targetPc;
	BTB_table[index].history_record_ptr->update_record(pc, taken);
}

void branch_predictor::BP_GetStats(SIM_stats *curStats)
{
	curStats->br_num = branch_num;
	curStats->flush_num = flush_num;
	int total_size;
	if (isGlobalHist)
		if (isGlobalTable)
			total_size = btbSize * (tagSize + 31 /*ADDR_SIZE*/) + historySize + 2 * pow(2, historySize);
		else // global history, local table
			total_size = historySize + btbSize * (tagSize + 31 /*ADDR_SIZE*/) + btbSize * 2 * pow(2, historySize);
	else // local hist local table
		total_size = btbSize * (tagSize + 31 /*ADDR_SIZE*/ + historySize) + btbSize * 2 * pow(2, historySize);

	curStats->size = total_size;
}
/*
	std::vector<btb_record>::iterator ptr; 
	if (isGlobalHist)
		delete BTB_table[0].history_record_ptr;
	else
		for (ptr = BTB_table.begin(); ptr < BTB_table.end(); ptr++) {
				delete ptr->history_record_ptr;
		}
}

*/


// Functions implementation for the given interface:
branch_predictor bp;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return bp.init( btbSize,  historySize,  tagSize,  fsmState,isGlobalHist,  isGlobalTable,  Shared);
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return bp.BP_predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	bp.BP_update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	bp.BP_GetStats(curStats);


}


