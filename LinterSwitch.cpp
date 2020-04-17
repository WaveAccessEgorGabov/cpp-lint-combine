#include "LinterSwitch.h"

int LinterSwitch::callLinter( bool isNeedHelp ) const {
    return linter.get()->callLinter(isNeedHelp);
}

bool LinterSwitch::createUpdatedYaml() const {
    return linter.get()->createUpdatedYaml();
}
