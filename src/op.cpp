#include "pch.hpp"
#include "op.hpp"

namespace be {
namespace op {

///////////////////////////////////////////////////////////////////////////////
Op::Op() { }

///////////////////////////////////////////////////////////////////////////////
Op::Op(OpData data)
   : data_(std::move(data))
{ }

///////////////////////////////////////////////////////////////////////////////
Op::Op(Op&& other)
   : Handleable<Op>(std::move(other)),
     data_(std::move(other.data_))
{ }

///////////////////////////////////////////////////////////////////////////////
Op& Op::operator=(Op&& other) {
   swap_(other);
   return *this;
}

///////////////////////////////////////////////////////////////////////////////
const OpData::action_func& Op::action() const {
   return data_.action;
}

///////////////////////////////////////////////////////////////////////////////
void Op::action(OpData::action_func func) {
   data_.action = std::move(func);
}

///////////////////////////////////////////////////////////////////////////////
F64 Op::remaining() const {
   return data_.remaining;
}

///////////////////////////////////////////////////////////////////////////////
F64 Op::remaining(F64 new_value) {
   F64 val = data_.remaining;
   data_.remaining = new_value;
   return val;
}

///////////////////////////////////////////////////////////////////////////////
F64 Op::total() const {
   return data_.total;
}

///////////////////////////////////////////////////////////////////////////////
F64 Op::total(F64 new_value) {
   F64 val = data_.total;
   data_.total = new_value;
   return val;
}

///////////////////////////////////////////////////////////////////////////////
void Op::operator()(F64 dt) {
   data_.action(data_, dt);
}

///////////////////////////////////////////////////////////////////////////////
void Op::swap_(Op& other) {
   using std::swap;
   swap(data_, other.data_);
   swap_source_handle_(other);
}

} // be::op
} // be
