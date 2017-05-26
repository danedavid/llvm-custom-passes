using namespace llvm;

namespace {
    /* template class to store zero (or one for mul and div) */
    template <typename ZType> ZType getZeroOne(unsigned num_bits, int zeroOne);
    template <> APInt   getZeroOne(unsigned num_bits, int zeroOne) { return APInt(num_bits,zeroOne); }

    /* compare a constant value with zero */
    template <typename ConstantType, typename ZType> bool val_compare(ConstantType& cint, ZType zero);
    template <> bool val_compare(ConstantInt& cint, APInt zero) { return cint.getValue().eq(zero); }

}
