#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace ZemaxDDE {
    std::vector<std::string> tokenize(std::string_view bufferStr);
    std::string cp1251_to_utf8(const char* data, size_t len);
    std::string wstring_to_utf8(const std::wstring& wstr);
    /// Formats a duration for human-readable logging.
    /// Examples: "2.345s", "10m 30.456s", "1h 2m 3.456s"
    std::string formatDuration(std::chrono::milliseconds dur);

    /// Reply of the GetRefresh/GetUpdate item.
    enum class UpdateStatus { Updated, NotUpdated, Unknown };

    /// "0" -> Updated, "-1" -> NotUpdated, anything else (empty, timeout text,
    /// garbage) -> Unknown. Trims NUL/CR/LF/spaces the DDE payload may carry.
    UpdateStatus parseUpdateStatus(std::string_view buffer);

    /// Verdict of a DDE acknowledgement for a request in flight.
    enum class AckVerdict {
        Accepted,    ///< the server acknowledged the request
        Busy,        ///< the server is busy, the data follows later
        Rejected,    ///< negative acknowledgement
        Unspecified  ///< the acknowledgement carries no readable status
    };

    /// dde.h defines DDEACK.fAck as bit 15 and DDEACK.fBusy as bit 14.
    constexpr UINT_PTR kDdeAckFAck  = 0x8000;
    constexpr UINT_PTR kDdeAckFBusy = 0x4000;

    /// Classifies the status word of a WM_DDE_ACK.
    /// @param status         the status word read from the packed acknowledgement
    /// @param statusReadable true when the packed block carried the flags at
    ///                       all - an empty block yields a meaningless status
    AckVerdict classifyAck(UINT_PTR status, bool statusReadable);

    /// Reads the name of an item atom. Returns an empty string when the atom
    /// is invalid - the conversion is fully checked so a failed lookup can
    /// never leave uninitialised bytes behind.
    std::string atomName(ATOM itemAtom);

    /// True when the item name equals the command that was sent.
    bool itemNameMatches(std::string_view itemName, std::string_view command);
}
