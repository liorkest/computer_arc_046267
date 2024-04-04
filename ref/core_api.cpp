/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include<vector>
using std::vector;

class Single_Thread
{
public:
    bool halt;
    int next_cycle;
    int end_line;
    tcontext registers_files;

    Single_Thread():halt(false), next_cycle(0), end_line(-1)
    {
        int i = 0;
        while (i < REGS_COUNT)
        {
            if(registers_files.reg[i] != NULL)
                registers_files.reg[i] = 0;
            i++;
        }
        
    }
    
    ~Single_Thread() = default;

    bool is_available(int curr_cycle)
    {
        if (!halt && curr_cycle >= next_cycle) 
        {
            return true;
        }
        else return false;
    }
};

class MT_sim
{
public:
    vector <Single_Thread*> threads;
    int cycles_num;  //number of cycles
	bool mt_type;  // 1 - blocked MT, 0-fine grain MT
    int instructions_num; 
    //int number_of_cycles_for_context_switch; // relevant only in blocked

	//Default Constructor
    MT_sim(bool mt_type) : cycles_num(0), mt_type(mt_type), instructions_num(0){};
	
	// Destructor
    ~MT_sim() 
	{
        for(int i=0; i < SIM_GetThreadsNum(); i++)
        {
            delete (threads[i]);
        }    
    }

    
    void initialize_sim()
    {
        for(int i=0; i < SIM_GetThreadsNum(); i++)
        {
            threads.push_back(new Single_Thread);
        }
    }

    bool treads_finished()
    {
        for(int i=0; i < SIM_GetThreadsNum(); i++)
        {
            if(!(threads[i]->halt)) return false;
        }
        return true;
    }

    int get_next_tread(int curr_thread_index)
    {
        Single_Thread* curr_thread = threads[curr_thread_index];
        if (mt_type)
        {
            if( curr_thread->is_available(cycles_num)) 
                return curr_thread_index;
        }
        int temp = curr_thread_index + 1;
        for (int i = temp % SIM_GetThreadsNum(); i != curr_thread_index; i = (i + 1) % SIM_GetThreadsNum())  
        {
            if (threads[i]->is_available(cycles_num)) 
                return i;
        }
        
        if (!mt_type)
        {
            if(curr_thread->is_available(cycles_num)) 
                return curr_thread_index;
        }
        
        return -1; // No available thread
    }


    bool arithmetic_instruction(Instruction instruction, Single_Thread &thread)
    {
        int destination_index = instruction.dst_index;
        int source1_index = instruction.src1_index;
        bool is_source2_is_imm = instruction.isSrc2Imm; // if it false so source2 is index
        int source2_index_or_imm = instruction.src2_index_imm;
        cmd_opcode opcode = instruction.opcode;

        if (opcode == CMD_NOP) {
            return true;
        }
        
        if (opcode == CMD_ADD) {
            thread.registers_files.reg[destination_index] = thread.registers_files.reg[source1_index] + thread.registers_files.reg[source2_index_or_imm];
            thread.next_cycle++;
            return true;
        }
        
        if (opcode == CMD_SUB) {
            thread.registers_files.reg[destination_index] = thread.registers_files.reg[source1_index] - thread.registers_files.reg[source2_index_or_imm];
            thread.next_cycle++;
            return true;
        }
        
        if (opcode == CMD_ADDI) {
            thread.registers_files.reg[destination_index] = thread.registers_files.reg[source1_index] + source2_index_or_imm;
            thread.next_cycle++;
            return true;
        }
        
        if (opcode == CMD_SUBI) {
            thread.registers_files.reg[destination_index] = thread.registers_files.reg[source1_index] - source2_index_or_imm;
            thread.next_cycle++;
            return true;
        }
        return false;
    }

    void instruction(Instruction instruction, Single_Thread &thread)
    {
        if (arithmetic_instruction(instruction, thread)) return;
        
        int destination_index = instruction.dst_index;
        int source1_index = instruction.src1_index;
        bool is_source2_is_imm = instruction.isSrc2Imm; // if it false so source2 is index
        int source2_index_or_imm = instruction.src2_index_imm;
        cmd_opcode opcode = instruction.opcode;
        
        if (opcode == CMD_LOAD)
        {
            uint32_t load_address;
            if(is_source2_is_imm) load_address = thread.registers_files.reg[source1_index] + source2_index_or_imm;
            else load_address = thread.registers_files.reg[source1_index] + thread.registers_files.reg[source2_index_or_imm];
            int32_t load_temp_destination;
            SIM_MemDataRead(load_address, &load_temp_destination);

            thread.registers_files.reg[destination_index] = load_temp_destination;
			printf("LOAD thread  reg[%d]=MEM[%d]=%d\n",  destination_index,load_address, load_temp_destination);
            thread.next_cycle = cycles_num + SIM_GetLoadLat() + 1;
            return;
        }
          
        if (opcode == CMD_STORE)
        {
            uint32_t store_address;
            if(is_source2_is_imm) store_address = thread.registers_files.reg[destination_index] + source2_index_or_imm;
            else store_address = thread.registers_files.reg[destination_index] + thread.registers_files.reg[source2_index_or_imm];
			printf("STORE thread  MEM[%d]=reg[%d]=%d\n", store_address, source1_index,thread.registers_files.reg[source1_index] );
            SIM_MemDataWrite(store_address, thread.registers_files.reg[source1_index]);
            thread.next_cycle = cycles_num + SIM_GetStoreLat() + 1;
			
            return;
        }

        if (opcode == CMD_HALT)
        {
            thread.halt = true;
            return;
        }

    }
    
    void blocked_sim_calc() // here we assume that the MT is blocked. SIM_GetSwitchCycles is allowed
    {
        initialize_sim();
        int last_thread_index = 0;
        Instruction curr_instruction;

        while(!treads_finished())
        {
            int curr_thread_index = get_next_tread(last_thread_index);

            if(curr_thread_index == -1){
                cycles_num++;
                continue;
            }
            if(curr_thread_index != last_thread_index)
            {
                int temp_switch_cycles = SIM_GetSwitchCycles();
                cycles_num = cycles_num + temp_switch_cycles;
            }

            //Thread* curr_thread = threads[curr_thread_index];
            threads[curr_thread_index]->end_line++;
            SIM_MemInstRead(threads[curr_thread_index]->end_line,&curr_instruction, curr_thread_index);

            instruction(curr_instruction, *threads[curr_thread_index]);
            
            instructions_num ++;
            cycles_num++;

            last_thread_index = curr_thread_index;
        }
    }

    void fine_grained_sim_calc() // here we assume that the MT is fine_grained
    {
        initialize_sim();
        int last_thread_index = -1;
        Instruction curr_instruction;

        while(!treads_finished())
        {
            int curr_thread_index = get_next_tread(last_thread_index);
            if(curr_thread_index == -1)
            {
                cycles_num++;
                continue;
            }

            //Thread* curr_thread = threads[curr_thread_index];
            threads[curr_thread_index]->end_line++;

            SIM_MemInstRead(threads[curr_thread_index]->end_line,&curr_instruction, curr_thread_index);
            instruction(curr_instruction, *threads[curr_thread_index]);
            
            instructions_num++;
            cycles_num++;

            last_thread_index = curr_thread_index;
        }
    }

};

MT_sim blocked = MT_sim(1);
MT_sim fine_grained = MT_sim(0);


void CORE_BlockedMT()
{
    blocked.blocked_sim_calc();
}

void CORE_FinegrainedMT()
{
    fine_grained.fine_grained_sim_calc();
}

double CORE_BlockedMT_CPI()
{
	return (((double)blocked.cycles_num) / (double)blocked.instructions_num);
}

double CORE_FinegrainedMT_CPI()
{
	return (((double)fine_grained.cycles_num) / (double)fine_grained.instructions_num);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid)
{
    tcontext temp_context = blocked.threads[threadid]->registers_files;
    context[threadid] = temp_context;
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid)
{
    tcontext temp_context = fine_grained.threads[threadid]->registers_files;
    context[threadid] = temp_context;
}
