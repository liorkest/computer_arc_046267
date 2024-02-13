/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h" 
#include <stdio.h>

/* Constants definitions */
const int ADDR_SIZE = 32;


struct btb_record
{
	/* This is a single record in the BTB */
	int tag;
	uint32_t dst_addr;
	int * history;
};

enum share_use_method { not_using_share = 0, using_share_lsb = 1, using_share_mid = 2 };

class branch_predictor
{
	private:
		btb_record *BTB_table; 
		unsigned *history;
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
		TBD here : check if all the parameters have valid values
	*
	*/
	share_use = (share_use_method)Shared;
	unsigned *BTB_table = new unsigned[btbSize];
	for (int i=0; i<btbSize; i++) {
		/* INITIALIZE */
	}
	printf("Hello");
	return 0;
}

bool branch_predictor::BP_predict(uint32_t pc, uint32_t *dst)
{

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
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}


