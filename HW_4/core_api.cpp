/* 046267 Computer Architecture - HW #4 LIOR and LEV */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>
#include <cstring>

class thread{
	int tid;
	tcontext register_file;
	int inst_line_number;
	bool halted;
	int thread_timer ;
public:
	void init_thread(int tid)
	{
		halted = false;
		thread::tid = tid;
		for (int i=0; i<REGS_COUNT; i++){register_file.reg[i]=0;}
		inst_line_number = 0;
		thread_timer = 0;
	}
	bool is_halted() {return halted;}

	void clk_cycle_passed(int cycles_elapsed=1)
	{
		if (thread_timer > cycles_elapsed)
			thread_timer-=cycles_elapsed;	
		else
			thread_timer=0;
	}
	bool is_paused(){
		return thread_timer > 0;
	}
	int get_tid(){return tid;}

	void execute_next_cmd(){
		int delay = 0;
		Instruction curr_inst;
		SIM_MemInstRead(inst_line_number, &curr_inst, tid);
		if (tid == 2){
			print_thread_status();
		}

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
			thread_timer =  1 +  SIM_GetLoadLat();
			delay =  SIM_GetLoadLat() ; // DELAY MIGHT BE LOWER FOR THREAD SWITCHING!!
			uint32_t load_addr;
			//if Imm command
			if(curr_inst.isSrc2Imm){
				load_addr =(uint32_t) register_file.reg[curr_inst.src1_index] + curr_inst.src2_index_imm;
			}
			else{
				load_addr =(uint32_t) register_file.reg[curr_inst.src1_index] +  register_file.reg[curr_inst.src2_index_imm];
			}
			load_addr = load_addr>>2; // align to reading frame
            load_addr = load_addr<<2;
			//load addr after calc
			int32_t data;
			SIM_MemDataRead(load_addr, &data);
			printf("LOAD tid %d: reg[%d]=%d\n", tid, curr_inst.dst_index, data);
			register_file.reg[curr_inst.dst_index] = data;
			break;

		case CMD_STORE: // Mem[dst + src2] <- src1  (src2 may be an immediate)
			thread_timer = 1 + SIM_GetStoreLat();
			delay = SIM_GetStoreLat() ; // DELAY MIGHT BE LOWER FOR THREAD SWITCHING!!
			uint32_t store_addr;
			if(curr_inst.isSrc2Imm){
				store_addr = (uint32_t) register_file.reg[curr_inst.src1_index] + curr_inst.src2_index_imm;
			}
			else{
				store_addr = (uint32_t) register_file.reg[curr_inst.src1_index] +  register_file.reg[curr_inst.src2_index_imm];
			}
			store_addr = store_addr>>2; // align to reading frame
            store_addr = store_addr<<2;
			printf("STORE tid %d: MEM[%d]=%d\n", tid, store_addr, register_file.reg[curr_inst.src1_index]);
			SIM_MemDataWrite(store_addr,register_file.reg[curr_inst.src1_index]);
			break;	
		case CMD_HALT:
			delay++;
			halted = true;
			break;	
		}
		inst_line_number++;
		if (tid == 2){
			print_thread_status();
		}

	}

	void copy_context(tcontext* context)
	{
		for(int i=0;i<REGS_COUNT;i++){
			context[tid].reg[i] = register_file.reg[i];
		}

	}

	void print_thread_status(){
		printf("thread id: %d, thread instruction count: %d,thread timer: %d, is halted: %d, is paused: %d\n",tid,inst_line_number,thread_timer,halted, is_paused());
		for (int i=0;i<REGS_COUNT;i++) {
			printf("%d: %d;",register_file.reg[i]);
		}
		printf("\n");
	}

};


class core {
protected:
	int threads_num;
	std::vector<thread> threads;
	int cycles, instructions;
public:
	core() {}
	void init_core(){
		cycles = 0;
		instructions = 0;
		threads_num = SIM_GetThreadsNum();
		for (int i=0; i<threads_num; i++) {
			thread t;
			t.init_thread(i);
			threads.push_back(t);
			//printf("%d\n", threads[i].get_tid());
		}
	}
	void clock_tick(int cycles_elapsed=1){
		cycles+=cycles_elapsed;
		for(int i = 0; i< threads_num; i++){
			threads[i].clk_cycle_passed(cycles_elapsed);
		}
	}

	void copy_context(tcontext* context, int threadid)
	{
		threads[threadid].copy_context(context);
	}

	int get_cycles(){return cycles;}
	int get_instructions(){return instructions;}

	void print_all_threads()
	{
		for(int i=0; i< int(threads.size());i++){
			//printf("cycle num:%d\n , instruction count:%d",cycles,instructions);
			threads[i].print_thread_status();
		}
	}

};

class Blocked_core: public core
{
	public:
	void run_benchmark()
	{
		bool finished = false;
		int curr_tid = 0;
		int next_tid;
		int count = 20;
		while(!finished)
		{
			//printf("cycle: %d\n", cycles);
			next_tid = get_next_thread(curr_tid);
			//printf("Next tid: %d\n", next_tid);
			//print_all_threads();
			if (next_tid == -1) // all finisheds
			{
				finished = true;
			} 
			else if (next_tid == -2) // all are waiting
			{
 				//printf("All threads are stalled\n");
				clock_tick();

			} 
			else if (curr_tid == next_tid) 
			{
				threads[curr_tid].execute_next_cmd();
				clock_tick();
				instructions++;
			}
			else //switch to another
			{
				//printf("switching from %d to %d\n", curr_tid, next_tid);
				clock_tick(SIM_GetSwitchCycles());
				threads[next_tid].execute_next_cmd();
				clock_tick();
				instructions++;
				curr_tid = next_tid;
			}


			//threads[curr_tid].print_thread_status();		
		}
		
	}
	int get_next_thread(int curr_tid)
	{
		bool active_thread_exists = false;
		for (int i=0; i<int(threads.size()); i++)
		{
			int next_tid=(curr_tid+i)%threads.size();
			if (!threads[next_tid].is_halted())
			{ // it needs to be executed next!
				active_thread_exists = true;
			} 

			if (!threads[next_tid].is_halted() && !threads[next_tid].is_paused())
			{ // it needs to be executed next!
				active_thread_exists = true;
				return next_tid;
			} 
		}
		if (!active_thread_exists)
			return -1; // all threads finished!
		else
			return -2; // no free thread, need to stall

	}
};

class Finegrained_core: public core
{
	public:
	void run_benchmark()
	{
		bool finished = false;
		int curr_tid = 0;
		int next_tid;
		bool first_cycle = true;
		while(!finished)
		{

			if (first_cycle) {
				next_tid = 0;
				first_cycle = false;
			} else {
				next_tid = get_next_thread(curr_tid);
			}
			//printf("cycle: %d\n", cycles);
			//print_all_threads();
			if (next_tid == -1) // all finished
			{
				finished = true;
			} 
			else if (next_tid == -2) // all are waiting
			{
 				//printf("All threads are stalled\n");
				clock_tick();

			} 
			else if (curr_tid == next_tid)
			{
				threads[curr_tid].execute_next_cmd();

				instructions++;
				clock_tick();
			}
			else //switch to another
			{
				threads[next_tid].execute_next_cmd();
				instructions++;
				curr_tid = next_tid;
				clock_tick();
			}			
		}
	}
	

	int get_next_thread(int curr_tid)
	{
		bool active_thread_exists = false;
		for (int i=1; i<=int(threads.size()); i++)
		{
			int next_tid=(curr_tid+i)%threads.size();
			if (!threads[next_tid].is_halted())
			{ // it needs to be executed next!
				active_thread_exists = true;
			} 

			if (!threads[next_tid].is_halted() && !threads[next_tid].is_paused())
			{ // it needs to be executed next!
				active_thread_exists = true;
				return next_tid;
			} 
		}
		if (!active_thread_exists)
			return -1; // all threads finished!
		else
			return -2; // no free thread, need to stall

	}
};


// instantiation of the class
Blocked_core core_inst_BK;
Finegrained_core core_inst_FG;
void CORE_BlockedMT() {
	core_inst_BK.init_core();
	core_inst_BK.run_benchmark();
}

void CORE_FinegrainedMT() {
	core_inst_FG.init_core();
	core_inst_FG.run_benchmark();
}

double CORE_BlockedMT_CPI(){
	return (double)core_inst_BK.get_cycles()/core_inst_BK.get_instructions();
}

double CORE_FinegrainedMT_CPI(){
	return (double)core_inst_FG.get_cycles()/core_inst_FG.get_instructions();
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	core_inst_BK.copy_context(context, threadid);
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	core_inst_FG.copy_context(context, threadid);
}
