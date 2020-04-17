#include "LinterSwitch.h"

int LinterSwitch::callLinter( bool isNeedHelp ) const {
    return linter->callLinter( isNeedHelp );
}

bool LinterSwitch::createUpdatedYaml() const {
    return linter->createUpdatedYaml();
}
