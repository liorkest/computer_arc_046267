/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>

class thread{
	int tid;
	tcontext register_file;
	int inst_line_number;
	bool halted;
	int thread_timer = 0;
public:
	void init_thread(int tid)
	{
		halted = false;
		thread::tid = tid;
		for (int i=0; i<REGS_COUNT; i++){register_file.reg[i]=0;}
		inst_line_number = 0;
	}
	bool is_halted() {return halted;}

	void clk_cycle_passed()
	{
		if(thread_timer!=0)
			thread_timer--;	
	}
	bool is_paused(){
		return thread_timer > 0;
	}

	int execute_next_cmd(){
		int delay = 0;

		Instruction curr_inst;
		SIM_MemInstRead(inst_line_number, &curr_inst, tid);
		switch (curr_inst.opcode)
		{
		case CMD_NOP:
			delay++;
			break;
		case CMD_ADD:
			delay++;
			register_file.reg[curr_inst.dst_index] = register_file.reg[curr_inst.src1_index] + register_file.reg[curr_inst.src2_index_imm];
			break;
		case CMD_SUB:
			delay++;
			register_file.reg[curr_inst.dst_index] = register_file.reg[curr_inst.src1_index] - register_file.reg[curr_inst.src2_index_imm];
			break;
		case CMD_ADDI:
			delay++;
			register_file.reg[curr_inst.dst_index] = register_file.reg[curr_inst.src1_index] + curr_inst.src2_index_imm;
			break;
		case CMD_SUBI:
			delay++;
			register_file.reg[curr_inst.dst_index] = register_file.reg[curr_inst.src1_index] - curr_inst.src2_index_imm;
			break;
		case CMD_LOAD:
			thread_timer = SIM_GetLoadLat();
			delay = delay + SIM_GetLoadLat() ; // DELAY MIGHT BE LOWER FOR THREAD SWITCHING!!
			uint32_t load_addr;
			//if Imm command
			if(curr_inst.isSrc2Imm){
				load_addr =(uint32_t) register_file.reg[curr_inst.src1_index] + curr_inst.src2_index_imm;
			}
			else{
				load_addr =(uint32_t) register_file.reg[curr_inst.src1_index] +  register_file.reg[curr_inst.src2_index_imm];
			}
			//load addr after calc
			int32_t data;
			SIM_MemDataRead(load_addr,&data);
			register_file.reg[curr_inst.dst_index] = data;
			break;
		case CMD_STORE:
			thread_timer = SIM_GetStoreLat();
			delay = delay + SIM_GetStoreLat() ; // DELAY MIGHT BE LOWER FOR THREAD SWITCHING!!
			uint32_t store_addr;
			if(curr_inst.isSrc2Imm){
				store_addr =(uint32_t) register_file.reg[curr_inst.src1_index] + curr_inst.src2_index_imm;
			}
			else{
				store_addr =(uint32_t) register_file.reg[curr_inst.src1_index] +  register_file.reg[curr_inst.src2_index_imm];
			}
			SIM_MemDataWrite(store_addr,register_file.reg[curr_inst.src1_index]);
			break;	
		case CMD_HALT:
			delay++;
			halted = true;
			break;	
		}
	}
	void copy_context(tcontext* context)
	{
		std::memcpy(context, &register_file, sizeof(tcontext)); 
	}
};

class core {
	int threads_num;
	
	std::vector<thread> threads;
	int cycles, instructions;
	core() {
		cycles = 0;
		instructions = 0;
		threads_num = SIM_GetThreadsNum();
		for (int i=0; i<threads.size(); i++) {
			threads[i].init_thread(i);
		}
	}
	void clock_tick(){
		for(auto t : threads){
			t.clk_cycle_passed();
		}
	}
	
	public:
	void CORE_BlockedMT()
	{
		bool finished = false;
		int curr_tid = 0;
		while(!finished)
		{
			cycles += SIM_GetSwitchCycles();
			cycles += threads[curr_tid].execute_next_cmd();
			curr_tid = get_next_thread(curr_tid);
			if (curr_tid == -1)
				finished = true;
			clock_tick();
		}
		
	}
	void CORE_FinegrainedMT()
	{
		
	}
	
	void copy_context(tcontext* context, int threadid)
	{
		threads[threadid].copy_context(context);
	}

	int get_next_thread(int curr_tid)
	{
		bool active_thread_exists = false;
		for (int i=0; i<threads.size(); i++)
		{
			int next_tid=(curr_tid+i)%threads.size();
			if (!threads[next_tid].is_halted())
			{ // it needs to be executed next!
				active_thread_exists = true;
			} 
			if (!threads[next_tid].is_paused())
			{ // it needs to be executed next!
				active_thread_exists = true;
				return next_tid;
			} 
		}
		if (!active_thread_exists)
			return -1; // all threads finished!
	
	}
	int get_cycles(){return cycles;}
	int get_instructions(){return instructions;}

};


// instantiation of the class
core core_inst;

void CORE_BlockedMT() {
	core_inst.CORE_BlockedMT();
}

void CORE_FinegrainedMT() {
	core_inst.CORE_FinegrainedMT();
}

double CORE_BlockedMT_CPI(){
	return (double)core_inst.get_cycles()/core_inst.get_instructions();
}

double CORE_FinegrainedMT_CPI(){
	return (double)core_inst.get_cycles()/core_inst.get_instructions();
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	core_inst.copy_context(context, threadid);
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	core_inst.copy_context(context, threadid);
}
