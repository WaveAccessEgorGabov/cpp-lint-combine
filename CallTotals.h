#ifndef __CALLTOTALS_H__
#define __CALLTOTALS_H__

namespace LintCombine {
    struct CallTotals {
        CallTotals() = default;

        CallTotals( const unsigned int success, const unsigned int fail ) : success( success ), fail( fail ) {
        }

        CallTotals & operator+=( const CallTotals & rhs ) {
            this->success += rhs.success;
            this->fail    += rhs.fail;
            return * this;
        }

        unsigned int success = 0;
        unsigned int fail = 0;
    };
}

#endif //__CALLTOTALS_H__
