#pragma once
#ifndef BE_CORE_OP_FUNCTIONS_HPP_
#define BE_CORE_OP_FUNCTIONS_HPP_

#include "op.hpp"

namespace be {
namespace op {
namespace detail {

template <typename T>
struct OpFunc {
   operator T&() { return static_cast<T&>(*this); }
   operator const T&() const { return static_cast<const T&>(*this); }
};

struct Empty : OpFunc<Empty> {
   void operator()(OpData& data, F64& dt) {
      BE_IGNORE2(data, dt);
   }
};

enum class DtConsumptionPolicy {
   disable,
   consume,
   consume_all
};
template <DtConsumptionPolicy Value>
using DtConsumptionTag = Tag<DtConsumptionPolicy, Value>;


template <typename F>
struct Wrap : OpFunc<Wrap<F>>, F {
   Wrap(F func = F()) : F(std::move(func)) { }
   void operator()(OpData& data, F64& dt) {
      BE_IGNORE2(data, dt);
      static_cast<F&>(*this)();
   }
};

struct FuncPtrWrap : OpFunc<FuncPtrWrap> {
   FuncPtrWrap(void (*func)()) : func(func) { }
   void operator()(OpData& data, F64& dt) {
      BE_IGNORE2(data, dt);
      func();
   }
   void (*func)();
};

using OpFuncPtr = void (*)(OpData&, F64&);
struct OpFuncPtrWrap : OpFunc<OpFuncPtrWrap> {
   OpFuncPtrWrap(OpFuncPtr func) : func(func) { }
   void operator()(OpData& data, F64& dt) {
      func(data, dt);
   }
   OpFuncPtr func;
};

template <typename F>
struct PostSetCompleted : OpFunc<PostSetCompleted<F>>, F {
   PostSetCompleted(F func = F()) : F(std::move(func)) { }
   void operator()(OpData& data, F64& dt) {
      static_cast<F&>(*this)(data, dt);
      data.remaining = 0;
   }
};

template <typename F>
struct ResetWhenComplete : OpFunc<ResetWhenComplete<F>>, F {
   ResetWhenComplete(F func = F()) : F(std::move(func)) { }
   void operator()(OpData& data, F64& dt) {
      static_cast<F&>(*this)(data, dt);
      if (!data.remaining) {
         F64 zero = 0;
         static_cast<F&>(*this)(data, zero);
      }
   }
};

template <typename F>
struct Resettable : OpFunc<Resettable<F>>, F {
   Resettable(F func = F()) : F(std::move(func)) { }
   void operator()(OpData& data, F64& dt) {
      if (dt == 0) {
         data.remaining = data.total;
      }
      static_cast<F&>(*this)(data, dt);
   }
};

template <typename F, I64 Numer = 1, I64 Denom = 1>
struct StaticResettable : OpFunc<StaticResettable<F, Numer, Denom>>, F {
   StaticResettable(F func = F()) : F(std::move(func)) { }
   void operator()(OpData& data, F64& dt) {
      if (dt == 0) {
         constexpr const F64 val = Numer / (F64)Denom;
         data.remaining = val;
      }
      static_cast<F&>(*this)(data, dt);
   }
};

template <typename F, typename ValueType = F64>
struct StatefulResettable : OpFunc<StatefulResettable<F, ValueType>>, F {
   StatefulResettable(ValueType reset_value, F func = F())
      : F(std::move(func)),
        val(std::move(reset_value))
   { }

   void operator()(OpData& data, F64& dt) {
      if (dt == 0) {
         data.remaining = static_cast<F64>(val);
      }
      static_cast<F&>(*this)(data, dt);
   }
   ValueType val;
};

template <typename F, typename ValueFunc>
struct DynamicResettable : OpFunc<DynamicResettable<F, ValueFunc>>, F, ValueFunc {
   DynamicResettable(ValueFunc reset_func = ValueFunc(), F func = F())
      : F(std::move(func)),
        ValueFunc(std::move(reset_func))
   { }

   void operator()(OpData& data, F64& dt) {
      if (dt == 0) {
         data.remaining = static_cast<V&>(*this)();
      }
      static_cast<F&>(*this)(data, dt);
   }
};

template <typename F, I64 Numer, I64 Denom = 1, DtConsumptionPolicy Dtcp = DtConsumptionPolicy::consume>
struct StaticTimestretch : OpFunc<StaticTimestretch<F, Numer, Denom, Dtcp>>, F {
   StaticTimestretch(F func = F()) : F(std::move(func)) { }

   void operator()(OpData& data, F64& dt) {
      exec(data, dt, DtConsumptionTag<Dtcp>());
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::disable>) {
      constexpr const F64 f = Numer / (F64)Denom;
      F64 mdt = dt * f;
      static_cast<F&>(*this)(data, mdt);
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume>) {
      constexpr const F64 f = Numer / (F64)Denom;
      F64 mdt = dt * f;
      static_cast<F&>(*this)(data, mdt);
      dt = mdt / f;
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume_all>) {
      constexpr const F64 f = Numer / (F64)Denom;
      F64 mdt = dt * f;
      static_cast<F&>(*this)(data, mdt);
      dt = 0;
   }
};

template <typename F, typename ValueType, DtConsumptionPolicy Dtcp = DtConsumptionPolicy::consume>
struct StatefulTimestretch : OpFunc<StatefulTimestretch<F, ValueType, Dtcp>>, F {
   StatefulTimestretch(ValueType factor, F func = F())
      : F(std::move(func)),
        factor(std::move(factor))
   { }

   void operator()(OpData& data, F64& dt) {
      exec(data, dt, DtConsumptionTag<Dtcp>());
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::disable>) {
      F64 mdt = dt * factor;
      static_cast<F&>(*this)(data, mdt);
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume>) {
      F64 mdt = dt * factor;
      static_cast<F&>(*this)(data, mdt);
      dt = mdt / factor;
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume_all>) {
      F64 mdt = dt * factor;
      static_cast<F&>(*this)(data, mdt);
      dt = 0;
   }

   ValueType factor;
};

template <typename F, typename FactorFunc, DtConsumptionPolicy Dtcp = DtConsumptionPolicy::consume>
struct DynamicTimestretch : OpFunc<DynamicTimestretch<F, FactorFunc, Dtcp>>, F, FactorFunc {
   DynamicTimestretch(FactorFunc factor_func, F func = F())
      : F(std::move(func)),
        FactorFunc(std::move(factor_func))
   { }

   void operator()(OpData& data, F64& dt) {
      exec(data, dt, DtConsumptionTag<Dtcp>());
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::disable>) {
      F64 mdt = dt * factor_func();
      static_cast<F&>(*this)(data, mdt);
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume>) {
      const F64 f = factor_func();
      F64 mdt = dt * f;
      static_cast<F&>(*this)(data, mdt);
      dt = mdt / f;
   }

   void exec(OpData& data, F64& dt, DtConsumptionTag<DtConsumptionPolicy::consume_all>) {
      F64 mdt = dt * factor_func();
      static_cast<F&>(*this)(data, mdt);
      dt = 0;
   }
};

// TODO interpolator
// TODO delay
// TODO timedWrap
// TODO perftimed
// TODO consumable


} // be::op::detail
} // be::op
} // be

#endif
