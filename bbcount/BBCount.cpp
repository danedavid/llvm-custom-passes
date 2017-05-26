#include<llvm/IR/Instruction.h>
#include<llvm/Pass.h>
#include<llvm/IR/Function.h>
#include<llvm/Support/raw_ostream.h>

#include<llvm/ADT/Statistic.h>
#define DEBUG_TYPE "count"

using namespace llvm;

STATISTIC(count,"number of basic blocks");
//STATISTIC(counter, "Number of instructions");
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
			errs()<<"BB"<<++i<<":"<<counter<<"\n";
			return false;
		}
		
	/*	bool runOnFunction(Function &f) override {
			counter=0;
			errs()<<"Function "<<f.getName()<<" is there ";
			
			for(;const auto &bb;) {
				counter++;
			}
			
			errs()<<"with "<<counter<<" basic blocks\n";
			
			return false;
		}
	*/
	};
	
	char BBCount::ID = 0;
}

RegisterPass <BBCount> X("bbcount","Prints no. of instructions in each basic block",false,false);
