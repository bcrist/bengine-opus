#include "pch.hpp"
#include "opus.hpp"
#include "logging.hpp"

namespace be {
namespace op {

///////////////////////////////////////////////////////////////////////////////
Opus::Opus(op_generator op_gen)
   : root_(op_gen(Id())),
     dirty_(false),
     op_gen_(std::move(op_gen))
{
   op_meta rootMeta;
   rootMeta.op = static_cast<Handle<Op>>(root_);
   meta_.emplace(Id(), std::move(rootMeta));
}

///////////////////////////////////////////////////////////////////////////////
F64 Opus::operator()(F64 dt) {
   if (dirty_) {
      clean_();
   }
   root_(dt);
   return dt;
}

///////////////////////////////////////////////////////////////////////////////
Op& Opus::root() {
   return root_;
}

///////////////////////////////////////////////////////////////////////////////
Op& Opus::operator[](Id id) {
   return *get_or_create_with_op_(id).op;
}

///////////////////////////////////////////////////////////////////////////////
Opus::iterator Opus::begin() const {
   return begin(Id());
}

///////////////////////////////////////////////////////////////////////////////
Opus::iterator Opus::end() const {
   return end(Id());
}

///////////////////////////////////////////////////////////////////////////////
Opus::iterator Opus::begin(Id id) const {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      return it->second.children.begin();
   }
   return iterator();
}

///////////////////////////////////////////////////////////////////////////////
Opus::iterator Opus::end(Id id) const {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      return it->second.children.end();
   }
   return iterator();
}

///////////////////////////////////////////////////////////////////////////////
Op& Opus::child(Id parent_id, Id child_id, I32 priority) {
   assert((U64)child_id);

   op_meta* parent = nullptr;
   op_meta* meta;

   auto it = meta_.find(child_id);
   if (it != meta_.end()) {
      meta = &it->second;
      // ID exists
      if (meta->parent != parent_id) {
         // move from old parent to new

         parent = &get_or_create_with_op_(parent_id);
         op_meta& old_parent = get_or_create_(meta->parent);

         // remove from old_parent meta children list
         // don't need to mark dirty because it it doesn't change the ordering of other ops
         auto it2 = std::find(old_parent.children.begin(), old_parent.children.end(), child_id);
         if (it2 != old_parent.children.end()) {
            old_parent.children.erase(it2);
         } else {
            be_error() << "Op ID not found in old parent!"
               & attr(ids::log_attr_op_id) << child_id
               & attr(ids::log_attr_old_parent_id) << meta->parent
               & attr(ids::log_attr_new_parent_id) << parent_id
               | default_log();
         }

         // add to parent meta children list
         parent->children.push_back(child_id);
         parent->children_dirty = true;
         dirty_ = true;

         if (old_parent.op) {
            // if old parent is alive, see if we need to move the op
            Op* op = meta->op.get();
            if (op) {
               auto& old_children = old_parent.op->data_.children;
               auto it3 = std::find_if(old_children.begin(), old_children.end(), [=](const Op& child) { return &child == op; });
               if (it3 != old_children.end()) {
                  parent->op->data_.children.push_back(std::move(*it3));
                  old_children.erase(it3);
               } else {
                  // something's wrong...
                  be_error() << "Op not found in old parent!"
                     & attr(ids::log_attr_op_id) << child_id
                     & attr(ids::log_attr_old_parent_id) << meta->parent
                     & attr(ids::log_attr_new_parent_id) << parent_id
                     | default_log();

                  meta->op = Handle<Op>();
               }
            }
         }
         
         meta->parent = parent_id;
      }

      if (meta->priority != priority) {
         meta->priority = priority;
         if (!parent) {
            parent = &get_or_create_with_op_(parent_id);
         }
         parent->children_dirty = true;
         dirty_ = true;
      }

      if (meta->op) {
         return *meta->op;
      }
   } else {
      // doesn't exist yet; create it.
      op_meta newMeta;
      auto result = meta_.emplace(child_id, newMeta);
      meta = &result.first->second;
   }

   if (!parent) {
      parent = &get_or_create_with_op_(parent_id);
   }

   auto& children = parent->op->data_.children;
   children.push_back(op_gen_(child_id));
   parent->children_dirty = true;
   dirty_ = true;
   meta->op = static_cast<Handle<Op>>(children.back());

   return children.back();
}

///////////////////////////////////////////////////////////////////////////////
Op& Opus::before(Id sibling_id, Id op_id, I32 priority_delta) {
   assert((U64)sibling_id);
   assert((U64)op_id);

   op_meta& sibling = get_or_create_(sibling_id);
   return child(sibling.parent, op_id, sibling.priority + priority_delta);
}

///////////////////////////////////////////////////////////////////////////////
Op& Opus::after(Id sibling_id, Id op_id, I32 priority_delta) {
   assert((U64)sibling_id);
   assert((U64)op_id);

   op_meta& sibling = get_or_create_(sibling_id);
   return child(sibling.parent, op_id, sibling.priority - priority_delta);
}

///////////////////////////////////////////////////////////////////////////////
Id Opus::parent(Id child_id) const {
   auto it = meta_.find(child_id);
   if (it != meta_.end()) {
      return it->second.parent;
   }
   return Id();
}

///////////////////////////////////////////////////////////////////////////////
Id Opus::parent(Id child_id, Id new_parent_id) {
   op_meta& meta = get_or_create_(child_id);
   Id old_parent_id = meta.parent;
   
   if (meta.parent != new_parent_id) {
      // move from old parent to new

      op_meta& parent = get_or_create_with_op_(new_parent_id);
      op_meta& old_parent = get_or_create_(meta.parent);

      // remove from old_parent meta children list
      // don't need to mark dirty because it it doesn't change the ordering of other ops
      auto it = std::find(old_parent.children.begin(), old_parent.children.end(), child_id);
      if (it != old_parent.children.end()) {
         old_parent.children.erase(it);
      } else {
         be_error() << "Op ID not found in old parent!"
            & attr(ids::log_attr_op_id) << child_id
            & attr(ids::log_attr_old_parent_id) << old_parent_id
            & attr(ids::log_attr_new_parent_id) << new_parent_id
            | default_log();
      }

      // add to parent meta children list
      parent.children.push_back(child_id);
      parent.children_dirty = true;
      dirty_ = true;

      if (old_parent.op) {
         // if old parent is alive, see if we need to move the op
         Op* op = meta.op.get();
         if (op) {
            auto& old_children = old_parent.op->data_.children;
            auto it2 = std::find_if(old_children.begin(), old_children.end(), [=](const Op& child) { return &child == op; });
            if (it2 != old_children.end()) {
               parent.op->data_.children.push_back(std::move(*it2));
               old_children.erase(it2);
            } else {
               // something's wrong...
               be_error() << "Op not found in old parent!"
                  & attr(ids::log_attr_op_id) << child_id
                  & attr(ids::log_attr_old_parent_id) << old_parent_id
                  & attr(ids::log_attr_new_parent_id) << new_parent_id
                  | default_log();

               meta.op = Handle<Op>();
            }
         }
      }

      meta.parent = new_parent_id;
   }

   return old_parent_id;
}

///////////////////////////////////////////////////////////////////////////////
I32 Opus::priority(Id id) const {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      return it->second.priority;
   }
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
I32 Opus::priority(Id id, I32 new_priority) {
   op_meta& meta = get_or_create_(id);
   I32 old_priority = meta.priority;
   
   if (meta.priority != new_priority) {
      meta.priority = new_priority;
      op_meta& parent = get_or_create_(meta.parent);
      parent.children_dirty = true;
      dirty_ = true;
   }

   return old_priority;
}

///////////////////////////////////////////////////////////////////////////////
bool Opus::exists(Id id) const {
   return meta_.count(id) != 0;
}

///////////////////////////////////////////////////////////////////////////////
void Opus::erase(Id id) {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      op_meta& meta = it->second;
      
      // erase children
      while (!meta.children.empty()) {
         erase(meta.children.back());
      }

      op_meta& parent = get_or_create_(meta.parent);

      // remove from old_parent meta children list
      // don't need to mark dirty because it it doesn't change the ordering of other ops
      auto it2 = std::find(parent.children.begin(), parent.children.end(), id);
      if (it2 != parent.children.end()) {
         parent.children.erase(it2);
      } else {
         be_error() << "Op ID not found in parent!"
            & attr(ids::log_attr_op_id) << id
            & attr(ids::log_attr_parent_id) << meta.parent
            | default_log();
      }

      if (parent.op) {
         // if old parent is alive, see if we need to remove the op
         Op* op = meta.op.get();
         if (op) {
            auto& old_children = parent.op->data_.children;
            auto it3 = std::find_if(old_children.begin(), old_children.end(), [=](const Op& child) { return &child == op; });
            if (it3 != old_children.end()) {
               old_children.erase(it3);
            } else {
               // something's wrong...
               be_error() << "Op not found in parent!"
                  & attr(ids::log_attr_op_id) << id
                  & attr(ids::log_attr_parent_id) << meta.parent
                  | default_log();

               meta.op = Handle<Op>();
            }
         }
      }

      // re-find() `it` since we might have erased other children and invalidated `it`
      meta_.erase(meta_.find(id));
   }
}

///////////////////////////////////////////////////////////////////////////////
Opus::op_meta& Opus::get_or_create_(Id id) {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      return it->second;
   }

   // doesn't exist, create it as a child of root_
   op_meta newMeta;
   auto result = meta_.emplace(id, newMeta);

   op_meta rootMeta = meta_[Id()];
   rootMeta.children.push_back(id);
   rootMeta.children_dirty = true;
   dirty_ = true;

   return result.first->second;
}

///////////////////////////////////////////////////////////////////////////////
Opus::op_meta& Opus::get_or_create_with_op_(Id id) {
   auto it = meta_.find(id);
   if (it != meta_.end()) {
      op_meta& meta = it->second;
      // ID exists; check to make sure handle is valid
      if (meta.op) {
         return meta;
      } else {
         // Op died; find parent and recreate it
         op_meta& parent = get_or_create_with_op_(meta.parent);
         auto& children = parent.op->data_.children;
         children.push_back(op_gen_(id));
         meta.op = static_cast<Handle<Op>>(children.back());
         parent.children_dirty = true;
         dirty_ = true;
         return meta;
      }
   } else {
      // doesn't exist, create it as a child of root_
      op_meta newMeta;
      root_.data_.children.push_back(op_gen_(id));
      newMeta.op = static_cast<Handle<Op>>(root_.data_.children.back());
      auto result = meta_.emplace(id, newMeta);

      op_meta rootMeta = meta_[Id()];
      rootMeta.children.push_back(id);
      rootMeta.children_dirty = true;
      dirty_ = true;

      return result.first->second;
   }
}

///////////////////////////////////////////////////////////////////////////////
void Opus::clean_() {
   if (dirty_) {
      for (auto& p : meta_) {
         clean_(p.second);
      }
      dirty_ = false;
   }
}

///////////////////////////////////////////////////////////////////////////////
void Opus::clean_(op_meta& meta) {
   if (!meta.children_dirty) {
      return;
   }
   
   Op* op = meta.op.get();
   if (op) {
      auto& kids = meta.children;
      auto& op_kids = op->data_.children;

      std::stable_sort(kids.begin(), kids.end(), [this](Id a, Id b) {
         return priority(a) > priority(b);
      });

      auto op_it = op_kids.begin();
      for (Id id : kids) {
         if (op_it == op_kids.end()) {
            break;
         }

         auto it = meta_.find(id);
         if (it != meta_.end()) {
            op_meta& child = it->second;
            Op* child_op = child.op.get();
            if (child_op) {
               // swap *op_it with *child_op
               using std::swap;
               swap(*op_it, *child_op);
               ++op_it;
            }
         }
      }
      
   }

   meta.children_dirty = false;
}

} // be::op
} // be
