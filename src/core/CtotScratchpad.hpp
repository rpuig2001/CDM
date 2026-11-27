#pragma once

#include <cctype>
#include <string>

namespace ctot_scratchpad {

struct RemarkToken {
    std::size_t position;
    std::size_t length;
};

inline bool isRemarkBoundary(char character) {
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

inline bool isCtotMarker(char character) {
    return character == 'C' || character == 'c';
}

inline RemarkToken findCtotRemark(const std::string& scratchpad) {
    for (std::size_t position = 0; position + 6 <= scratchpad.size(); ++position) {
        if (scratchpad[position] != '.' || !isCtotMarker(scratchpad[position + 1])) {
            continue;
        }

        bool hasFourDigits = true;
        for (std::size_t digit = position + 2; digit < position + 6; ++digit) {
            if (std::isdigit(static_cast<unsigned char>(scratchpad[digit])) == 0) {
                hasFourDigits = false;
                break;
            }
        }

        if (hasFourDigits &&
            (position + 6 == scratchpad.size() || isRemarkBoundary(scratchpad[position + 6]))) {
            return {position, 6};
        }
    }
    return {std::string::npos, 0};
}

inline std::string setCtotRemark(const std::string& scratchpad, const std::string& ctot) {
    const std::string marker = ".C" + ctot.substr(0, 4);
    const RemarkToken token = findCtotRemark(scratchpad);
    if (token.position != std::string::npos) {
        std::string updated = scratchpad;
        updated.replace(token.position, token.length, marker);
        return updated;
    }
    return scratchpad.empty() ? marker : marker + " " + scratchpad;
}

inline std::string removeCtotRemark(const std::string& scratchpad) {
    const RemarkToken token = findCtotRemark(scratchpad);
    if (token.position == std::string::npos) {
        return scratchpad;
    }

    std::size_t position = token.position;
    std::size_t length = token.length;
    if (position + length < scratchpad.size() && isRemarkBoundary(scratchpad[position + length])) {
        ++length;
    } else if (position > 0 && isRemarkBoundary(scratchpad[position - 1])) {
        --position;
        ++length;
    }

    std::string updated = scratchpad;
    updated.erase(position, length);
    if (token.position == 0 && !updated.empty() && updated[0] != '.' && !isRemarkBoundary(updated[0])) {
        updated.insert(0, 1, '.');
    }
    return updated;
}

}  // namespace ctot_scratchpad