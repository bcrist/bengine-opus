#pragma once
#ifndef BE_CORE_OPUS_HPP_
#define BE_CORE_OPUS_HPP_

#include "op.hpp"
#include "op_containers.hpp"
#include <unordered_map>

namespace be {
namespace op {

inline Op default_op_generator(Id) {
   Op op;
   op.action(detail::StaticSet());
   return op;
}

///////////////////////////////////////////////////////////////////////////////
class Opus final : Movable {
   using child_id_list = std::vector<Id>;
   struct op_meta {
      Id parent;
      Handle<Op> op;
      child_id_list children;
      bool children_dirty = false;
      I32 priority = 0;
   };
   using opus_map = std::unordered_map<Id, op_meta>;
   using op_generator = std::function<Op(Id)>;
public:
   using iterator = child_id_list::const_iterator;

   Opus(op_generator op_gen = default_op_generator);

   F64 operator()(F64 dt);

   Op& root();
   Op& operator[](Id id);

   iterator begin() const;
   iterator end() const;

   iterator begin(Id id) const;
   iterator end(Id id) const;

   Op& child(Id parent_id, Id child_id, I32 priority);

   Op& before(Id sibling_id, Id op_id, I32 priority_delta);
   Op& after(Id sibling_id, Id op_id, I32 priority_delta);

   Id parent(Id child_id) const;
   Id parent(Id child_id, Id new_parent_id);

   I32 priority(Id id) const;
   I32 priority(Id id, I32 new_priority);

   bool exists(Id id) const;

   void erase(Id id);
   
private:
   op_meta& get_or_create_(Id id);
   op_meta& get_or_create_with_op_(Id id);

   void clean_();
   void clean_(op_meta& meta);

   Op root_;
   opus_map meta_;
   bool dirty_;
   op_generator op_gen_;
};

// TODO printtraits?

} // be::op
} // be

#endif