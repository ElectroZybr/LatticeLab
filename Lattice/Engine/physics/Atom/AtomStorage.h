#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <span>
#include <type_traits>
#include <vector>

#include "Lattice/Engine/NeighborSearch/SpatialGrid.h"
#include "Lattice/Engine/math/DynamicSoA.h"
#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "Lattice/Engine/physics/Atom/AtomSort.h"

class AtomStorage {
public:
    using AtomId = uint32_t;
    static constexpr AtomId InvalidAtomId = std::numeric_limits<AtomId>::max();

    enum class ColumnId : uint32_t {
        X,
        Y,
        Z,
        Vx,
        Vy,
        Vz,
        Fx,
        Fy,
        Fz,
        Pfx,
        Pfy,
        Pfz,
        Energy,
        InvMass,
        Charge,
        Type,
        Valence,
        Id
    };

    struct X { using ValueType = float; static constexpr ColumnId id = ColumnId::X; };
    struct Y { using ValueType = float; static constexpr ColumnId id = ColumnId::Y; };
    struct Z { using ValueType = float; static constexpr ColumnId id = ColumnId::Z; };
    struct Vx { using ValueType = float; static constexpr ColumnId id = ColumnId::Vx; };
    struct Vy { using ValueType = float; static constexpr ColumnId id = ColumnId::Vy; };
    struct Vz { using ValueType = float; static constexpr ColumnId id = ColumnId::Vz; };
    struct Fx { using ValueType = float; static constexpr ColumnId id = ColumnId::Fx; };
    struct Fy { using ValueType = float; static constexpr ColumnId id = ColumnId::Fy; };
    struct Fz { using ValueType = float; static constexpr ColumnId id = ColumnId::Fz; };
    struct Pfx { using ValueType = float; static constexpr ColumnId id = ColumnId::Pfx; };
    struct Pfy { using ValueType = float; static constexpr ColumnId id = ColumnId::Pfy; };
    struct Pfz { using ValueType = float; static constexpr ColumnId id = ColumnId::Pfz; };
    struct Energy { using ValueType = float; static constexpr ColumnId id = ColumnId::Energy; };
    struct InvMass { using ValueType = float; static constexpr ColumnId id = ColumnId::InvMass; };
    struct Charge { using ValueType = float; static constexpr ColumnId id = ColumnId::Charge; };
    struct Type { using ValueType = AtomData::Type; static constexpr ColumnId id = ColumnId::Type; };
    struct Valence { using ValueType = uint8_t; static constexpr ColumnId id = ColumnId::Valence; };
    struct Id { using ValueType = AtomId; static constexpr ColumnId id = ColumnId::Id; };

private:
    static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

    AtomSort sort_;
    DynamicSoA soa_;
    size_t mobileCount_ = 0;
    std::vector<size_t> atomIdToIndex_;
    AtomId nextAtomId_ = 0;

    template<typename T>
    T* col(ColumnId c) {
        return soa_.column<T>(static_cast<uint32_t>(c));
    }

    template<typename T>
    const T* col(ColumnId c) const {
        return soa_.column<T>(static_cast<uint32_t>(c));
    }

    template<typename T>
    std::span<T> colSpan(ColumnId c) {
        return soa_.span<T>(static_cast<uint32_t>(c));
    }

    template<typename T>
    std::span<const T> colSpan(ColumnId c) const {
        return soa_.span<T>(static_cast<uint32_t>(c));
    }

    template<typename T>
    T& at(ColumnId c, size_t i) {
        return col<T>(c)[i];
    }

    template<typename T>
    const T& at(ColumnId c, size_t i) const {
        return col<T>(c)[i];
    }

    void setVec3(ColumnId x, ColumnId y, ColumnId z, size_t i, const glm::vec3& v) {
        at<float>(x, i) = v.x;
        at<float>(y, i) = v.y;
        at<float>(z, i) = v.z;
    }

public:
    AtomStorage() {
        soa_.add<float>(static_cast<uint32_t>(X::id));
        soa_.add<float>(static_cast<uint32_t>(Y::id));
        soa_.add<float>(static_cast<uint32_t>(Z::id));
        soa_.add<float>(static_cast<uint32_t>(Vx::id));
        soa_.add<float>(static_cast<uint32_t>(Vy::id));
        soa_.add<float>(static_cast<uint32_t>(Vz::id));
        soa_.add<float>(static_cast<uint32_t>(Fx::id));
        soa_.add<float>(static_cast<uint32_t>(Fy::id));
        soa_.add<float>(static_cast<uint32_t>(Fz::id));
        soa_.add<float>(static_cast<uint32_t>(Pfx::id));
        soa_.add<float>(static_cast<uint32_t>(Pfy::id));
        soa_.add<float>(static_cast<uint32_t>(Pfz::id));
        soa_.add<float>(static_cast<uint32_t>(Energy::id));
        soa_.add<float>(static_cast<uint32_t>(InvMass::id));
        soa_.add<float>(static_cast<uint32_t>(Charge::id));
        soa_.add<AtomData::Type>(static_cast<uint32_t>(Type::id));
        soa_.add<uint8_t>(static_cast<uint32_t>(Valence::id));
        soa_.add<AtomId>(static_cast<uint32_t>(Id::id));
    }

    AtomStorage(const AtomStorage&) = delete;
    AtomStorage& operator=(const AtomStorage&) = delete;
    AtomStorage(AtomStorage&&) noexcept = default;
    AtomStorage& operator=(AtomStorage&&) noexcept = default;

    void clear() {
        soa_.clear();
        mobileCount_ = 0;
        nextAtomId_ = 0;
        atomIdToIndex_.clear();
        atomIdToIndex_.shrink_to_fit();
    }

    void reserve(size_t count) {
        soa_.reserve(count);
        atomIdToIndex_.reserve(count);
    }

    [[nodiscard]] AtomId addAtom(const glm::vec3& coords, const glm::vec3& speed, AtomData::Type typeValue, bool fixed = false) {
        soa_.resize(size() + 1);
        const size_t i = size() - 1;

        setPos(i, coords);
        setVel(i, speed);
        setForce(i, glm::vec3(0.0f));
        setPrevForce(i, glm::vec3(0.0f));
        at<float>(Energy::id, i) = 0.0f;

        const auto& props = AtomData::getProps(typeValue);
        at<float>(InvMass::id, i) = 1.0f / props.mass;
        at<float>(Charge::id, i) = props.defaultCharge;
        at<AtomData::Type>(Type::id, i) = typeValue;
        at<uint8_t>(Valence::id, i) = props.maxValence;

        const AtomId id = nextAtomId_++;
        setAtomId(i, id);

        if (!fixed) {
            swapAtoms(i, mobileCount_);
            ++mobileCount_;
        }
        return id;
    }

    void removeAtom(size_t index) {
        if (index >= size()) {
            return;
        }

        const size_t last = size() - 1;
        if (index < mobileCount_) {
            swapAtoms(index, mobileCount_ - 1);
            --mobileCount_;
            swapAtoms(mobileCount_, last);
        } else if (index != last) {
            swapAtoms(index, last);
        }

        atomIdToIndex_[atomId(last)] = InvalidIndex;
        soa_.resize(last);
    }

    void swapAtoms(size_t a, size_t b) {
        if (a >= size() || b >= size() || a == b) {
            return;
        }
        soa_.swapRows(a, b);
        atomIdToIndex_[atomId(a)] = a;
        atomIdToIndex_[atomId(b)] = b;
    }

    void swapPrevCurrentForces() {
        for (size_t i = 0; i < size(); ++i) {
            std::swap(at<float>(Fx::id, i), at<float>(Pfx::id, i));
            std::swap(at<float>(Fy::id, i), at<float>(Pfy::id, i));
            std::swap(at<float>(Fz::id, i), at<float>(Pfz::id, i));
        }
    }

    size_t size() const { return soa_.size(); }
    size_t mobileCount() const { return mobileCount_; }
    bool empty() const { return size() == 0; }
    bool isAtomFixed(size_t i) const { return i >= mobileCount_; }

    size_t memoryBytes() const { return soa_.storageBytes() + atomIdToIndex_.capacity() * sizeof(size_t); }

    float* x() { return col<float>(X::id); }
    const float* x() const { return col<float>(X::id); }
    std::span<float> xSpan() { return colSpan<float>(X::id); }
    std::span<const float> xSpan() const { return colSpan<float>(X::id); }

    float* y() { return col<float>(Y::id); }
    const float* y() const { return col<float>(Y::id); }
    std::span<float> ySpan() { return colSpan<float>(Y::id); }
    std::span<const float> ySpan() const { return colSpan<float>(Y::id); }

    float* z() { return col<float>(Z::id); }
    const float* z() const { return col<float>(Z::id); }
    std::span<float> zSpan() { return colSpan<float>(Z::id); }
    std::span<const float> zSpan() const { return colSpan<float>(Z::id); }

    float* vx() { return col<float>(Vx::id); }
    const float* vx() const { return col<float>(Vx::id); }
    std::span<float> vxSpan() { return colSpan<float>(Vx::id); }
    std::span<const float> vxSpan() const { return colSpan<float>(Vx::id); }

    float* vy() { return col<float>(Vy::id); }
    const float* vy() const { return col<float>(Vy::id); }
    std::span<float> vySpan() { return colSpan<float>(Vy::id); }
    std::span<const float> vySpan() const { return colSpan<float>(Vy::id); }

    float* vz() { return col<float>(Vz::id); }
    const float* vz() const { return col<float>(Vz::id); }
    std::span<float> vzSpan() { return colSpan<float>(Vz::id); }
    std::span<const float> vzSpan() const { return colSpan<float>(Vz::id); }

    float* fx() { return col<float>(Fx::id); }
    const float* fx() const { return col<float>(Fx::id); }
    std::span<float> fxSpan() { return colSpan<float>(Fx::id); }
    std::span<const float> fxSpan() const { return colSpan<float>(Fx::id); }

    float* fy() { return col<float>(Fy::id); }
    const float* fy() const { return col<float>(Fy::id); }
    std::span<float> fySpan() { return colSpan<float>(Fy::id); }
    std::span<const float> fySpan() const { return colSpan<float>(Fy::id); }

    float* fz() { return col<float>(Fz::id); }
    const float* fz() const { return col<float>(Fz::id); }
    std::span<float> fzSpan() { return colSpan<float>(Fz::id); }
    std::span<const float> fzSpan() const { return colSpan<float>(Fz::id); }

    float* pfx() { return col<float>(Pfx::id); }
    const float* pfx() const { return col<float>(Pfx::id); }
    std::span<float> pfxSpan() { return colSpan<float>(Pfx::id); }
    std::span<const float> pfxSpan() const { return colSpan<float>(Pfx::id); }

    float* pfy() { return col<float>(Pfy::id); }
    const float* pfy() const { return col<float>(Pfy::id); }
    std::span<float> pfySpan() { return colSpan<float>(Pfy::id); }
    std::span<const float> pfySpan() const { return colSpan<float>(Pfy::id); }

    float* pfz() { return col<float>(Pfz::id); }
    const float* pfz() const { return col<float>(Pfz::id); }
    std::span<float> pfzSpan() { return colSpan<float>(Pfz::id); }
    std::span<const float> pfzSpan() const { return colSpan<float>(Pfz::id); }

    float* charge() { return col<float>(Charge::id); }
    const float* charge() const { return col<float>(Charge::id); }
    std::span<float> chargeSpan() { return colSpan<float>(Charge::id); }
    std::span<const float> chargeSpan() const { return colSpan<float>(Charge::id); }

    float* energy() { return col<float>(Energy::id); }
    const float* energy() const { return col<float>(Energy::id); }
    std::span<float> energySpan() { return colSpan<float>(Energy::id); }
    std::span<const float> energySpan() const { return colSpan<float>(Energy::id); }

    float* invMass() { return col<float>(InvMass::id); }
    const float* invMass() const { return col<float>(InvMass::id); }
    std::span<float> invMassSpan() { return colSpan<float>(InvMass::id); }
    std::span<const float> invMassSpan() const { return colSpan<float>(InvMass::id); }

    AtomData::Type* type() { return col<AtomData::Type>(Type::id); }
    const AtomData::Type* type() const { return col<AtomData::Type>(Type::id); }
    std::span<AtomData::Type> typeSpan() { return colSpan<AtomData::Type>(Type::id); }
    std::span<const AtomData::Type> typeSpan() const { return colSpan<AtomData::Type>(Type::id); }

    uint8_t* valence() { return col<uint8_t>(Valence::id); }
    const uint8_t* valence() const { return col<uint8_t>(Valence::id); }
    std::span<uint8_t> valenceSpan() { return colSpan<uint8_t>(Valence::id); }
    std::span<const uint8_t> valenceSpan() const { return colSpan<uint8_t>(Valence::id); }

    std::span<const AtomId> atomIdDataSpan() const { return colSpan<AtomId>(ColumnId::Id); }
    std::span<const float> floatDataSpan() const { return {}; }

    glm::vec3 pos(size_t i) const { return {at<float>(X::id, i), at<float>(Y::id, i), at<float>(Z::id, i)}; }
    glm::vec3 vel(size_t i) const { return {at<float>(Vx::id, i), at<float>(Vy::id, i), at<float>(Vz::id, i)}; }
    glm::vec3 force(size_t i) const { return {at<float>(Fx::id, i), at<float>(Fy::id, i), at<float>(Fz::id, i)}; }
    glm::vec3 prevForce(size_t i) const { return {at<float>(Pfx::id, i), at<float>(Pfy::id, i), at<float>(Pfz::id, i)}; }

    void setPos(size_t i, const glm::vec3& v) { setVec3(X::id, Y::id, Z::id, i, v); }
    void setVel(size_t i, const glm::vec3& v) { setVec3(Vx::id, Vy::id, Vz::id, i, v); }
    void setForce(size_t i, const glm::vec3& v) { setVec3(Fx::id, Fy::id, Fz::id, i, v); }
    void setPrevForce(size_t i, const glm::vec3& v) { setVec3(Pfx::id, Pfy::id, Pfz::id, i, v); }

    void setAtomId(size_t index, AtomId id) noexcept {
        at<AtomId>(Id::id, index) = id;
        if (static_cast<size_t>(id) >= atomIdToIndex_.size()) {
            atomIdToIndex_.resize(static_cast<size_t>(id) + 1, InvalidIndex);
        }
        atomIdToIndex_[id] = index;
    }

    [[nodiscard]] AtomId atomId(size_t index) const noexcept { return index < size() ? at<AtomId>(Id::id, index) : InvalidAtomId; }
    [[nodiscard]] size_t indexOf(AtomId id) const noexcept { return id < atomIdToIndex_.size() ? atomIdToIndex_[id] : InvalidIndex; }
    [[nodiscard]] bool containsAtomId(AtomId id) const noexcept { return indexOf(id) != InvalidIndex; }

    void setFixed(size_t i, bool fixed) {
        if (fixed) {
            if (i >= mobileCount_) {
                return;
            }
            --mobileCount_;
            swapAtoms(i, mobileCount_);
        } else {
            if (i < mobileCount_) {
                return;
            }
            swapAtoms(i, mobileCount_);
            ++mobileCount_;
        }
    }

    void sort(SpatialGrid& grid) { sort_.mortonOrder(*this, grid); }
    [[nodiscard]] const std::vector<uint32_t>& lastSortOldToNew() const noexcept { return sort_.oldToNew(); }
};
