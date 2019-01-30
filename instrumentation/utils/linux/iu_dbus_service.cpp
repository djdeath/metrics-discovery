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

    File Name:  iu_dbus_service.cpp

    Abstract:   Implements setting the frequency of the GPU through a DBus call

\*****************************************************************************/

#include "iu_dbus_service.h"
#include "md_utils.h"

#include <dbus/dbus.h>

using namespace MetricsDiscovery;

namespace MetricsDiscoveryInternal
{

CDBusService::CDBusService()
    : m_Connection(nullptr)
{
    MD_LOG_ENTER();
    MD_LOG_EXIT();
}

CDBusService::~CDBusService()
{
    MD_LOG_ENTER();

    if (m_Connection)
        dbus_connection_close(m_Connection);

    MD_LOG_EXIT();
}

TCompletionCode CDBusService::Init()
{
    if (m_Connection)
        return CC_ALREADY_INITIALIZED;

    DBusError error = DBUS_ERROR_INIT;
    m_Connection =
        dbus_connection_open_private("unix:path=/var/run/dbus/system_bus_socket",
                                     &error);
    if (!m_Connection) {
        dbus_error_free(&error);
        return CC_ERROR_NO_MEMORY;
    }

    if (!dbus_bus_register(m_Connection, &error)) {
        dbus_error_free(&error);
        dbus_connection_close(m_Connection);
        m_Connection = nullptr;
        return CC_ERROR_NO_MEMORY;
    }

    return CC_OK;
}

TCompletionCode CDBusService::AcquireFrequencyOverride(uint32_t device)
{
    if (!m_Connection)
        return CC_ERROR_GENERAL;

    if (m_AcquiredDevices.find(device) != m_AcquiredDevices.end())
        return CC_ALREADY_INITIALIZED;

    DBusMessage *call_message =
        dbus_message_new_method_call("com.intel.GPU",
                                     "/com/intel/GPU",
                                     "com.intel.GPU",
                                     "AcquireFrequencyOverride");
    dbus_bool_t sample_oa = true;
    if (!dbus_message_append_args(call_message,
                                  DBUS_TYPE_UINT32, &device,
                                  DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        return CC_ERROR_NO_MEMORY;
    }

    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *reply_message =
        dbus_connection_send_with_reply_and_block(m_Connection,
                                                  call_message,
                                                  -1 /* wait forever */,
                                                  &error);
    if (!reply_message) {
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    const char *dbus_path;
    if (!dbus_message_get_args(reply_message, &error,
                               DBUS_TYPE_STRING, &dbus_path,
                               DBUS_TYPE_INVALID)) {
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        dbus_clear_message(&reply_message);
        return CC_TRY_AGAIN;
    }

    m_AcquiredDevices.insert(std::pair<uint32_t, std::string>(device, std::string(dbus_path)));

    dbus_clear_message(&call_message);
    dbus_clear_message(&reply_message);

    return CC_OK;
}

TCompletionCode CDBusService::SetFrequency(uint32_t device, uint32_t minFreq, uint32_t maxFreq, uint32_t boostFreq)
{
    auto device_iter = m_AcquiredDevices.find(device);
    if (device_iter == m_AcquiredDevices.end())
        return CC_ERROR_FILE_NOT_FOUND;

    const std::string& dbus_path = device_iter->second;
    DBusMessage *call_message =
        dbus_message_new_method_call("com.intel.GPU",
                                     dbus_path.c_str(),
                                     "com.intel.GPU.Frequency",
                                     "SetFrequency");
    dbus_bool_t sample_oa = true;
    if (!dbus_message_append_args(call_message,
                                  DBUS_TYPE_UINT32, &minFreq,
                                  DBUS_TYPE_UINT32, &maxFreq,
                                  DBUS_TYPE_UINT32, &boostFreq,
                                  DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *reply_message =
        dbus_connection_send_with_reply_and_block(m_Connection, call_message,
                                                  -1 /* wait forever */, &error);
    if (!reply_message) {
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    dbus_clear_message(&call_message);
    dbus_clear_message(&reply_message);

    return CC_OK;
}

TCompletionCode CDBusService::ReleaseFrequencyOverride(uint32_t device)
{
    auto device_iter = m_AcquiredDevices.find(device);
    if (device_iter == m_AcquiredDevices.end())
        return CC_ERROR_FILE_NOT_FOUND;

    const std::string& dbus_path = device_iter->second;
    DBusMessage *call_message =
        dbus_message_new_method_call("com.intel.GPU",
                                     dbus_path.c_str(),
                                     "com.intel.GPU.Frequency",
                                     "Release");
    dbus_bool_t sample_oa = true;
    if (!dbus_message_append_args(call_message, DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *reply_message =
        dbus_connection_send_with_reply_and_block(m_Connection, call_message,
                                                  -1 /* wait forever */, &error);
    if (!reply_message) {
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    dbus_clear_message(&call_message);
    dbus_clear_message(&reply_message);

    return CC_OK;
}

bool CDBusService::IsFrequencyAcquired(uint32_t device)
{
    return m_AcquiredDevices.find(device) != m_AcquiredDevices.end();
}

TCompletionCode CDBusService::RegisterConfiguration(uint32_t device,
                                                    const std::string& uuid,
                                                    const std::vector<iu_i915_perf_config_register>& mux_regs,
                                                    const std::vector<iu_i915_perf_config_register>& boolean_regs,
                                                    const std::vector<iu_i915_perf_config_register>& flex_regs,
                                                    int32_t *config)
{
    if (!m_Connection)
        return CC_ERROR_GENERAL;

    uint32_t n_mux_regs = mux_regs.size();
    uint32_t n_boolean_regs = boolean_regs.size();
    uint32_t n_flex_regs = flex_regs.size();

    const char *c_uuid = uuid.c_str();

    DBusMessage *call_message =
        dbus_message_new_method_call("com.intel.GPU",
                                     "/com/intel/GPU",
                                     "com.intel.GPU",
                                     "RegisterPerformanceConfiguration");
    if (!dbus_message_append_args(call_message,
                                  DBUS_TYPE_UINT32, &device,
                                  DBUS_TYPE_STRING, &c_uuid,
                                  DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
                                  &mux_regs[0], &n_mux_regs,
                                  DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
                                  &boolean_regs[0], &n_boolean_regs,
                                  DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
                                  &flex_regs[0], &n_flex_regs,
                                  DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *reply_message =
        dbus_connection_send_with_reply_and_block(m_Connection, call_message,
                                                  -1 /* wait forever */, &error);
    if (!reply_message) {
        TCompletionCode code =
            dbus_error_has_name(&error, "com.intel.GPU.Error.InUse") ?
            CC_ALREADY_INITIALIZED : CC_ERROR_GENERAL;
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        return code;
    }

    uint32_t metric_id = 0;
    if (!dbus_message_get_args(reply_message, &error,
                               DBUS_TYPE_UINT32, &metric_id,
                               DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        dbus_clear_message(&reply_message);
        return CC_ERROR_GENERAL;
    }

    *config = (int32_t) metric_id;

    dbus_clear_message(&call_message);
    dbus_clear_message(&reply_message);

    return CC_OK;
}

TCompletionCode CDBusService::UnregisterConfiguration(uint32_t device, int32_t config)
{
    if (!m_Connection)
        return CC_ERROR_GENERAL;

    DBusMessage *call_message =
        dbus_message_new_method_call("com.intel.GPU",
                                     "/com/intel/GPU",
                                     "com.intel.GPU",
                                     "UnregisterPerformanceConfiguration");
    if (!dbus_message_append_args(call_message,
                                  DBUS_TYPE_UINT32, &device,
                                  DBUS_TYPE_UINT32, &config,
                                  DBUS_TYPE_INVALID)) {
        dbus_clear_message(&call_message);
        return CC_TRY_AGAIN;
    }

    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *reply_message =
        dbus_connection_send_with_reply_and_block(m_Connection, call_message,
                                                  -1 /* wait forever */, &error);
    if (!reply_message) {
        dbus_error_free(&error);
        dbus_clear_message(&call_message);
        return CC_ERROR_GENERAL;
    }

    dbus_clear_message(&call_message);
    dbus_clear_message(&reply_message);

    return CC_OK;
}

}
