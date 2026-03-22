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
};
