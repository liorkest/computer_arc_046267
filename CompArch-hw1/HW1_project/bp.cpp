/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h" 
#include <stdio.h>
#include <vector>
#include <math.h>


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
	public:
		history_record(unsigned historySize, unsigned fsmState) // in case of local bimodal counters
		{
			history = 0x0;
			_bimodal_state_vector = init_fsm_vec(historySize, fsmState);
		}
		history_record(unsigned historySize, unsigned fsmState, std::vector<bimodal_FSM> *global_bimodal_state_vector) // in case of global bimodal counters
		{
			history = 0x0;
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

		bool predict (uint32_t pc, share_use_method share_use)
		{
			/* need to implement forr all share method options*/
			return true;
		}

};


struct btb_record
{
	/* This is a data structure for a single record in the BTB */
	uint32_t tag;
	unsigned tag_size; // tag field size in bits
	uint32_t dst_addr;
	history_record * history_record_ptr;
};

class branch_predictor
{
	private:
		std::vector<btb_record> BTB_table; 
		short **bimodal_counters; 
		unsigned btbSize, historySize, tagSize;
		bool isGlobalHist, isGlobalTable;
		share_use_method share_use;

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
	/*
	*
		< - TBD here : check if all the parameters have valid values
	*
	*/
	share_use = (share_use_method)Shared;

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
			history_record_ptr_global = new history_record(historySize, fsmState, bimodal_state_vector_global);
		else
			history_record_ptr_global = new history_record(historySize, fsmState);
	}

	// generate the BTB table
	for (unsigned int i=0; i<btbSize; i++) 
	{
		btb_record new_record;
		new_record.tag_size = tagSize;
		new_record.dst_addr = 0x0;
		if (isGlobalHist) { // one history for all
			new_record.history_record_ptr = history_record_ptr_global;
		} else { // seperate history for each record
			if (isGlobalTable)
				new_record.history_record_ptr = new history_record(historySize, fsmState, bimodal_state_vector_global);
			else
				new_record.history_record_ptr = new history_record(historySize, fsmState);
		}
		new_record.tag = 0x0;

		BTB_table.push_back(new_record);
	}

	printf("Hello");
	return 0;
}

bool branch_predictor::BP_predict(uint32_t pc, uint32_t *dst)
{
	bool prediction = false; 
	dst = new uint32_t(0x32);
	return prediction;
}

void branch_predictor::BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{

}

void branch_predictor::BP_GetStats(SIM_stats *curStats)
{

}




// Functions implementation for the given interface:
branch_predictor bp;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return bp.init( btbSize,  historySize,  tagSize,  fsmState,isGlobalHist,  isGlobalTable,  Shared);
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	// TBD
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t S){
		// TBD

	return;
}

void BP_GetStats(SIM_stats *curStats){
		// TBD

	return;
}


