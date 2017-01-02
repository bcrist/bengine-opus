#include "pch.hpp"
#include "service_opus.hpp"
#include "service_helpers.hpp"
#include "service_log.hpp"

namespace be {

///////////////////////////////////////////////////////////////////////////////
void ServiceInitDependencies<op::Opus>::operator()() {
   default_log();
}

} // be
