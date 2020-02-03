#pragma once

#include "iss.h"
#include "core/common/mmu.h"

namespace rv32 {

    typedef GenericMMU<ISS> MMU;

}  // namespace rv64