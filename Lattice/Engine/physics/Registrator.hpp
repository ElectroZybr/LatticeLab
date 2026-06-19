#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// базовый виртуальный класс модуля
class IModule {
public:
    virtual ~IModule() = default;
};

template <typename TModule>
using ModuleFactory = std::unique_ptr<TModule> (*)();

template <typename TModule>
struct ModuleMeta {
    std::string id;
    std::string description;
    ModuleFactory<TModule> factory = nullptr;
};

template <typename TModule>
class ModuleRegistry {
public:
    using Meta = ModuleMeta<TModule>;

    bool add(Meta meta) {
        if (meta.id.empty() || meta.factory == nullptr || find(meta.id) != nullptr) {
            return false;
        }

        items_.push_back(std::move(meta));
        return true;
    }

    const Meta* find(std::string_view id) const {
        for (const Meta& item : items_) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

    std::unique_ptr<TModule> create(std::string_view id) const {
        const Meta* meta = find(id);
        return meta != nullptr && meta->factory != nullptr ? meta->factory() : nullptr;
    }

    const std::vector<Meta>& items() const { return items_; }

private:
    std::vector<Meta> items_{};
};

template <typename TDerived, typename TModule>
class RegisteredModuleOwner {
public:
    RegisteredModuleOwner() = default;
    RegisteredModuleOwner(const RegisteredModuleOwner&) = delete;
    RegisteredModuleOwner& operator=(const RegisteredModuleOwner&) = delete;
    RegisteredModuleOwner(RegisteredModuleOwner&&) noexcept = default;
    RegisteredModuleOwner& operator=(RegisteredModuleOwner&&) noexcept = default;

    bool set(std::string_view id) {
        if constexpr (requires { TDerived::allowEmpty; }) {
            if constexpr (TDerived::allowEmpty) {
                if (id.empty()) {
                    currentId_.clear();
                    impl_.reset();
                    return true;
                }
            }
        }

        const ModuleMeta<TModule>* meta = TDerived::registry().find(id);
        if (meta == nullptr || meta->factory == nullptr) {
            return false;
        }

        std::unique_ptr<TModule> impl = meta->factory();
        if (!impl) {
            return false;
        }

        if constexpr (requires(TDerived& self, TModule & module) { self.onModuleSet(module); }) {
            static_cast<TDerived&>(*this).onModuleSet(*impl);
        }

        currentId_ = meta->id;
        impl_ = std::move(impl);
        return true;
    }

    std::string_view get() const { return currentId_; }
    TModule* active() const { return impl_.get(); }

protected:
    std::string currentId_{};
    std::unique_ptr<TModule> impl_{};
};

#ifndef LL_CONCAT_IMPL
#define LL_CONCAT_IMPL(a, b) a##b
#endif

#ifndef LL_CONCAT
#define LL_CONCAT(a, b) LL_CONCAT_IMPL(a, b)
#endif

#define REGISTER_MODULE(Type, ModuleType, RegistryFn, Prefix)                            \
    namespace {                                                                          \
        const bool LL_CONCAT(Prefix, __LINE__) = []() {                                  \
            return RegistryFn().add(ModuleMeta<ModuleType>{                              \
                std::string(Type::id),                                                   \
                std::string(Type::description),                                          \
                []() -> std::unique_ptr<ModuleType> {                                    \
                    return std::make_unique<Type>();                                     \
                },                                                                       \
            });                                                                          \
        }();                                                                             \
    }
