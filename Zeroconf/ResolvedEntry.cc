#include <dns_sd.h>

#include "ResolvedEntry.h"

using namespace SideCar::Zeroconf;

ResolvedEntry::ResolvedEntry(const std::string& fullName, const std::string& host, uint16_t port, uint16_t textSize,
                             const unsigned char* text) :
    fullName_(fullName),
    nativeHost_(host), host_(), port_(port), textEntries_()
{
    // Zeroconf likes to add a '.' to all host entries, even if they are IP addresses. Strip it off of those.
    //
    if (nativeHost_.size() && isdigit(nativeHost_[0]) && nativeHost_[nativeHost_.size() - 1] == '.') {
        nativeHost_.erase(nativeHost_.size() - 1, 1);
    }
    host_ = nativeHost_;
    updateTextEntries(textSize, text);
    hasTextEntry("host", &host_);
}

void
ResolvedEntry::updateTextEntries(uint16_t textSize, const unsigned char* text)
{
    char key[256];
    uint8_t valueLen;
    const void* value;
    uint16_t numRecords = TXTRecordGetCount(textSize, text);
    for (uint16_t index = 0; index < numRecords; ++index) {
        int err = TXTRecordGetItemAtIndex(textSize, text, index, sizeof(key), key, &valueLen, &value);
        if (err == kDNSServiceErr_NoError) { textEntries_[key] = std::string((const char*)(value), valueLen); }
    }
}

bool
ResolvedEntry::hasTextEntry(const std::string& key, std::string* value) const
{
    StringMap::const_iterator pos = textEntries_.find(key);
    if (pos != textEntries_.end()) {
        if (value) *value = pos->second;
        return true;
    }

    return false;
}

std::string
ResolvedEntry::getTextEntry(const std::string& key) const
{
    StringMap::const_iterator pos = textEntries_.find(key);
    return pos != textEntries_.end() ? pos->second : "";
}
