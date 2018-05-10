**********************************************************************
Project	     : CAPRI - Prediction of Compaction Adequacy for handling
	       Control Divergence in GPGPU Architectures.

Project team : Veerakumar Natrajan
	       Dhruv Sheth
	       Kunal Ochani
	       Prit Gala
********************************************************************** 

# Modified Files:
-----------------------------------------------------------------------
      NAME 		      |		      Directory
1. absarct_hardware_model.h   | ~/gpgpu-sim_distribution/src 
2. abstarct_hardware_model.cc | ~/gpgpu-sim_distribution/src
3. gpu-sim.cc                 |	~/gpgpu-sim_distribution/src/gpgpu-sim
4. gpgpusim_entrypoint.cc     | ~/gpgpu-sim_distribution/src 
-----------------------------------------------------------------------
			      	
# New Files Added:
-----------------------------------------------------------------------
      NAME 		      |		      Directory
1. capri.cc		      |	~/gpgpu-sim_distribution/src
2. capri.h		      |	~/gpgpu-sim_distribution/src
-----------------------------------------------------------------------

# Modifiacions:

1. absract_hardware_model.h
------------------------------------------------------------------------
 -   line 307;
	fn		: void update(dim3 ctaId, int warpId,......)
	Added parameters: dim3 ctaId
			  int warpId

------------------------------------------------------------------------
2. absract_hardware_model.cc
------------------------------------------------------------------------
 -   line 29: 
	#include "capri.h"

 -   line 649:
	fn              : void simt_stack::update(dim3 ctaId, int warpId,........... );
	parameters added: ctaId - To pass he current TB to data_collecor fn.
			  warpId- To pass the warpId of current instruction being executed.

 -   line 714:
	funtion call to data_collector funcion in case of 
	  - no divergence
	  - divergence -> only not taken path

		CONTROL_DIVERGENCE::TB_ID tb_id(ctaId.x,ctaId.y,ctaId.z);
	        CONTROL_DIVERGENCE::CAPRI::createCapri()->data_collector(tb_id, warpId, tmp_next_pc, tmp_active_mask, ptx_fetch_inst(tmp_next_pc)->op);

 -   line 725:
	funcion call to data_collecor in case of 
	  - divergence -> only taken path

		CONTROL_DIVERGENCE::TB_ID tb_id(ctaId.x,ctaId.y,ctaId.z);
	        CONTROL_DIVERGENCE::CAPRI::createCapri()->data_collector(tb_id, warpId, tmp_next_pc, tmp_active_mask, ptx_fetch_inst(tmp_next_pc)->op);
------------------------------------------------------------------------
3. gpu-sim.cc
------------------------------------------------------------------------
 -   line 727:
	After finishing the execuion of complete benchmark we call the capri run() fn to get the the stats.
	*** In case you want to limit the number of instruction in the benchmark then look for descripion 
	    of gpgpusim-enrypoint.cc
------------------------------------------------------------------------
4. gpgpusim_enrypoint.cc
------------------------------------------------------------------------
 -   line 137:
	If the number of instrucions were limied in the .config file then we call the capri run
	funcion here to get the stats.

	command to limit the insn: -gpgpu_max_insn 300000 (add in he config file).
------------------------------------------------------------------------
5. capri.h
------------------------------------------------------------------------
 -   struct TB_ID:
	-This structure is used to store the thread block id.
	-Three operator overload functions are defined which will be used 
	 by the find() function to search the thread block in the map.

 -    Data Structure hierarchy for storing all the instructions

	 map    : trace will have TB_ID : TB
	 vector : TB is a vector of warps
	 vector ; warp is a vector of instruction
	 struct : Insn is a structure for storing the pc, opcode and active mask of all instructions

 -   class CAPT:
	-CAPT table: stores pc of branch and history bit
	-get_history_bit()
	-set_history_bit()

 -   class CAPRI-
	-Instantiated the CAPT table
	-Instantiated the trace data structure 
	-Data is collected using the data collection function
		void data_collector(TB_ID tbid, unsigned long long wid, long pc, BitMask mask, int opcode);
	-Processing the data in the run function
		void run();
------------------------------------------------------------------------
6. capri.cc
------------------------------------------------------------------------
 -  Fn: data_collector:
	The data collector fn will collect all the instructions that are issued by the 
	GPGPU-sim scheduler and will be stored in the respective order in the m_trace map.

 -   Fn: run:
      - The run function will be processing the data for each thread block one by one.
      - We iterate through the instructions looking for a branch.
      - It will be maintaining a stack to keep track of all the branch instructions and
   	then check compaction adequency for all subequent instruction until reconvergence 
   	point is reached.

 -   Fn: is_compactable:
       - The is_compactible fucntion will calculate the column sum for all the 32 threads.
       - if max of column sum < total number of warps execution this instruction then 
		- compaction is effective.
		- else compaction is ineffective.
       - Depending on the above test the history bit in the CAPT table will be set or reset
      - After that we again iterate through the vector of instructions and perform the 
   	same set of operations when the next branch is encountered.  
      - It will also calculate the parameters like 
     		- Number of warps saved 
     		- SIMD utilization rate 
   	for TBC and CAPRI.
------------------------------------------------------------------------
