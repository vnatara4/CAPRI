#include "capri.h"
#include <limits>
#include <iostream>

using namespace std;
using namespace CONTROL_DIVERGENCE;

unsigned long long total_instructions_issued= 0;
unsigned long long total_instructions_diverged = 0;
unsigned long long tbc_warp_saved = 0;
unsigned long long capri_warp_saved = 0;
unsigned long long capri_misprediction = 0;
unsigned long long num_predictions = 0;

CAPRI *CAPRI::capri;

CAPRI* CAPRI::createCapri()
{
  if(capri == NULL)
    capri = new CAPRI();
  return capri;
}

void CAPRI::deleteCapri()
{
  if(capri){
    delete capri;
    capri = NULL;
  }
}
/*
 * Fn: data_collector

 * Parameters: 
   tb_id   : represents thread block id
   warp_id : represents the warp executing this instruction
	 op      : opcode of the current instruction
	 pc      : pc of the current instruction
	 mask    : bitset of the active mask of current pc
 
 * return:
	 return void

	data collector will collect all the instructions that are issued by the GPGPU-sim scheduler and will be stored in the respective order
	in the m_trace map.

*/

void CAPRI::data_collector(TB_ID tb_id, unsigned long long warp_id, long pc, BitMask mask, int op)
{
    trace::iterator it = m_trace.find(tb_id);
    // If the threadblock is not present in the map, add it to the map and return its id  
    if(it == m_trace.end()){
      it = m_trace.insert(pair<TB_ID,TB>(tb_id, TB())).first;
    }
    TB &tb = it->second;
    // Add the warps to the threadblolck if it is not already present in the threadblock
    while((int)tb.size() <= warp_id){
      tb.push_back(warp());
    }
    // Store the instruction with respect to the active threadblock and warp. 
    Insn I;
    I.op_code = (OpCodes)op;
    I.pc = pc;
    I.mask = mask;
    tb[warp_id].push_back(I);
}

/*
 * Fn: run

 * Parameters: 
   
 * return:
	 return void

	-The run function will be processing the data for each thread block one by one.
  -We iterate through the instructions looking for a branch.
  -It will be maintaining a stack to keep track of all the branch instructions and
   then check compaction adequency for all subequent instruction until reconvergence 
   point is reached.
  -After that we again iterate through the vector of instructions and perform the 
   same set of operations when the next branch is encountered.  
	-It will also calculate the parameters like 
     - Number of warps saved 
     - SIMD utilization rate 
   for TBC and CAPRI. 
*/

void CAPRI::run()
{
	for(trace::iterator itblock = m_trace.begin(); itblock != m_trace.end(); itblock++) {

	  std::stack <BitMask> simd_mask;
		TB &curr_tb = itblock->second;

		for(unsigned long long i = 0; i < curr_tb[0].size();) {
			Insn curr_insn = curr_tb[0][i]; 
			
			//If branch insn was pushed on the stack in the last cycle then copy its active mask.
			if(simd_mask.empty() == false) {
				BitMask top_mask = simd_mask.top();
       				// If the copied active mask of the branch insn from the top of the stack is not equal to the current
				// instruction mask that means there is divergence and preform compaction.
        			// Else  this is the reconvergence point and pop the entry from the stack.
				if(curr_insn.mask != top_mask) {
      	  				is_compactable(curr_tb, i);			
				} else {
					simd_mask.pop();
					if(simd_mask.empty() == false) {
						is_compactable(curr_tb, i);
					}
				}
			}

		 //If the current instruction is a branch then push the entry on the stack.
     		 //All instructions after the branch will be checked for compaction adequacy.
  		if(curr_insn.op_code == BRANCH_OP){ 
  			simd_mask.push(curr_insn.mask);
			}

			i++;
			total_instructions_issued += curr_tb.size();
		}
		
	}

	printf("\n");
	printf("WPDOM: \n");
	printf("Total instrucions issued: %llu\n", total_instructions_issued);
	printf("Number of divergent insructions: %llu\n", total_instructions_diverged);
	printf("\n");
	
	printf("TBC: \n");
	printf("Number of warps saved in TBC: %llu\n", tbc_warp_saved);
	printf("Increase in SIMD Utilization rate(TBC): %f\n", 100.0 - (((total_instructions_issued - tbc_warp_saved)/(float)total_instructions_issued) * 100.0));
	printf("\n");

	printf("CAPRI: \n");
	printf("Number of warps saved in CAPRI: %llu\n", capri_warp_saved);
	printf("Misprediction rate: %f\n", (float)capri_misprediction/num_predictions);
	printf("Increase in SIMD Utilization rate(CAPRI): %f\n", 100.0 - (((total_instructions_issued - capri_warp_saved)/(float)total_instructions_issued) * 100.0));
	
	printf("\n");
}

/*
 * Fn: is_compactable

 * Parameters: 
   	curr_tb   : represents the current active thread block
	pos       : represents the position of the current instruction in the warp vector
 
 * return:
	 return void

	-The is_compactible fucntion will calculate the column sum for all the 32 threads.
	-if max of column sum < total number of warps execution this instruction then 
	  - compaction is effective.
	  - else compaction is ineffective.
  -Depending on the above test the history bit in the CAPT table will be set or reset
*/

void CAPRI::is_compactable(TB &curr_tb, unsigned long long pos)
{
	unsigned long long warp_count = 0;
	unsigned long long col_sum[32] = {0};
	unsigned long long warp_saved = 0;
	
	//Calculating the column sum for all the 32 SIMD lanes
	for(unsigned long long wid=0; wid<curr_tb.size(); wid++) {
		for(int i=0; i<32; i++) {
			col_sum[i] += curr_tb[wid][pos].mask[i];
		}
		warp_count++;
	}

	unsigned long long sum_mask = 0;
	for(unsigned long long wid=0; wid<curr_tb.size(); wid++) {	
		for(unsigned long long i=0; i<32; i++) {
			sum_mask += curr_tb[wid][pos].mask[i];
		}
		if(sum_mask != 32)
			total_instructions_diverged++;
		sum_mask = 0;
	}

	//Calculating the max of the column sum which will be used to determine compaction adequacy
	unsigned long long max = col_sum[0];
	for(int i=0; i<32; i++) {
		if(col_sum[i] > max)
			max = col_sum[i];
	}

	warp_saved = (warp_count - max);
	tbc_warp_saved += warp_saved;

	long pc = curr_tb[0][pos].pc;

	//If history bit is 1 and if compaction was effective set the history bit else reset history bit
	//Else if history bit was 0 then if compaction was effective then set history bit else reset
	num_predictions++;
	if(capt_obj.get_history_bit(pc)) {
		if(warp_saved > 0) {
			capt_obj.set_history_bit(pc, true);
			capri_warp_saved += warp_saved;
		} else {
			capt_obj.set_history_bit(pc, false);
			capri_misprediction++;
		}
	} else {
		if(warp_saved > 0) {
			capt_obj.set_history_bit(pc, true);
			capri_misprediction++;
		} else {
			capt_obj.set_history_bit(pc, false);
		}
	}
}
