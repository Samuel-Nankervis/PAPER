#include "api/RequiredUpdates.h"

#include "PaperLog.h"

#include <PAPER/Provider.h>
#include <RPS/Runtime/CallerModule.h>

namespace paper::required_updates
{
    namespace
    {
        namespace caller_module = RPS::Runtime::CallerModule;

        // Consumers request the exact major, minor and table size their headers
        // declare (PAPER/Client.h), so every version refusal is a real mismatch.
        bool isVersionRefusal(const rock::api::Status status) noexcept
        {
            using rock::api::Status;
            return status == Status::UnknownInterface || status == Status::UnsupportedMajor ||
                status == Status::UnsupportedMinor || status == Status::TableTooSmall || status == Status::InvalidSize;
        }
    }

    void noteDiscoveryRefusal(const api::Status status, const api::InterfaceId id, const std::uint32_t requestedMajor,
        const void* callerAddress) noexcept
    {
        using api::Status;
        bool consumerOlder = false;
        switch (status) {
        case Status::UnsupportedMajor:
        case Status::UnknownInterface:
            consumerOlder = requestedMajor < api::provider::kMajor;
            break;
        case Status::UnsupportedMinor:
        case Status::TableTooSmall:
            break;
        default:
            return;  // not a version refusal
        }
        if (!caller_module::firstFromCaller(callerAddress))
            return;
        try {
            const auto dll = caller_module::moduleNameForLog(callerAddress);
            if (consumerOlder) {
                PAPER_LOG_ERROR(Api,
                    "{} asked for interface {} major {}: it was built for an older PAPER and gets no PAPER features "
                    "until it is updated.",
                    dll, static_cast<std::uint32_t>(id), requestedMajor);
            } else {
                PAPER_LOG_ERROR(Api,
                    "{} asked for interface {} major {} (status {}): it needs a newer PAPER than {} and gets no PAPER "
                    "features until PAPER is updated.",
                    dll, static_cast<std::uint32_t>(id), requestedMajor, static_cast<std::uint32_t>(status),
                    Version::NAME);
            }
        } catch (...) {}
    }

    void noteRetiredExport(const char* exportName, const void* callerAddress) noexcept
    {
        if (!caller_module::firstFromCaller(callerAddress))
            return;
        try {
            PAPER_LOG_ERROR(Api,
                "{} called {}, PAPER's retired pre-2.0 API: it was built for an older PAPER and gets no PAPER "
                "features until it is updated.",
                caller_module::moduleNameForLog(callerAddress), exportName ? exportName : "a retired export");
        } catch (...) {}
    }

    void noteRockMissingDiscovery() noexcept
    {
        try {
            PAPER_LOG_ERROR(Api,
                "ROCK.dll does not export ROCKAPI_QueryInterfaceV1: it is older than PAPER supports. PAPER stays idle "
                "until ROCK and PAPER match.");
        } catch (...) {}
    }

    void noteRockRefusal(const rock::api::Status status, const char* interfaceName) noexcept
    {
        if (!isVersionRefusal(status))
            return;
        try {
            PAPER_LOG_ERROR(Api,
                "ROCK refused PAPER's {} request with status {}: ROCK and PAPER were built for different ROCK API "
                "versions. PAPER stays idle until they match.",
                interfaceName ? interfaceName : "discovery", static_cast<std::uint32_t>(status));
        } catch (...) {}
    }

    void noteRpsUiWithoutDiscovery() noexcept
    {
        try {
            PAPER_LOG_ERROR(Api,
                "RPS_UI_Framework.dll does not export its 2.0 discovery: it is older than PAPER supports. PAPER's "
                "diagnostic drawing stays off until it is updated.");
        } catch (...) {}
    }
}
