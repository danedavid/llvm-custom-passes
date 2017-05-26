//pass to implement algebraic simplifications for addition and subtraction and simple constant folding
#include<llvm/IR/BasicBlock.h>
#include<llvm/IR/Constants.h>
#include<llvm/IR/Instruction.h>
#include<llvm/IR/Instructions.h>
#include<llvm/Pass.h>
#include<llvm/Transforms/Utils/BasicBlockUtils.h>
#include<llvm/ADT/Statistic.h>
#include<llvm/Support/raw_ostream.h>

#include "templates.h"

#define DEBUG_TYPE "algebraic"
STATISTIC(numtransforms, "The no. of algebraic identity transformations performed");
STATISTIC(NumConstFolds, "The no. of constant folding transformations performed");
using namespace llvm;

namespace {

	bool replay = false;		//if replay is true, the basic block is evaluated again from 1st instruction

	template<typename ZType, typename ConstantType>
	Value * algebraicAddSub(Instruction &I, ZType generalZeroOne, int operation) {
		
		Value *operand1 = I.getOperand(0);
		Value *operand2 = I.getOperand(1);
		ConstantType *cint;
		
		if((operand1 == operand2)&&(operation==1)){
			IntegerType *intype = dyn_cast<IntegerType>(I.getType());
			//ConstantInt *zero;
			//errs()<<"entered";
			//if(zero = ConstantInt::get(intype->getContext(), getZeroOne<APInt>(intype->getBitWidth(),0)))
				return ConstantInt::get(intype->getContext(), getZeroOne<APInt>(intype->getBitWidth(),0));
		}
		
		//for X + 0 = X and X - 0 = X
		if(cint = dyn_cast<ConstantType>(operand2)) {
			if(val_compare<ConstantType,ZType>(*cint, generalZeroOne))
				return operand1;
		}
		//for 0 + X = X
		else if((cint = dyn_cast<ConstantType>(operand1)) && (operation==0)) {
			if(val_compare<ConstantType,ZType>(*cint, generalZeroOne))
				return operand2;
		}
		return NULL;
	}
	
	//Constant Folding
	ConstantInt* fold_constants(unsigned operation, ConstantInt *op1, ConstantInt *op2){
        switch(operation){
            case Instruction::Add:
                return ConstantInt::get(op1->getContext(), op1->getValue() + op2->getValue());
            case Instruction::Sub:
                return ConstantInt::get(op1->getContext(), op1->getValue() + op2->getValue());
            case Instruction::Mul:
                return ConstantInt::get(op1->getContext(), op1->getValue() * op2->getValue());
            case Instruction::UDiv:
                return ConstantInt::get(op1->getContext(), op1->getValue().udiv(op2->getValue()));
            case Instruction::SDiv:
                return ConstantInt::get(op1->getContext(), op1->getValue().sdiv(op2->getValue()));

        }
        return NULL;
    }
	
	void makeTransforms(BasicBlock::iterator &bi, Value* val) {
		bi->replaceAllUsesWith(val);					//transforms all uses of the instruction; replaces with val
		BasicBlock::iterator temp = bi;
		bi++;
		temp->eraseFromParent();
		
		if(bi != bi->getParent()->begin())
			bi--;
		else
			replay = true;
	}
	
	class Algebraic : public BasicBlockPass {
		public:
			static char ID;
			
			Algebraic() : BasicBlockPass(ID){}
			
			virtual bool runOnBasicBlock(BasicBlock &bb) {
				for(BasicBlock::iterator i = bb.begin(), e = bb.end(); i != e; i++) {
					if(replay){
						i = bb.begin();
						replay = false;
					}
					
					Value *v = NULL;
					Instruction *in = dyn_cast<Instruction>(i);
					IntegerType *intype = dyn_cast<IntegerType>(i->getType());
					
					if( in->getOpcode() == Instruction::Add ) {
						v = algebraicAddSub<APInt,ConstantInt>(*in,getZeroOne<APInt>(intype->getBitWidth(),0),0);
					}
					else if( in->getOpcode() == Instruction::Sub ) {
						v = algebraicAddSub<APInt,ConstantInt>(*in,getZeroOne<APInt>(intype->getBitWidth(),0),1);
					}
					if(v){
						makeTransforms(i,v);
						++numtransforms;
						continue;
					}
					
					//Constant Folding
                    if((in->getNumOperands() == 2) && isa<Constant>(in->getOperand(0)) && isa<Constant>(in->getOperand(1))){
                        Value *v1 = fold_constants(in->getOpcode(), dyn_cast<ConstantInt>(in->getOperand(0)), dyn_cast<ConstantInt>(in->getOperand(1)));
                        if(v1) {  
                            makeTransforms(i,v1);
                            ++NumConstFolds; 
                            continue;
                        }
                    }
				}
			}
	};
}

char Algebraic::ID = 0;

RegisterPass <Algebraic> X("algebraic","algebraic simplifications",false,false);
