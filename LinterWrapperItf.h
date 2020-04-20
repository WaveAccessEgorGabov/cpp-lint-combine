#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

class LinterWrapperItf {
public:
    [[nodiscard]] virtual int callLinter( bool isNeedHelp = false ) const = 0;

    [[nodiscard]] virtual bool createUpdatedYaml() const = 0;

    virtual ~LinterWrapperItf() = default;
};

#endif //__LINTERWRAPPERITF_H__
