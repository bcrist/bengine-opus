#include "pch.hpp"
#include "op_containers.hpp"

namespace be {
namespace op {
namespace detail {

///////////////////////////////////////////////////////////////////////////////
/// \brief  Executes its children one at a time until they have all completed,
///         then marks itself complete.
///
/// \details Each time the op is run, the queue will delegate to whichever
///         child op it is currently "looking at".  When that child's
///         remaining() time is 0, it wll move on to the next child, until all
///         children have finished.
///
///         When there is work left in the queue, the queue's own remaining()
///         time will be set to -1.  When all work is finished, it will be set
///         to 0.
///
///         When called for the first time or when called with a dt of 0, the
///         internal pointer will be reset to the first child of the op.
///
///         If additional children are added to the end of an existing queue,
///         or if the final child's remaining() time is increased after the
///         queue has finished, then if called, the queue will continue with
///         the newly added children or the last child which now has more
///         remaining time, however the queue's remaining() time won't be
///         updated until it is called at least once and discovers that there
///         is more work.
void Queue::operator()(OpData& data, F64& dt) {
   if (!initialized || dt == 0) {
      position = data.children.begin();
      if (position == data.children.end()) {
         initialized = false;
         return;
      } else {
         initialized = true;
      }
   }

   while (dt > 0) {
      Op& op = *position;
      if (op.remaining()) {
         data.remaining = -1;
         op(dt);
         if (op.remaining()) {
            return;
         }
      }
      auto it = position + 1;
      if (it != data.children.end()) {
         position = it;
      } else {
         data.remaining = 0;
         return;
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief  Executes each of its children simultaneously.
///
/// \details Each time the op is run, the set will delegate to each of its
///         children, in order, with a copy of the dt parameter passed to each
///         child.
///
///         When there  work left in the queue, the queue's own remaining()
///         time will be set to -1.  When all work is finished, it will be set
///         to 0.
///
///         When called for the first time or when called with a dt of 0, the
///         internal pointer will be reset to the first child of the op.
///
///         If additional children are added to the end of an existing queue,
///         or if the final child's remaining() time is increased after the
///         queue has finished, then if called, the queue will continue with
///         the newly added children or the last child which now has more
///         remaining time, however the queue's remaining() time won't be
///         updated until it is called at least once and discovers that there
///         is more work.
void Set::operator()(OpData& data, F64& dt) {
   bool finished = true;
   for (Op& op : data.children) {
      if (op.remaining() != 0) {
         finished = false;
         F64 mdt = dt;
         op(mdt);
      }
   }
   data.remaining = finished ? 0 : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief  Works like op::detail::Set, but always executes all children,
///         regardless of whether or not they have finished their work.
///
/// \details Unlike op::detail::Set, StaticSet doesn't check child.remaining()
///         before calling children, and doesn't set its own remaining() value
///         when all tasks finish.  This makes it ideal for static or
///         semi-static configurations where its children are fixed parts of a
///         game loop or long term process.
void StaticSet::operator()(OpData& data, F64& dt) {
   for (Op& op : data.children) {
      F64 mdt = dt;
      op(mdt);
   }
}

} // be::op::detail
} // be::op
} // be
