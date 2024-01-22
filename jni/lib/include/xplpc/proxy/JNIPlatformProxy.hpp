#pragma once

#include "xplpc/jni/jni_bind_release.h"
#include "xplpc/jni/support.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <memory>
#include <string>

namespace xplpc
{
namespace proxy
{

class JNIPlatformProxy : public PlatformProxy
{
public:
    static std::shared_ptr<JNIPlatformProxy> shared();

    virtual void initialize() override;
    virtual void initializePlatform() override;
    virtual void finalize() override;
    virtual void finalizePlatform() override;
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void onNativeProxyCallback(const jstring &key, const std::string &data);
    void setPlatformJavaVM(JavaVM *pjvm);

private:
    static std::shared_ptr<JNIPlatformProxy> instance;

    static constexpr ::jni::Class kPlatformProxy{
        "com/xplpc/proxy/PlatformProxy",
        ::jni::Static{
            ::jni::Method{"onInitializePlatform", ::jni::Return<void>{}, ::jni::Params{}},
            ::jni::Method{"onFinalizePlatform", ::jni::Return<void>{}, ::jni::Params{}},
            ::jni::Method{"onNativeProxyCall", ::jni::Return<void>{}, ::jni::Params<jstring, jstring>{}},
            ::jni::Method{"onHasMapping", ::jni::Return<jboolean>{}, ::jni::Params<jstring>{}},
            ::jni::Method{"onNativeProxyCallback", ::jni::Return<void>{}, ::jni::Params<jstring, jstring>{}},
        },
    };

    JavaVM *platformJavaVM;
};

} // namespace proxy
} // namespace xplpc
