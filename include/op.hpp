#pragma once
#ifndef BE_CORE_OP_HPP_
#define BE_CORE_OP_HPP_

#include "handleable.hpp"
#include <boost/function.hpp>
#include <boost/container/vector.hpp>

namespace be {
namespace op {

struct OpData;
class Op;
class Opus;

inline void empty_op_func(OpData&, F64&) { }

///////////////////////////////////////////////////////////////////////////////
struct OpData {
   using action_func = boost::function<void(OpData&, F64&)>;
   using child_list_type = boost::container::vector<Op>;

   F64 remaining = -1;
   F64 total = 0;
   action_func action = empty_op_func;
   child_list_type children;
};

///////////////////////////////////////////////////////////////////////////////
class Op final : public Handleable<Op> {
   friend class Opus;
   friend void swap(Op& a, Op& b) { a.swap_(b); }
public:
   Op();
   Op(OpData data);
   Op(Op&& other);
   Op& operator=(Op&& other);

   const OpData::action_func& action() const;
   void action(OpData::action_func func);

   F64 remaining() const;
   F64 remaining(F64 new_value);

   F64 total() const;
   F64 total(F64 new_value);

   void operator()(F64 dt);

private:
   void swap_(Op& other);
   OpData data_;
};

} // be::op
} // be

#endif
