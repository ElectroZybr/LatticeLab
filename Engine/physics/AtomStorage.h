#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <span>
#include <algorithm>
#include "Engine/math/Vec3f.h"

#include "AtomData.h"

class AtomStorage {
private:
    static constexpr size_t kFloatFieldCount = 14;

    size_t count_ = 0;
    size_t capacity_ = 0;
    size_t mobileCount_ = 0;

    std::vector<float> floatData_;

    float* x_ = nullptr;
    float* y_ = nullptr;
    float* z_ = nullptr;
    float* vx_ = nullptr;
    float* vy_ = nullptr;
    float* vz_ = nullptr;
    float* invMass_ = nullptr;
    float* fx_ = nullptr;
    float* fy_ = nullptr;
    float* fz_ = nullptr;
    float* pfx_ = nullptr;
    float* pfy_ = nullptr;
    float* pfz_ = nullptr;
    float* pe_ = nullptr;

    std::vector<AtomData::Type> atomType_;
    std::vector<uint8_t> valence_;

    void bindFloatViews() {
        if (capacity_ == 0 || floatData_.empty()) {
            x_ = y_ = z_ = nullptr;
            vx_ = vy_ = vz_ = nullptr;
            fx_ = fy_ = fz_ = nullptr;
            pfx_ = pfy_ = pfz_ = nullptr;
            pe_ = invMass_ = nullptr;
            return;
        }

        float* base = floatData_.data();
        x_       = base +  0 * capacity_;
        y_       = base +  1 * capacity_;
        z_       = base +  2 * capacity_;
        vx_      = base +  3 * capacity_;
        vy_      = base +  4 * capacity_;
        vz_      = base +  5 * capacity_;
        invMass_ = base +  6 * capacity_;
        fx_      = base +  7 * capacity_;
        fy_      = base +  8 * capacity_;
        fz_      = base +  9 * capacity_;
        pfx_     = base + 10 * capacity_;
        pfy_     = base + 11 * capacity_;
        pfz_     = base + 12 * capacity_;
        pe_      = base + 13 * capacity_;
    }

    void ensureCapacity(size_t requiredCount) {
        if (requiredCount <= capacity_) {
            return;
        }

        size_t newCapacity = (capacity_ == 0) ? requiredCount : capacity_;
        while (newCapacity < requiredCount) {
            newCapacity = newCapacity * 3 / 2 + 1;
        }

        std::vector<float> newFloatData(kFloatFieldCount * newCapacity, 0.0f);

        if (count_ > 0) {
            float* newBase = newFloatData.data();

            auto getNewFieldPtr = [&](size_t fieldIndex) {
                return newBase + (fieldIndex * newCapacity);
            };

            const std::array<float*, kFloatFieldCount> oldFields = {
                x_, y_, z_, 
                fx_, fy_, fz_, pe_, 
                vx_, vy_, vz_, invMass_, 
                pfx_, pfy_, pfz_
            };

            for (size_t i = 0; i < oldFields.size(); ++i) {
                std::copy_n(oldFields[i], count_, getNewFieldPtr(i));
            }
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
    const float* vxData() const { return vx_; }
    const float* vyData() const { return vy_; }
    const float* vzData() const { return vz_; }

    float* fxData() { return fx_; }
    float* fyData() { return fy_; }
    float* fzData() { return fz_; }

    float* pfxData() { return pfx_; }
    float* pfyData() { return pfy_; }
    float* pfzData() { return pfz_; }

    float* energyData() { return pe_; }
    float* invMassData() { return invMass_; }

    AtomData::Type* atomTypeData() { return atomType_.data(); }

    std::span<float> xDataSpan() const { return {x_, count_}; }
    std::span<float> yDataSpan() const { return {y_, count_}; }
    std::span<float> zDataSpan() const { return {z_, count_}; }

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
        bindFloatViews();
        other.count_ = 0;
        other.capacity_ = 0;
        other.mobileCount_ = 0;
        other.bindFloatViews();
        return *this;
    }

    const AtomData::Type* atomTypeData() const { return atomType_.data(); }

    size_t size() const { return count_; }
    size_t mobileCount() const { return mobileCount_; }
    bool empty() const { return count_ == 0; }
    size_t memoryBytes() const {
        return floatData_.capacity() * sizeof(float)
            + atomType_.capacity() * sizeof(AtomData::Type)
            + valence_.capacity() * sizeof(uint8_t);
    }

    void clear() {
        count_ = 0;

        floatData_.clear();
        floatData_.shrink_to_fit();
        capacity_ = 0;
        bindFloatViews();

        mobileCount_ = 0;
        atomType_.clear();
        atomType_.shrink_to_fit();
        valence_.clear();
        valence_.shrink_to_fit();
        // floatData_.clear(); TODO разобраться почему если убрать то бенчмарки падают с segfault
        // bindFloatViews();
    }

    void reserve(size_t count) {
        ensureCapacity(count);
        atomType_.reserve(count);
        valence_.reserve(count);
    }

    void addAtom(const Vec3f& coords, const Vec3f& speed, AtomData::Type type, bool fixed = false) {
        ensureCapacity(count_ + 1);

        x_[count_] = static_cast<float>(coords.x);
        y_[count_] = static_cast<float>(coords.y);
        z_[count_] = static_cast<float>(coords.z);

        vx_[count_] = static_cast<float>(speed.x);
        vy_[count_] = static_cast<float>(speed.y);
        vz_[count_] = static_cast<float>(speed.z);

        invMass_[count_] = 1.0f / AtomData::getProps(type).mass;

        fx_[count_] = 0.0f;
        fy_[count_] = 0.0f;
        fz_[count_] = 0.0f;

        pfx_[count_] = 0.0f;
        pfy_[count_] = 0.0f;
        pfz_[count_] = 0.0f;
        pe_[count_]  = 0.0f;

        atomType_.emplace_back(type);
        valence_.emplace_back(AtomData::getProps(type).maxValence);

        ++count_;

        if (!fixed) {
            // Если не фиксирован то заменяем с 1 фиксированным атомов для сохранения инварианта
            swapAtoms(count_ - 1, mobileCount_);
            ++mobileCount_;
        }
    }

    void swapAtoms(size_t aIndex, size_t bIndex) {
        if (aIndex >= size() || bIndex >= size() || aIndex == bIndex) {
            return;
        }

        std::swap(x_[aIndex], x_[bIndex]);
        std::swap(y_[aIndex], y_[bIndex]);
        std::swap(z_[aIndex], z_[bIndex]);

        std::swap(vx_[aIndex], vx_[bIndex]);
        std::swap(vy_[aIndex], vy_[bIndex]);
        std::swap(vz_[aIndex], vz_[bIndex]);

        std::swap(invMass_[aIndex], invMass_[bIndex]);

        std::swap(fx_[aIndex], fx_[bIndex]);
        std::swap(fy_[aIndex], fy_[bIndex]);
        std::swap(fz_[aIndex], fz_[bIndex]);

        std::swap(pfx_[aIndex], pfx_[bIndex]);
        std::swap(pfy_[aIndex], pfy_[bIndex]);
        std::swap(pfz_[aIndex], pfz_[bIndex]);
        std::swap(pe_[aIndex], pe_[bIndex]);

        std::swap(atomType_[aIndex], atomType_[bIndex]);
        std::swap(valence_[aIndex], valence_[bIndex]);
    }

    void removeAtom(size_t index) {
        if (index >= count_) return;

        const size_t lastIndex = count_ - 1;
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
        --count_;
    }

    float& posX(size_t i) { return x_[i]; }
    float& posY(size_t i) { return y_[i]; }
    float& posZ(size_t i) { return z_[i]; }
    const float& posX(size_t i) const { return x_[i]; }
    const float& posY(size_t i) const { return y_[i]; }
    const float& posZ(size_t i) const { return z_[i]; }

    float& velX(size_t i) { return vx_[i]; }
    float& velY(size_t i) { return vy_[i]; }
    float& velZ(size_t i) { return vz_[i]; }
    const float& velX(size_t i) const { return vx_[i]; }
    const float& velY(size_t i) const { return vy_[i]; }
    const float& velZ(size_t i) const { return vz_[i]; }

    float& invMass(size_t i) { return invMass_[i]; }
    const float& invMass(size_t i) const { return invMass_[i]; }

    float& forceX(size_t i) { return fx_[i]; }
    float& forceY(size_t i) { return fy_[i]; }
    float& forceZ(size_t i) { return fz_[i]; }
    const float& forceX(size_t i) const { return fx_[i]; }
    const float& forceY(size_t i) const { return fy_[i]; }
    const float& forceZ(size_t i) const { return fz_[i]; }

    float& prevForceX(size_t i) { return pfx_[i]; }
    float& prevForceY(size_t i) { return pfy_[i]; }
    float& prevForceZ(size_t i) { return pfz_[i]; }
    const float& prevForceX(size_t i) const { return pfx_[i]; }
    const float& prevForceY(size_t i) const { return pfy_[i]; }
    const float& prevForceZ(size_t i) const { return pfz_[i]; }

    AtomData::Type& type(size_t i) { return atomType_[i]; }
    const AtomData::Type& type(size_t i) const { return atomType_[i]; }

    float& energy(size_t i) { return pe_[i]; }
    const float& energy(size_t i) const { return pe_[i]; }

    uint8_t& valenceCount(size_t i) { return valence_[i]; }
    const uint8_t& valenceCount(size_t i) const { return valence_[i]; }

    bool isAtomFixed(size_t i) const { return i >= mobileCount_; }
    void setFixed(size_t i, bool fixed) {
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

    Vec3f pos(size_t i) const { return Vec3f(x_[i], y_[i], z_[i]); }
    Vec3f vel(size_t i) const { return Vec3f(vx_[i], vy_[i], vz_[i]); }
    Vec3f force(size_t i) const { return Vec3f(fx_[i], fy_[i], fz_[i]); }
    Vec3f prevForce(size_t i) const { return Vec3f(pfx_[i], pfy_[i], pfz_[i]); }

    void setPos(size_t i, const Vec3f& coords) {
        x_[i] = static_cast<float>(coords.x);
        y_[i] = static_cast<float>(coords.y);
        z_[i] = static_cast<float>(coords.z);
    }

    void setVel(size_t i, const Vec3f& speed) {
        vx_[i] = static_cast<float>(speed.x);
        vy_[i] = static_cast<float>(speed.y);
        vz_[i] = static_cast<float>(speed.z);
    }

    void setForce(size_t i, const Vec3f& force) {
        fx_[i] = static_cast<float>(force.x);
        fy_[i] = static_cast<float>(force.y);
        fz_[i] = static_cast<float>(force.z);
    }

    void setPrevForce(size_t i, const Vec3f& prevForce) {
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
