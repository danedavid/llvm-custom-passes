/*
*	LLVM pass to count the no. of instructions in each basic block of
*	input source program
*	
*	usage : opt -load <path-to-object-file> -bbcount <input-program.bc>
*/

#include<llvm/IR/Instruction.h>
#include<llvm/Pass.h>
#include<llvm/IR/Function.h>
#include<llvm/Support/raw_ostream.h>
#include<llvm/ADT/Statistic.h>
#define DEBUG_TYPE "count"

using namespace llvm;

STATISTIC(count,"number of basic blocks");

static int i=0;
unsigned int counter;

namespace {

	struct BBCount : BasicBlockPass {
		static char ID;
		
		BBCount() : BasicBlockPass(ID) { }
		
		bool runOnBasicBlock(BasicBlock &bb) {
			counter=0;
			count++;
			for(const auto &i:bb) {
				counter++;
			}
			errs()<<"Basic Block "<<++i<<" has "<<counter<<" instructions\n";
			return false;
		}
		
	};
	
	char BBCount::ID = 0;
}

RegisterPass <BBCount> X("bbcount","Prints no. of instructions in each basic block",false,false);
