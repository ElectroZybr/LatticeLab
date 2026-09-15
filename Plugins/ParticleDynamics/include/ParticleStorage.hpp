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

struct InvMass { using type = float; };


namespace ParticleDynamics {
    
class ParticleStorage : public StdData::SoA {
public:
    explicit ParticleStorage(Lattice::Node& branch) {

        addCol<Pos::X>();
        addCol<Pos::Y>();
        addCol<Pos::Z>();

        addCol<Vel::X>();
        addCol<Vel::Y>();
        addCol<Vel::Z>();

        addCol<Force::X>();
        addCol<Force::Y>();
        addCol<Force::Z>();

        addCol<InvMass>();
    }

    size_t add(const glm::vec3& pos, const glm::vec3& vel, bool fixed = false) {
        resize(size() + 1);
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

        resize(last);
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

    size_t mobileCount() const { return mobileCount_; }
    bool empty() const { return size() == 0; }
    bool isFixed(size_t i) const { return i >= mobileCount_; }

private:
    size_t mobileCount_ = 0;

    void swap(size_t a, size_t b) {
        if (a >= size() || b >= size() || a == b) {
            return;
        }
        swapRows(a, b);
    }

};

}