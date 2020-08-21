#pragma once

namespace LintCombine {
    struct CallTotals {
        CallTotals() = default;

        CallTotals( const unsigned int successNum, const unsigned int failNum )
            : successNum( successNum ), failNum( failNum ) {}

        CallTotals & operator+=( const CallTotals & rhs ) {
            this->successNum += rhs.successNum;
            this->failNum += rhs.failNum;
            return *this;
        }

        unsigned int successNum = 0;
        unsigned int failNum = 0;
    };
}
