#pragma once
#ifndef BE_CORE_SERVICE_OPUS_HPP_
#define BE_CORE_SERVICE_OPUS_HPP_

#include "service.hpp"
#include "opus.hpp"

namespace be {

///////////////////////////////////////////////////////////////////////////////
template <>
struct SuppressUndefinedService<op::Opus> : True { };

///////////////////////////////////////////////////////////////////////////////
template <>
struct ServiceTraits<op::Opus> : ServiceTraits<> {
   using lazy_ids = yes;
};

///////////////////////////////////////////////////////////////////////////////
template <>
struct ServiceName<op::Opus> {
   const char* operator()() {
      return "Opus";
   }
};

///////////////////////////////////////////////////////////////////////////////
template <>
struct ServiceInitDependencies<op::Opus> {
   void operator()();
};

} // be

#endif
