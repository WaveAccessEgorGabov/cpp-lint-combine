#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

class LinterWrapperItf {
public:
    virtual int callLinter( bool isNeedHelp ) = 0;

    virtual bool createUpdatedYaml() = 0;
};

#endif //__LINTERWRAPPERITF_H__
