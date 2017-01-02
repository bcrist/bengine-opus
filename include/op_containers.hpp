#pragma once
#ifndef BE_CORE_OP_CONTAINERS_HPP_
#define BE_CORE_OP_CONTAINERS_HPP_

#include "op_functions.hpp"

namespace be {
namespace op {
namespace detail {

struct Queue : OpFunc<Queue> {
   void operator()(OpData& data, F64& dt);
   OpData::child_list_type::iterator position;
   bool initialized = false;
};

struct Set : OpFunc<Set> {
   void operator()(OpData& data, F64& dt);
};

struct StaticSet : OpFunc<Set> {
   void operator()(OpData& data, F64& dt);
};

} // be::op::detail
} // be::op
} // be

#endif
