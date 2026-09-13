#pragma once

#include <glm/vec3.hpp>

#include <Lattice/Kernel/Node.hpp>
#include "StdData/include/SoA.hpp"


struct Pos {
    struct X { using type = float; };
    struct Y { using type = float; };
    struct Z { using type = float; };
};

struct Vel {
    struct X { using type = float; };
    struct Y { using type = float; };
    struct Z { using type = float; };
};

struct Force {
    struct X { using type = float; };
    struct Y { using type = float; };
    struct Z { using type = float; };
};

struct InvMass {using type = float;};


namespace ParticleDynamics {
    
class ParticleStorage {
public:
    explicit ParticleStorage(Lattice::Node& branch) {
        branch.add<StdData::SoA>();
        soa_ = branch.require<StdData::SoA>();

        soa_->addCol<Pos::X>();
        soa_->addCol<Pos::Y>();
        soa_->addCol<Pos::Z>();

        soa_->addCol<Vel::X>();
        soa_->addCol<Vel::Y>();
        soa_->addCol<Vel::Z>();

        soa_->addCol<Force::X>();
        soa_->addCol<Force::Y>();
        soa_->addCol<Force::Z>();

        soa_->addCol<InvMass>();
    }

    template<class Tag>
    typename Tag::type* addCol() noexcept {
        return soa_->addCol<Tag>();
    }

    template<class Tag>
    void removeCol() {
        soa_->remove<Tag>();
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type* getCol() noexcept {
        return soa_->get<Tag>();
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type* getCol() const noexcept {
        return soa_->get<Tag>();
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type* requireCol() {
        return soa_->require<Tag>();
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type* requireCol() const {
        return soa_->require<Tag>();
    }

    template<class Tag>
    [[nodiscard]] std::span<typename Tag::type> spanCol() noexcept {
        return soa_->span<Tag>();
    }

    template<class Tag>
    [[nodiscard]] std::span<const typename Tag::type> spanCol() const noexcept {
        return soa_->span<Tag>();
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type& at(size_t index) noexcept {
        return soa_->at<Tag>(index);
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type& at(size_t index) const noexcept {
        return soa_->at<Tag>(index);
    }

    size_t add(const glm::vec3& pos, const glm::vec3& vel, bool fixed = false) {
        soa_->resize(size() + 1);
        const size_t i = size() - 1;

        set<Pos>(pos, i);
        set<Vel>(vel, i);
        set<Force>(glm::vec3(0.0f), i);

        size_t dst = i;
        if (!fixed) {
            dst = mobileCount_;
            swap(i, dst);
            ++mobileCount_;
        }
        return dst;
    }

    void remove(size_t index) {
        if (index >= size()) {
            return;
        }

        const size_t last = size() - 1;
        if (index < mobileCount_) {
            swap(index, mobileCount_ - 1);
            --mobileCount_;
            swap(mobileCount_, last);
        } else if (index != last) {
            swap(index, last);
        }

        soa_->resize(last);
    }

    void setFixed(size_t i, bool fixed) {
        if (fixed) {
            if (i >= mobileCount_) {
                return;
            }
            --mobileCount_;
            swap(i, mobileCount_);
        } else {
            if (i < mobileCount_) {
                return;
            }
            swap(i, mobileCount_);
            ++mobileCount_;
        }
    }

    template<class V>
    void set(const glm::vec3& value, size_t i) noexcept {
        at<typename V::X>(i) = value.x;
        at<typename V::Y>(i) = value.y;
        at<typename V::Z>(i) = value.z;
    }

    template<class V>
    [[nodiscard]] glm::vec3 get(size_t i) const noexcept {
        return {
            at<typename V::X>(i),
            at<typename V::Y>(i),
            at<typename V::Z>(i)
        };
    }

    size_t size() const { return soa_->size(); }
    size_t mobileCount() const { return mobileCount_; }
    bool empty() const { return size() == 0; }
    bool isAtomFixed(size_t i) const { return i >= mobileCount_; }
    size_t memoryBytes() const { return soa_->storageBytes(); }

private:
    Ref<StdData::SoA> soa_;
    size_t mobileCount_ = 0;

    void swap(size_t a, size_t b) {
        if (a >= size() || b >= size() || a == b) {
            return;
        }
        soa_->swapRows(a, b);
    }

};

}