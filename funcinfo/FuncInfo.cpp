//pass to get details of every function in the program
#include<llvm/Pass.h>
#include<llvm/Support/FormattedStream.h>
#include<llvm/IR/InstIterator.h>
#include<llvm/IR/Module.h>
#include<llvm/IR/Function.h>
#include<llvm/IR/Instruction.h>

using namespace llvm;

namespace {

	class FuncInfo : public ModulePass {
	    public:
		static char ID;
		FuncInfo() : ModulePass(ID){ }
		
		void printFunctionDetails(Function &F){
			int arg_size = F.arg_size();
			int num_of_uses = F.getNumUses();
			int size = F.size();
			
			int num_inst = 0;
			
			for(inst_iterator I = inst_begin(F), E = inst_end(F) ; I!=E ; I++)
				num_inst++;
			
			errs()<<"\nFunction "<<F.getName()<<":";
			//errs()<<"\nReturn Type: "<<F.getReturnType()->getName();
			errs()<<"\nNumber of arguments: "<<arg_size;
			errs()<<"\nNumber of call sites: "<<num_of_uses;
			errs()<<"\nNumber of basic blocks: "<<size;
			errs()<<"\nNumber of instructions: "<<num_inst;
			errs()<<"\n";
		}
		
		virtual bool runOnModule(Module &M){
			for(Module::iterator MI = M.begin(), ME = M.end() ; MI != ME ; MI++)
				printFunctionDetails(*MI);
			return false;
		}
	};
	
	char FuncInfo::ID = 0;
}

RegisterPass <FuncInfo> X("funcinfo","prints details of functions",false,false);
