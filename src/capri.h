#include <bitset>
#include <vector>
#include <map>
#include <stack>
#include <set>

namespace CONTROL_DIVERGENCE
{
  enum OpCodes
  {
    NO_OP=-1,
    ALU_OP=1,
    SFU_OP,
    ALU_SFU_OP,
    LOAD_OP,
    STORE_OP,
    BRANCH_OP,
    BARRIER_OP,
    MEMORY_BARRIER_OP,
    CALL_OPS,
    RET_OPS
  };

  typedef std::bitset<32> BitMask;

	/*
	 -This structure is used to store the thread block id
	 -Three operator overload functions are defined which 
	  will be used by the find() function to search the 
		thread block in the map.
	*/
  struct TB_ID{
    int x;
    int y;
    int z;
    TB_ID(int ax, int ay, int az){
      x=ax;
			y=ay; 
			z=az;
    }

    bool operator <(const TB_ID & other) const
    {
      if(x < other.x) return true;
      if(y < other.y) return true;
      if(z < other.z) return true;
      return false;
    }
    bool operator >(const TB_ID & other) const
    {
      if(x > other.x) return true;
      if(y > other.y) return true;
      if(z > other.z) return true;
      return false;
    }
    bool operator ==(const TB_ID & other) const
    {
      if(x != other.x) return false;
      if(y != other.y) return false;
      if(z != other.z) return false;
      return true;
    }
  };

	/*
	 Data Structure hierarchy for storing all the instructions

	 map    : trace will have TB_ID : TB
	 vector : TB is a vector of warps
	 vector ; warp is a vector of instruction
	 struct : Insn is a structure for storing the pc, opcode and active mask
		  of all instructions
	*/

  struct Insn
  {
    long pc;
    OpCodes op_code;
    BitMask mask;
  };

  typedef std::vector<Insn> warp;
  typedef std::vector < warp > TB;
  typedef std::map < TB_ID, TB > trace;

	/*
	class CAPT-
		-CAPT table: stores pc of branch and history bit
		-get_history_bit()
		-set_history_bit()  
	*/
  class CAPT
  {
    typedef std::map<long, bool> capt;
		capt capri_table; 

    public:
		bool get_history_bit(long pc) {
			capt::iterator i = capri_table.find(pc);
			if(i != capri_table.end()) {
				return i->second;
			}
			return true;
		}

		void set_history_bit(long pc, bool h_bit) {
			capri_table[pc] = h_bit;
		
		}
  };

	/*
	class CAPRI-
		-Instantiated the CAPT table
		-Instantiated the trace data structure 
		-Data is collected using the data collection function
		-Processing the data in the run function
	*/
  class CAPRI
  {
    private:
      CAPT capt_obj;
      trace m_trace;

      static CAPRI *capri;
      void is_compactable(TB &curr_tb, unsigned long long pos);

    public:
      static CAPRI* createCapri();
      static void deleteCapri();
      void data_collector(TB_ID tbid, unsigned long long wid, long pc, BitMask mask, int opcode);
      void run();
  };

};

