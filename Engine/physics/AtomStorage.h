#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "AtomData.h"

class AtomStorage {
private:
    static constexpr std::size_t kFloatFieldCount = 13;

    std::size_t count_ = 0;
    std::size_t capacity_ = 0;
    std::size_t mobileCount_ = 0;

    std::vector<float> floatData_;

    float* x_ = nullptr;
    float* y_ = nullptr;
    float* z_ = nullptr;
    float* vx_ = nullptr;
    float* vy_ = nullptr;
    float* vz_ = nullptr;
    float* fx_ = nullptr;
    float* fy_ = nullptr;
    float* fz_ = nullptr;
    float* pfx_ = nullptr;
    float* pfy_ = nullptr;
    float* pfz_ = nullptr;
    float* pe_ = nullptr;

    std::vector<AtomData::Type> atomType_;
    std::vector<std::uint8_t> valence_;
    std::vector<std::uint8_t> selected_;

    void bindFloatViews() {
        if (capacity_ == 0 || floatData_.empty()) {
            x_ = y_ = z_ = nullptr;
            vx_ = vy_ = vz_ = nullptr;
            fx_ = fy_ = fz_ = nullptr;
            pfx_ = pfy_ = pfz_ = nullptr;
            pe_ = nullptr;
            return;
        }

        float* base = floatData_.data();
        x_   = base +  0 * capacity_;
        y_   = base +  1 * capacity_;
        z_   = base +  2 * capacity_;
        vx_  = base +  3 * capacity_;
        vy_  = base +  4 * capacity_;
        vz_  = base +  5 * capacity_;
        fx_  = base +  6 * capacity_;
        fy_  = base +  7 * capacity_;
        fz_  = base +  8 * capacity_;
        pfx_ = base +  9 * capacity_;
        pfy_ = base + 10 * capacity_;
        pfz_ = base + 11 * capacity_;
        pe_  = base + 12 * capacity_;
    }

    void ensureCapacity(std::size_t requiredCount) {
        if (requiredCount <= capacity_) {
            return;
        }

        std::size_t newCapacity = (capacity_ == 0) ? requiredCount : capacity_;
        while (newCapacity < requiredCount) {
            newCapacity *= 2;
        }

        std::vector<float> newFloatData(kFloatFieldCount * newCapacity, 0.0f);
        float* newBase = newFloatData.data();
        auto newField = [&](std::size_t fieldIndex) {
            return newBase + fieldIndex * newCapacity;
        };

        for (std::size_t i = 0; i < count_; ++i) {
            newField(0)[i]  = x_[i];
            newField(1)[i]  = y_[i];
            newField(2)[i]  = z_[i];
            newField(3)[i]  = vx_[i];
            newField(4)[i]  = vy_[i];
            newField(5)[i]  = vz_[i];
            newField(6)[i]  = fx_[i];
            newField(7)[i]  = fy_[i];
            newField(8)[i]  = fz_[i];
            newField(9)[i]  = pfx_[i];
            newField(10)[i] = pfy_[i];
            newField(11)[i] = pfz_[i];
            newField(12)[i] = pe_[i];
        }

        floatData_ = std::move(newFloatData);
        capacity_ = newCapacity;
        bindFloatViews();
    }

public:
    float* xData() const { return x_; }
    float* yData() const { return y_; }
    float* zData() const { return z_; }

    float* vxData() { return vx_; }
    float* vyData() { return vy_; }
    float* vzData() { return vz_; }

    float* fxData() { return fx_; }
    float* fyData() { return fy_; }
    float* fzData() { return fz_; }

    float* pfxData() { return pfx_; }
    float* pfyData() { return pfy_; }
    float* pfzData() { return pfz_; }

    float* energyData() { return pe_; }

    AtomStorage() = default;
    AtomStorage(const AtomStorage&) = delete;
    AtomStorage& operator=(const AtomStorage&) = delete;

    AtomStorage(AtomStorage&& other) noexcept
        : count_(other.count_)
        , capacity_(other.capacity_)
        , mobileCount_(other.mobileCount_)
        , floatData_(std::move(other.floatData_))
        , atomType_(std::move(other.atomType_))
        , valence_(std::move(other.valence_))
        , selected_(std::move(other.selected_))
    {
        bindFloatViews();
        other.count_ = 0;
        other.capacity_ = 0;
        other.mobileCount_ = 0;
        other.bindFloatViews();
    }
    AtomStorage& operator=(AtomStorage&& other) noexcept {
        if (this == &other) return *this;
        count_ = other.count_;
        capacity_ = other.capacity_;
        mobileCount_ = other.mobileCount_;
        floatData_ = std::move(other.floatData_);
        atomType_ = std::move(other.atomType_);
        valence_ = std::move(other.valence_);
        selected_ = std::move(other.selected_);
        bindFloatViews();
        other.count_ = 0;
        other.capacity_ = 0;
        other.mobileCount_ = 0;
        other.bindFloatViews();
        return *this;
    }

    const AtomData::Type* atomTypeData() const { return atomType_.data(); }
    const std::uint8_t* selectedData() const { return selected_.data(); }
    std::uint8_t* selectedData() { return selected_.data(); }

    std::size_t size() const { return count_; }
    std::size_t mobileCount() const { return mobileCount_; }
    bool empty() const { return count_ == 0; }
    std::size_t memoryBytes() const {
        return floatData_.capacity() * sizeof(float)
            + atomType_.capacity() * sizeof(AtomData::Type)
            + valence_.capacity() * sizeof(std::uint8_t)
            + selected_.capacity() * sizeof(std::uint8_t);
    }

    void clear() {
        count_ = 0;
        mobileCount_ = 0;
        atomType_.clear();
        valence_.clear();
        selected_.clear();
        // floatData_.clear(); TODO разобраться почему если убрать то бенчмарки падают с segfault
        // bindFloatViews();
    }

    void reserve(std::size_t count) {
        ensureCapacity(count);
        atomType_.reserve(count);
        valence_.reserve(count);
        selected_.reserve(count);
    }

    void addAtom(const Vec3f& coords, const Vec3f& speed, AtomData::Type type, bool fixed = false) {
        ensureCapacity(count_ + 1);

        x_[count_] = static_cast<float>(coords.x);
        y_[count_] = static_cast<float>(coords.y);
        z_[count_] = static_cast<float>(coords.z);

        vx_[count_] = static_cast<float>(speed.x);
        vy_[count_] = static_cast<float>(speed.y);
        vz_[count_] = static_cast<float>(speed.z);

        fx_[count_] = 0.0f;
        fy_[count_] = 0.0f;
        fz_[count_] = 0.0f;

        pfx_[count_] = 0.0f;
        pfy_[count_] = 0.0f;
        pfz_[count_] = 0.0f;
        pe_[count_]  = 0.0f;

        atomType_.emplace_back(type);
        valence_.emplace_back(AtomData::getProps(type).maxValence);
        selected_.emplace_back(0);

        ++count_;

        if (!fixed) {
            // Если не фиксирован то заменяем с 1 фиксированным атомов для сохранения инварианта
            swapAtoms(count_ - 1, mobileCount_);
            ++mobileCount_;
        }
    }

    void swapAtoms(std::size_t aIndex, std::size_t bIndex) {
        if (aIndex >= size() || bIndex >= size() || aIndex == bIndex) {
            return;
        }

        std::swap(x_[aIndex], x_[bIndex]);
        std::swap(y_[aIndex], y_[bIndex]);
        std::swap(z_[aIndex], z_[bIndex]);

        std::swap(vx_[aIndex], vx_[bIndex]);
        std::swap(vy_[aIndex], vy_[bIndex]);
        std::swap(vz_[aIndex], vz_[bIndex]);

        std::swap(fx_[aIndex], fx_[bIndex]);
        std::swap(fy_[aIndex], fy_[bIndex]);
        std::swap(fz_[aIndex], fz_[bIndex]);

        std::swap(pfx_[aIndex], pfx_[bIndex]);
        std::swap(pfy_[aIndex], pfy_[bIndex]);
        std::swap(pfz_[aIndex], pfz_[bIndex]);
        std::swap(pe_[aIndex], pe_[bIndex]);

        std::swap(atomType_[aIndex], atomType_[bIndex]);
        std::swap(valence_[aIndex], valence_[bIndex]);
        std::swap(selected_[aIndex], selected_[bIndex]);
    }

    void removeAtom(std::size_t index) {
        if (index >= count_) return;

        const std::size_t lastIndex = count_ - 1;
        if (index < mobileCount_) {
            swapAtoms(index, mobileCount_ - 1);
            --mobileCount_;
            swapAtoms(mobileCount_, lastIndex);
        }
        else {
            if (index != lastIndex) {
                swapAtoms(index, lastIndex);
            }
        }

        atomType_.pop_back();
        valence_.pop_back();
        selected_.pop_back();
        --count_;
    }

    float& posX(std::size_t i) { return x_[i]; }
    float& posY(std::size_t i) { return y_[i]; }
    float& posZ(std::size_t i) { return z_[i]; }
    const float& posX(std::size_t i) const { return x_[i]; }
    const float& posY(std::size_t i) const { return y_[i]; }
    const float& posZ(std::size_t i) const { return z_[i]; }

    float& velX(std::size_t i) { return vx_[i]; }
    float& velY(std::size_t i) { return vy_[i]; }
    float& velZ(std::size_t i) { return vz_[i]; }
    const float& velX(std::size_t i) const { return vx_[i]; }
    const float& velY(std::size_t i) const { return vy_[i]; }
    const float& velZ(std::size_t i) const { return vz_[i]; }

    float& forceX(std::size_t i) { return fx_[i]; }
    float& forceY(std::size_t i) { return fy_[i]; }
    float& forceZ(std::size_t i) { return fz_[i]; }
    const float& forceX(std::size_t i) const { return fx_[i]; }
    const float& forceY(std::size_t i) const { return fy_[i]; }
    const float& forceZ(std::size_t i) const { return fz_[i]; }

    float& prevForceX(std::size_t i) { return pfx_[i]; }
    float& prevForceY(std::size_t i) { return pfy_[i]; }
    float& prevForceZ(std::size_t i) { return pfz_[i]; }
    const float& prevForceX(std::size_t i) const { return pfx_[i]; }
    const float& prevForceY(std::size_t i) const { return pfy_[i]; }
    const float& prevForceZ(std::size_t i) const { return pfz_[i]; }

    AtomData::Type& type(std::size_t i) { return atomType_[i]; }
    const AtomData::Type& type(std::size_t i) const { return atomType_[i]; }

    float& energy(std::size_t i) { return pe_[i]; }
    const float& energy(std::size_t i) const { return pe_[i]; }

    std::uint8_t& valenceCount(std::size_t i) { return valence_[i]; }
    const std::uint8_t& valenceCount(std::size_t i) const { return valence_[i]; }

    bool isSelected(std::size_t i) const { return selected_[i] != 0; }
    void setSelected(std::size_t i, bool value) { selected_[i] = value ? 1 : 0; }

    bool isAtomFixed(std::size_t i) const { return i >= mobileCount_; }
    void setFixed(std::size_t i, bool fixed) {
        if (fixed) {
            if (i >= mobileCount_) return;
            --mobileCount_;
            swapAtoms(i, mobileCount_); // последний мобильный встаёт на место i
        }
        else {
            if (i < mobileCount_) return;
            swapAtoms(i, mobileCount_); // первый фиксированный встаёт на место i
            ++mobileCount_;
        }
    }

    Vec3f pos(std::size_t i) const { return Vec3f(x_[i], y_[i], z_[i]); }
    Vec3f vel(std::size_t i) const { return Vec3f(vx_[i], vy_[i], vz_[i]); }
    Vec3f force(std::size_t i) const { return Vec3f(fx_[i], fy_[i], fz_[i]); }
    Vec3f prevForce(std::size_t i) const { return Vec3f(pfx_[i], pfy_[i], pfz_[i]); }

    void setPos(std::size_t i, const Vec3f& coords) {
        x_[i] = static_cast<float>(coords.x);
        y_[i] = static_cast<float>(coords.y);
        z_[i] = static_cast<float>(coords.z);
    }

    void setVel(std::size_t i, const Vec3f& speed) {
        vx_[i] = static_cast<float>(speed.x);
        vy_[i] = static_cast<float>(speed.y);
        vz_[i] = static_cast<float>(speed.z);
    }

    void setForce(std::size_t i, const Vec3f& force) {
        fx_[i] = static_cast<float>(force.x);
        fy_[i] = static_cast<float>(force.y);
        fz_[i] = static_cast<float>(force.z);
    }

    void setPrevForce(std::size_t i, const Vec3f& prevForce) {
        pfx_[i] = static_cast<float>(prevForce.x);
        pfy_[i] = static_cast<float>(prevForce.y);
        pfz_[i] = static_cast<float>(prevForce.z);
    }

    void swapPrevCurrentForces() {
        std::swap(pfx_, fx_);
        std::swap(pfy_, fy_);
        std::swap(pfz_, fz_);
    }
};
