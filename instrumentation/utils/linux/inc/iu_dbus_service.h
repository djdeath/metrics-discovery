/*****************************************************************************\

    Copyright © 2019, Intel Corporation

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.

    File Name:  iu_dbus_service.h

    Abstract:   Contains definitions for dbus service usage

\*****************************************************************************/
#pragma once

#include <inttypes.h>
#include <map>
#include <string>
#include <vector>

#include "metrics_discovery_api.h"
#include "iu_i915_perf.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct DBusConnection DBusConnection;

using namespace MetricsDiscovery;

namespace MetricsDiscoveryInternal
{

class CDBusService
{
public:
    CDBusService();
    ~CDBusService();

public:
    TCompletionCode Init();

    TCompletionCode AcquireFrequencyOverride(uint32_t device);
    TCompletionCode SetFrequency(uint32_t device, uint32_t minFreq, uint32_t maxFreq, uint32_t boostFreq);
    TCompletionCode ReleaseFrequencyOverride(uint32_t device);
    bool            IsFrequencyAcquired(uint32_t device);

    TCompletionCode RegisterConfiguration(uint32_t device,
                                          const std::string& uuid,
                                          const std::vector<iu_i915_perf_config_register>& mux_regs,
                                          const std::vector<iu_i915_perf_config_register>& boolean_regs,
                                          const std::vector<iu_i915_perf_config_register>& flex_regs,
                                          int32_t *config);
    TCompletionCode UnregisterConfiguration(uint32_t device, int32_t config);

private:
    std::map<uint32_t, std::string> m_AcquiredDevices;
    DBusConnection *m_Connection;
};

}

#if defined(__cplusplus)
} // extern "C"
#endif
