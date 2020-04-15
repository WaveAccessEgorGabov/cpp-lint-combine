#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

class LinterWrapperItf {
public:
    virtual int callLinter() = 0;

    virtual bool createUpdatedYaml() = 0;
};

#endif //__LINTERWRAPPERITF_H__
