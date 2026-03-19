// Stub header for unit tests — shadows the real Logos SDK module_proxy.h.
// Provides a no-op ModuleProxy so tests can compile without the SDK.
#pragma once
#include <QVariant>
#include <QString>
#include <QVariantList>

class ModuleProxy {
public:
    // Accepts any number of QVariant-convertible arguments; always returns a
    // default-constructed QVariant.
    template<typename... Args>
    QVariant invokeRemoteMethod(const QString& /*module*/,
                                const QString& /*method*/,
                                Args&&... /*args*/)
    {
        return {};
    }
};
