#pragma once

#include <cstddef>
#include <vector>

#include "Atom.h"

class AtomStorage {
    private:
        std::vector<float> x, y, z;       // Координаты по трем осям
        std::vector<float> vx, vy, vz;    // Скорости по трем осям
        std::vector<float> fx, fy, fz;    // Силы по трем осям
        std::vector<float> pfx, pfy, pfz; // Предыдущие силы по трем осям

        std::vector<Atom::Type> atomType;
        std::vector<float> potential_energy;
        std::vector<int> valence;

        std::vector<bool> isFixed;

    public:
        std::size_t size() const { return x.size(); }
        bool empty() const { return x.empty(); }

        void clear() {
            x.clear();
            y.clear();
            z.clear();

            vx.clear();
            vy.clear();
            vz.clear();

            fx.clear();
            fy.clear();
            fz.clear();

            pfx.clear();
            pfy.clear();
            pfz.clear();

            atomType.clear();
            potential_energy.clear();
            valence.clear();
            isFixed.clear();
        }

        void reserve(std::size_t count) {
            x.reserve(count);
            y.reserve(count);
            z.reserve(count);

            vx.reserve(count);
            vy.reserve(count);
            vz.reserve(count);

            fx.reserve(count);
            fy.reserve(count);
            fz.reserve(count);

            pfx.reserve(count);
            pfy.reserve(count);
            pfz.reserve(count);

            atomType.reserve(count);
            potential_energy.reserve(count);
            valence.reserve(count);
            isFixed.reserve(count);
        }

        void addAtom(const Vec3D& coords, const Vec3D& speed, Atom::Type type, bool fixed = false) {
            x.push_back(static_cast<float>(coords.x));
            y.push_back(static_cast<float>(coords.y));
            z.push_back(static_cast<float>(coords.z));

            vx.push_back(static_cast<float>(speed.x));
            vy.push_back(static_cast<float>(speed.y));
            vz.push_back(static_cast<float>(speed.z));

            fx.push_back(0.0f);
            fy.push_back(0.0f);
            fz.push_back(0.0f);

            pfx.push_back(0.0f);
            pfy.push_back(0.0f);
            pfz.push_back(0.0f);

            atomType.push_back(type);
            potential_energy.push_back(0.0f);
            valence.push_back(0);
            isFixed.push_back(fixed);
        }

        void removeAtom(std::size_t index) {
            if (index >= size()) return;
            std::size_t lastIndex = size() - 1;
            if (index != lastIndex) {
                // Перемещаем последний элемент на место удаляемого
                x[index] = x[lastIndex];
                y[index] = y[lastIndex];
                z[index] = z[lastIndex];

                vx[index] = vx[lastIndex];
                vy[index] = vy[lastIndex];
                vz[index] = vz[lastIndex];

                fx[index] = fx[lastIndex];
                fy[index] = fy[lastIndex];
                fz[index] = fz[lastIndex];

                pfx[index] = pfx[lastIndex];
                pfy[index] = pfy[lastIndex];
                pfz[index] = pfz[lastIndex];

                atomType[index] = atomType[lastIndex];
                potential_energy[index] = potential_energy[lastIndex];
                valence[index] = valence[lastIndex];
                isFixed[index] = isFixed[lastIndex];
            }

            // Удаляем последний элемент
            x.pop_back();
            y.pop_back();
            z.pop_back();

            vx.pop_back();
            vy.pop_back();
            vz.pop_back();

            fx.pop_back();
            fy.pop_back();
            fz.pop_back();

            pfx.pop_back();
            pfy.pop_back();
            pfz.pop_back();

            atomType.pop_back();
            potential_energy.pop_back();
            valence.pop_back();
            isFixed.pop_back();
        }

        float& posX(std::size_t i) { return x[i]; }
        float& posY(std::size_t i) { return y[i]; }
        float& posZ(std::size_t i) { return z[i]; }
        const float& posX(std::size_t i) const { return x[i]; }
        const float& posY(std::size_t i) const { return y[i]; }
        const float& posZ(std::size_t i) const { return z[i]; }

        float& velX(std::size_t i) { return vx[i]; }
        float& velY(std::size_t i) { return vy[i]; }
        float& velZ(std::size_t i) { return vz[i]; }
        const float& velX(std::size_t i) const { return vx[i]; }
        const float& velY(std::size_t i) const { return vy[i]; }
        const float& velZ(std::size_t i) const { return vz[i]; }

        float& forceX(std::size_t i) { return fx[i]; }
        float& forceY(std::size_t i) { return fy[i]; }
        float& forceZ(std::size_t i) { return fz[i]; }
        const float& forceX(std::size_t i) const { return fx[i]; }
        const float& forceY(std::size_t i) const { return fy[i]; }
        const float& forceZ(std::size_t i) const { return fz[i]; }

        float& prevForceX(std::size_t i) { return pfx[i]; }
        float& prevForceY(std::size_t i) { return pfy[i]; }
        float& prevForceZ(std::size_t i) { return pfz[i]; }
        const float& prevForceX(std::size_t i) const { return pfx[i]; }
        const float& prevForceY(std::size_t i) const { return pfy[i]; }
        const float& prevForceZ(std::size_t i) const { return pfz[i]; }

        Atom::Type& type(std::size_t i) { return atomType[i]; }
        const Atom::Type& type(std::size_t i) const { return atomType[i]; }

        float& energy(std::size_t i) { return potential_energy[i]; }
        const float& energy(std::size_t i) const { return potential_energy[i]; }

        int& valenceCount(std::size_t i) { return valence[i]; }
        const int& valenceCount(std::size_t i) const { return valence[i]; }
        
        bool isAtomFixed(std::size_t i) { return isFixed[i]; }
        void setFixed(std::size_t i, bool fixed) { isFixed[i] = fixed; }

        Vec3D pos(std::size_t i) const {
            return Vec3D(x[i], y[i], z[i]);
        }

        Vec3D vel(std::size_t i) const {
            return Vec3D(vx[i], vy[i], vz[i]);
        }

        Vec3D force(std::size_t i) const {
            return Vec3D(fx[i], fy[i], fz[i]);
        }

        Vec3D prevForce(std::size_t i) const {
            return Vec3D(pfx[i], pfy[i], pfz[i]);
        }

        void setPos(std::size_t i, const Vec3D& coords) {
            x[i] = static_cast<float>(coords.x);
            y[i] = static_cast<float>(coords.y);
            z[i] = static_cast<float>(coords.z);
        }

        void setVel(std::size_t i, const Vec3D& speed) {
            vx[i] = static_cast<float>(speed.x);
            vy[i] = static_cast<float>(speed.y);
            vz[i] = static_cast<float>(speed.z);
        }

        void setForce(std::size_t i, const Vec3D& force) {
            fx[i] = static_cast<float>(force.x);
            fy[i] = static_cast<float>(force.y);
            fz[i] = static_cast<float>(force.z);
        }

        void setPrevForce(std::size_t i, const Vec3D& prev_force) {
            pfx[i] = static_cast<float>(prev_force.x);
            pfy[i] = static_cast<float>(prev_force.y);
            pfz[i] = static_cast<float>(prev_force.z);
        }
};
