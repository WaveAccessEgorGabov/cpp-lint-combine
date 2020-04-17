#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

class LinterWrapperItf {
public:
    [[nodiscard]] virtual int callLinter( bool isNeedHelp ) const = 0;

    [[nodiscard]] virtual bool createUpdatedYaml() const = 0;
};

#endif //__LINTERWRAPPERITF_H__
