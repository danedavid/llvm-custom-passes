//	File "dataflow.h"
//	
//	To perform backward and forward analysis of the source program
////////////////////////////////////////////////////////////////////////////////

#include<llvm/Pass.h>
#include<llvm/IR/BasicBlock.h>
#include<llvm/Support/raw_ostream.h>
#include<llvm/IR/Instruction.h>
#include<llvm/IR/Instructions.h>
#include<llvm/IR/Constants.h>
#include<llvm/Transforms/Utils/BasicBlockUtils.h>
#include<llvm/ADT/BitVector.h>
#include<llvm/IR/ValueMap.h>
#include<llvm/IR/CFG.h>
#include<llvm/ADT/DenseMap.h>

#include <ostream>
#include <list>

using namespace llvm;

#ifndef __DATAFLOW_H__
#define __DATAFLOW_H__

namespace {

    typedef std::list<BasicBlock*> Worklist;

    template <typename FlowValueType>			//FlowValueType indicates the type of flow value. Eg: BitVector
        class DataFlow{
            bool forward;						//flow direction; true for forward
            public:
            enum SetType {
                IN,
                OUT
            };

            DataFlow(bool forward) {
                this->forward = forward;  
                in  = new BlockInOutMap();
                out = new BlockInOutMap();
                neighbourSpecificValues = new BlockInOutMap();
                visited = new ValueMap<BasicBlock*, bool>();
            }

            ~DataFlow(){
                delete in;
                delete out;
                delete neighbourSpecificValues;
                delete visited;
             }
            //mapping from basicblock to lattice
            typedef ValueMap<const BasicBlock*, FlowValueType*> BlockInOutMap;    
            //IN and OUT sets for a basic block
            BlockInOutMap *in;
            BlockInOutMap *out;
            BlockInOutMap *neighbourSpecificValues;
            ValueMap<BasicBlock*, bool> *visited;
            

            //forward and backward analysis functions
            void performForwardAnalysis(Worklist &w){
                BasicBlock *hotBlock = *w.begin();
                w.pop_front();
                (*visited)[hotBlock] = true;
                
                //errs() << "Entering Block " << hotBlock->getName() << "\n";
                
                int numPred = 0;
                for (pred_iterator PI = pred_begin(hotBlock), E = pred_end(hotBlock); PI != E; ++PI){
                    ++numPred;
                    if(PI == pred_begin(hotBlock)){
                        *(*in)[hotBlock] = *(*out)[*PI];
                    }else{
                        meetOp((*in)[hotBlock], (*out)[*PI]);
                    }
                }

                if(numPred == 0)  setBoundaryCondition((*in)[hotBlock]);


                FlowValueType *newOut = transferFn(*hotBlock);
                bool changed = false;
                changed = (*newOut != *(*out)[hotBlock]);

                if(changed)
                    *(*out)[hotBlock] = *newOut;

                    for (succ_iterator SI = succ_begin(hotBlock), E = succ_end(hotBlock); SI != E; ++SI) {
                        if(changed || !(*visited)[*SI]){                            
                            w.push_back(*SI); 
                        } 
                    }
            }

            void performBackwardAnalysis(Worklist &w){
                BasicBlock *hotBlock = *w.begin();
                w.pop_front();
                (*visited)[hotBlock] = true;
                
                //errs() << "Entering Block " << hotBlock->getName() << "\n";
                
                //OUT of this basic block is equivalent to IN of its successor
                int numSucc = 0; //to check for the exit node

                for (succ_iterator SI = succ_begin(hotBlock), E = succ_end(hotBlock); SI != E; ++SI) {
                    numSucc++;
                    if(SI == succ_begin(hotBlock)){
                        *(*out)[hotBlock] = *(*in)[*SI]; //copy the first IN set values
                    }else{
                        meetOp((*out)[hotBlock], (*in)[*SI]); //call the meet operator                        
                    }
                }

                if((*neighbourSpecificValues).find(hotBlock) != (*neighbourSpecificValues).end() ){
                    //for phi node. meet the variables that are live from this specific block
                    meetOp((*out)[hotBlock], (*neighbourSpecificValues)[hotBlock]);
                }

                if(numSucc == 0) setBoundaryCondition((*out)[hotBlock]); //set boundary condition for the exit node.

                FlowValueType *newIn = transferFn(*hotBlock);

                bool changed = false;
                changed = ((*newIn) != (*(*in)[hotBlock]));

                if(changed)
                    *(*in)[hotBlock] = *newIn;
                    
                    for (pred_iterator PI = pred_begin(hotBlock), E = pred_end(hotBlock); PI != E; ++PI) {
                        if(changed || !(*visited)[*PI]){                            
                            w.push_back(*PI); 
                        } 
                    }
            }
            
            void finalizeBackwardAnalysis(Function &func){
                for (Function::iterator i = func.begin(), e = func.end(); i != e; ++i){
                    int numSucc = 0;
                    BasicBlock *hotBlock = &*i;
                    if((*neighbourSpecificValues).find(hotBlock) != (*neighbourSpecificValues).end()){
                        for (succ_iterator SI = succ_begin(&*i), SE = succ_end(&*i); SI != SE; SI++){
                            meetOp((*in)[*SI], (*neighbourSpecificValues)[hotBlock]);
                        }                            
                    }
                }
            }

            //add just the last block in case of backward analysis
            void initializeWorklist(Function &func, Worklist &worklist){
                if(forward){
                    BasicBlock& entry = func.getEntryBlock();
                    worklist.push_back(&entry);
                    return;
                }

                for (Function::iterator i = func.begin(), e = func.end(); i != e; ++i){
                    int numSucc = 0;

                    for (succ_iterator SI = succ_begin(&*i), SE = succ_end(&*i); SI != SE; SI++)
                        numSucc++;

                    if(numSucc==0) worklist.push_back(&*i);
                }
            }

            bool runOnFunction(Function &F){
                visited = new ValueMap<BasicBlock*,bool>();
                for (Function::iterator bi = F.begin(), be = F.end(); bi != be; bi++) {
                    BasicBlock * bb = &*bi;

                    //set the appropriate values for forward and backward flow
                    //(*in)[bb] -> bit vector::lattice for that block
                    (*in)[bb] = initializeFlowValue(*bi, IN);
                    (*out)[bb] = initializeFlowValue(*bi, OUT);
                }

                Worklist *worklist = new Worklist();
                initializeWorklist(F,*worklist);
                while(!worklist->empty()){
                    if(forward)
                        performForwardAnalysis(*worklist);
                    else
                        performBackwardAnalysis(*worklist);
                }
                finalizeBackwardAnalysis(F);
            }

            protected:
            
            virtual void setBoundaryCondition(FlowValueType*) = 0;
            
            virtual void meetOp(FlowValueType* lhs, const FlowValueType* rhs) = 0;
            
            virtual FlowValueType* initializeFlowValue(BasicBlock& b, SetType setType) = 0;
            
            virtual FlowValueType* transferFn(BasicBlock& b) = 0;
        };
}

#endif
