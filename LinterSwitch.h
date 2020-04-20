#ifndef __LINTERSWITCH_H__
#define __LINTERSWITCH_H__

#include "LinterWrapperItf.h"

#include <memory>

class LinterSwitch : public LinterWrapperItf {
public:
    explicit LinterSwitch( const std::shared_ptr < LinterWrapperItf > & receivedLinter )
            : linter( receivedLinter ) {}

    int callLinter( bool isNeedHelp = false ) const override;

    bool createUpdatedYaml() const override;


private:
    std::shared_ptr < LinterWrapperItf > linter;
};

#endif //__LINTERSWITCH_H__
