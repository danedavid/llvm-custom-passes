/*
	to print source line number
	run clang with -g option to enable debug info
	run opt with -line option
*/
#include<llvm/Pass.h>
#include<llvm/Support/FormattedStream.h>
#include<llvm/IR/Module.h>
#include<llvm/IR/DebugInfo.h>
#include<llvm/IR/Instruction.h>
#include<llvm/IR/AssemblyAnnotationWriter.h>		//for writing comments

using namespace llvm;

namespace {

	class Commenter : public AssemblyAnnotationWriter {
		public:
			unsigned linenum(const Instruction *I) {
				DILocation *Loc = I->getDebugLoc();
				if(Loc != NULL){
					unsigned lno = Loc->getLine();					// to get source line number
					//errs()<< lno <<"\n";
					return lno;
				}
			}
			
			virtual void printInfoComment(const Value &V, formatted_raw_ostream &str) {	// override for writing comments
				if(const Instruction *I = dyn_cast<Instruction>(&V)) {
					str << " ; source line no: "<< linenum(I);
				}
			}
	};

	class LineNumber : public ModulePass {
		public:
			static char ID;
		
			LineNumber() : ModulePass(ID) { }
			
			Commenter c;
		
			virtual bool runOnModule(Module &M) {
				M.print( errs(), &c );
				return false;
			}
	};
	
	char LineNumber::ID = 0;
}

RegisterPass <LineNumber> X("line","comments source line numbers",false,false);
