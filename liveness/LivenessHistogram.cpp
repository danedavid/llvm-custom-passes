//	LivenessHistogram.cpp
//	To perform live variable analysis and represent the information using a histogram
//
//	Based on code from Todd C. Mowry
//	Modified by Arthur Peters
//	Modified by Ankit Goyal
/////////////////////////////////////////////////////////////////////////////////////

/******************************************************************************

Liveness: OUT[n] = UNION_{s E succ[n]} IN[s]  //meet
IN[n] = GEN[n] U (OUT[n] - KILL[n]) //transfer function

Flow Direction: Backward
A BitVector stored at each node for IN and OUT. Bit vector contains an entry for all the values

Boundary Conditions: empty set for flow value. identified by no successors.

 *********************************************************************************/

#include<llvm/Pass.h>
#include<llvm/IR/DebugInfo.h>
#include<llvm/IR/Function.h>
#include<llvm/IR/Module.h>
#include<llvm/Support/raw_ostream.h>
#include<llvm/Support/FormattedStream.h>
#include<llvm/IR/InstIterator.h>
#include<llvm/IR/Instruction.h>
#include<llvm/IR/AssemblyAnnotationWriter.h>
#include<llvm/ADT/BitVector.h>
#include<llvm/IR/ValueMap.h>
#include<llvm/ADT/DenseMap.h>

#include "dataflow.h"

#include <ostream>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <algorithm>

using namespace llvm;

namespace {

    class LivenessHistogram : public ModulePass, public DataFlow<BitVector>, public AssemblyAnnotationWriter{

        public:
            static char ID;

            // set forward false in the constructor DataFlow()
            LivenessHistogram() : DataFlow<BitVector>(false), ModulePass(ID) {
                bvIndexToInstrArg = new std::vector<Value*>();
                valueToBitVectorIndex = new ValueMap<Value*, int>();
                instrInSet = new ValueMap<const Instruction*, BitVector*>();
            }

            // domain vector to store all definitions and function arguments
            std::vector<Value*> domain;
            std::vector<Value*> *bvIndexToInstrArg; 				//map values to their bitvector
            ValueMap<Value*, int> *valueToBitVectorIndex;      		//map values (args and variables) to their bit vector index
            ValueMap<const Instruction*, BitVector*> *instrInSet;     //IN set for an instruction inside basic block

            int domainSize;
            int numArgs;
            int numInstr;
            
            int live[50];
            unsigned int n = 0;

            //print functions
			//live variables before each basic block
            virtual void emitBasicBlockStartAnnot(const BasicBlock *bb, formatted_raw_ostream &os) {
                os << "; ";
                if (!isa<PHINode>(*(bb))) {
                    const BitVector *bv = (*in)[&*bb];

                    for (int i=0; i < bv->size(); i++) {
                        if ( (*bv)[i] ) {
                            os << (*bvIndexToInstrArg)[i]->getName();
                            os << ", ";
                        }
                    }
                }
                os << "\n";
            }

			//live variables before each instruction: used for computing histogram
            virtual void emitInstructionAnnot(const Instruction *i, formatted_raw_ostream &os) {
                os << "; ";
                if (!isa<PHINode>(*(i))) {
                    const BitVector *bv = (*instrInSet)[&*i];
/*                    
                    live[bv->size()]++;
                    if(bv->size() > n)
                    	n = bv->size();
*/                    
                    for (int i=0; i < bv->size(); i++) {
                        if ( (*bv)[i] ) {
                            os << (*bvIndexToInstrArg)[i]->getName();
                            os << ", ";
                        }
                    }
                }
                os << "\n";
            }

            //implementation of functions

            //set the boundary condition for block
            //explicit constructor of BitVector
            virtual void setBoundaryCondition(BitVector *blockBoundary) {
                *blockBoundary = BitVector(domainSize, false); 
            }

            //union (bitwise OR) operator '|=' overriden in BitVector class
            virtual void meetOp(BitVector* lhs, const BitVector* rhs){
                *lhs |= *rhs; 
            }

            //empty set initially; each bit represent a value
            virtual BitVector* initializeFlowValue(BasicBlock& b, SetType setType){ 
                return new BitVector(domainSize, false); 
            }


            //transfer function:
            //IN[n] = USE[n] U (OUT[n] - DEF[n])
            
            virtual BitVector* transferFn(BasicBlock& bb) {
                BitVector* outNowIn = new BitVector(*((*out)[&bb]));
                                   
                BitVector* immIn = outNowIn; // for empty blocks
                Instruction* tempInst;
                bool breakme=false;
                // go through instructions in reverse
                BasicBlock::iterator ii = --(bb.end()), ib = bb.begin();
                while (true) {

                    // inherit data from next instruction
                    tempInst = &*ii;
                    immIn = (*instrInSet)[tempInst];            
                    *immIn = *outNowIn;

                    // if this instruction is a new definition, remove it
                    if (isDefinition(tempInst)){
                        (*immIn)[(*valueToBitVectorIndex)[tempInst]] = false;
                    }

                    // add the arguments, unless it is a phi node
                    if (!isa<PHINode>(*ii)) {
                        User::op_iterator OI, OE;
                        for (OI = tempInst->op_begin(), OE=tempInst->op_end(); OI != OE; ++OI) {
                            if (isa<Instruction>(*OI) || isa<Argument>(*OI)) {
                                (*immIn)[(*valueToBitVectorIndex)[*OI]] = true;
                            }
                        }
                    }else if(isa<PHINode>(*ii)){
                        PHINode* phiNode = cast<PHINode>(&*ii);
                        for (int incomingIdx = 0; incomingIdx < phiNode->getNumIncomingValues(); incomingIdx++) {
                            Value* val = phiNode->getIncomingValue(incomingIdx);
                            if (isa<Instruction>(val) || isa<Argument>(val)) {
                                int valIdx = (*valueToBitVectorIndex)[val];
                                BasicBlock* incomingBlock = phiNode->getIncomingBlock(incomingIdx);
                                if ((*neighbourSpecificValues).find(incomingBlock) == (*neighbourSpecificValues).end())
                                    (*neighbourSpecificValues)[incomingBlock] = new BitVector(domainSize);
                                (*(*neighbourSpecificValues)[incomingBlock]).set(valIdx);                                
                            }
                        }
                    }

                    outNowIn = immIn;

                    if (ii == ib) break;

                    --ii;
                }

                return immIn;
            }

            bool isDefinition(Instruction *ii) {
                return !(isa<TerminatorInst>(ii)) ;
            }

			void calculate(const Instruction *i) {
				//static unsigned int live[50], n=0;
				int count=0;
				
				if (!isa<PHINode>(*(i))) {
                    const BitVector *bv = (*instrInSet)[&*i];
                   	
                   	for(int i=0;i<bv->size();i++){
                   		if((*bv)[i])
                   			count++;
                   	}
                   	
                   	if(count > n) {
                   		n = count+1;
                   	}
                   	
                   	live[count]++;
                }
			}
            
            //evaluate each function
            virtual bool evalFunc(Function &F) {
                domain.clear();
                bvIndexToInstrArg = new std::vector<Value*>();
                valueToBitVectorIndex = new ValueMap<Value*, int>();
                instrInSet = new ValueMap<const Instruction*, BitVector*>();
 
                int index = 0;
                for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg){
                    domain.push_back(&*arg);
                    bvIndexToInstrArg->push_back(&*arg);
                    (*valueToBitVectorIndex)[&*arg] = index;
                    index++;
                }

                for (inst_iterator instruction = inst_begin(F), e = inst_end(F); instruction != e; ++instruction) {
                    domain.push_back(&*instruction);
                    bvIndexToInstrArg->push_back(&*instruction);
                    (*valueToBitVectorIndex)[&*instruction] = index;
                    index++;
                }

                domainSize = domain.size();

                //initialize the IN set set inside the block for each instruction.     
                for (inst_iterator instruction = inst_begin(F), e = inst_end(F); instruction != e; ++instruction) {
                    (*instrInSet)[&*instruction] = new BitVector(domainSize, false); 
                }
				//call the backward analysis method in dataflow
                DataFlow<BitVector>::runOnFunction(F);
                F.print(errs(), this);
                
                //compute the histogram
                for(inst_iterator instruction = inst_begin(F), e = inst_end(F); instruction != e; ++instruction) {
                	calculate(&*instruction);
                }
                

                return false;
            }
            
            virtual bool runOnModule(Module &M){
            	std::fill_n(live,50,0);
            
                for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
                {
                    evalFunc(*MI);
                }
                
                for(unsigned int i=0;i<n;i++){
                	if(live[i] >= 0)
						errs()<<i<<" : "<<live[i]<<"\n";
				}
                return false;
            }

            virtual void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.setPreservesAll();
            }

    };

    char LivenessHistogram::ID = 0;

    RegisterPass<LivenessHistogram> X("live", "liveness pass");

}
