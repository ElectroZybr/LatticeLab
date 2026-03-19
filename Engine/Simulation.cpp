#include <fstream>
#include <cmath>
#include <iostream>
#include <numbers>

#include "imgui-SFML.h"

#include "Simulation.h"
#include "GUI/interface/interface.h"
#include "Tools.h"

Simulation::Simulation(sf::RenderWindow& w, SimBox& box)
    : window(w), gameView(window.getDefaultView()), uiView(window.getDefaultView()), sim_box(box)
{
    // sim_box = box;
    Interface::init(window);
    Tools::init(&window, &gameView, render, &sim_box.grid, &sim_box);
    Atom::setGrid(&sim_box.grid);

    // резервируем место под создание атомов
    atoms.reserve(50000);

    // setSizeBox(sizeX, sizeY);
}

void Simulation::setRenderer(IRenderer* r) {
    render = r;
    sim_box.setRenderer(render);
    Tools::init(&window, &gameView, render, &sim_box.grid, &sim_box);
}

void Simulation::update(float dt) {
    if (!Interface::getPause()) {
        for (Atom& atom : atoms)
            atom.PredictPosition(dt);
        for (Atom& atom : atoms)
            atom.ComputeForces(sim_box, dt);
        for (auto it = Bond::bonds_list.begin(); it != Bond::bonds_list.end(); ) {
            if (it->shouldBreak()) {
                it->detach();
                it = Bond::bonds_list.erase(it);
            } else {
                ++it;
            }
        }
        for (Bond& bond : Bond::bonds_list)
            bond.forceBond(dt);
        for (Atom& atom : atoms)
            atom.CorrectVelosity(dt);
        sim_step++;
    }
}

void Simulation::renderShot(float deltaTime) {
    render->drawShot(atoms, sim_box, deltaTime);
}

void Simulation::pollEvents() {
    while (const std::optional<sf::Event> optEvent = window.pollEvent()) {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
        const sf::Event& event = *optEvent;
        ImGui::SFML::ProcessEvent(window, event);

        if (event.is<sf::Event::Closed>())
            window.close();

        if (const auto* e = event.getIf<sf::Event::Resized>()) {
            uiView.setSize(sf::Vector2f(e->size));
            uiView.setCenter(sf::Vector2f(e->size) / 2.f);
            // ImGui::GetIO().DisplaySize = ImVec2(e->size.x, e->size.y);
        }

        Interface::CheckEvent(event);

        if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) { 
            if (e->button == sf::Mouse::Button::Left) {
                float zoom = render->camera.getZoom();

                // создание атома
                if (!Interface::cursorHovered && Interface::getSelectedAtom() != -1) {
                    const Vec2D world = Tools::screenToWorld(mouse_pos, zoom);
                    const Vec2D local(world.x - sim_box.start.x, world.y - sim_box.start.y);

                    const double atomRadius = Atom::getProps(Interface::getSelectedAtom()).radius;
                    const Vec3D spawnPos = Vec3D(local - atomRadius / 2.0);

                    bool hasNearAtom = false;
                    for (Atom& atom : atoms) {
                        if ((atom.coords - spawnPos).abs() <= atom.getProps().radius + atomRadius) {
                            hasNearAtom = true;
                            break;
                        }
                    }

                    if (!hasNearAtom) {
                        atoms.emplace_back(spawnPos,
                                        Vec3D(((double)std::rand() / RAND_MAX-0.5)*5, ((double)std::rand() / RAND_MAX-0.5)*5, 0), Interface::getSelectedAtom());
    
                        Atom& atom = atoms.back();
    
                        std::cout << "<Create atom> X: " << world.x <<  " Y: " << world.y << 
                            " | Type: " << Interface::getSelectedAtom() << 
                            " | Adress: " << &atom << 
                            " | Speed: " << atom.speed.x << " " << atom.speed.y << std::endl;
                    }
                }
                else {
                    Vec3D world = Tools::screenToWorld(mouse_pos, zoom);
                    Vec3D local = world - sim_box.start;
                    std::unordered_set<Atom*>* block = sim_box.grid.at(
                        sim_box.grid.worldToCellX(local.x - 0.5),
                        sim_box.grid.worldToCellY(local.y - 0.5),
                        sim_box.grid.worldToCellY(local.z - 0.5)
                    );

                    if (block != nullptr && !block->empty() && !selectionFrameMoveFlag) {
                        selectedMoveAtom = *block->begin();
                        atomMoveFlag = true; 
                    } else if (!Interface::cursorHovered && !selectionFrameMoveFlag) {
                        selectionFrameMoveFlag = true;
                        start_mouse_pos = sf::Mouse::getPosition(window);
                        Tools::selectionFrame(start_mouse_pos, sf::Mouse::getPosition(window), atoms);
                        render->showSelectionFrame(true);
                    }
                } 
            } 
        }
        
        if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (e->button == sf::Mouse::Button::Left) {
                atomMoveFlag = false;
                selectionFrameMoveFlag = false;
                render->showSelectionFrame(false);
                Interface::drawToolTrip = false;
            }
        } 
        render->camera.handleEvent(event, window);
    }
}

void Simulation::event() {
    Interface::setAverageEnergy(AverageEnegry());
    Interface::setSimStep(sim_step);
    Interface::Update();
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

    if (selectionFrameMoveFlag) {
        Tools::selectionFrame(start_mouse_pos, mouse_pos, atoms);
    }

    // Передвижение атома мышкой
    if (atomMoveFlag) {
        float zoom = render->camera.getZoom();
        Vec2D world = Tools::screenToBox(mouse_pos, zoom);
        Vec2D delta = Vec2D(selectedMoveAtom->coords.x, selectedMoveAtom->coords.y) - world;
        Vec3D force = delta * 30;
        for (Atom* atom : Tools::selected_atom_batch) {
            atom->force -= force;
        }
    }
}

void Simulation::setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize) {
    if (sim_box.setSizeBox(newStart, newEnd, cellSize)) {
        Atom::setGrid(&sim_box.grid);

        for (Atom& atom : atoms) {
            const int cellX = sim_box.grid.worldToCellX(atom.coords.x);
            const int cellY = sim_box.grid.worldToCellY(atom.coords.y);
            const int cellZ = sim_box.grid.worldToCellY(atom.coords.z);
            sim_box.grid.insert(cellX, cellY, cellZ, &atom);
        }
    }
}

void Simulation::createRandomAtoms(int type, int quantity) {
    const double z_mid = (sim_box.end.z - sim_box.start.z) * 0.5;
    for (int i = 0; i < quantity; ++i) {
        for (int j = 0; j < 10; ++j) {
            double r_x = std::rand() % int(sim_box.end.x-sim_box.start.x-4);
            double r_y = std::rand() % int(sim_box.end.y-sim_box.start.y-4);
            Vec3D coords(r_x+2, r_y+2, z_mid);
            if (!checkNeighbor(coords, 4)) {
                createAtom(coords, randomUnitVector3D(5), type);
                break;
            }
        }
    }
}

bool Simulation::checkNeighbor(Vec3D coords, float delta) {
    int curr_x = sim_box.grid.worldToCellX(coords.x);
    int curr_y = sim_box.grid.worldToCellY(coords.y);
    int curr_z = sim_box.grid.worldToCellY(coords.z);
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                if (auto cell = sim_box.grid.at(curr_x - i, curr_y - j, curr_z - k)) {
                    for (Atom* other : *cell) {
                        if ((coords - other->coords).sqrAbs() < delta*delta) return true;
                    }
                }
            }
        }
    }
    return false;
}

Atom* Simulation::createAtom(Vec3D start_coords, Vec3D start_speed, int type, bool fixed) {
    atoms.emplace_back(start_coords, start_speed, type, fixed);
    return &atoms.back();
}

void Simulation::addBond(Atom* a1, Atom* a2) {
    // FIXME Здесь возможен баг, что если вектор атомов переаллоцирован, то указатели станут не валидными
    Bond::CreateBond(a1, a2);
}

double Simulation::AverageEnegry() const {
    if (atoms.empty()) {
        return 0.0;
    }

    double kineticEnergy = 0.0;
    for (const Atom& atom : atoms) {
        kineticEnergy += atom.kineticEnergy();
    }

    return kineticEnergy / static_cast<double>(atoms.size());
}

void Simulation::logAtomPos() const {
    int i = 0;
    for (const Atom& atom : atoms) {
        std::cout << "<Pos> Atom (" << i
                  << ") X " << atom.coords.x
                  << " | Y " << atom.coords.y
                  << " | Z " << atom.coords.z
                  << std::endl;
        i++;
    }
}

void Simulation::logBondList() const {
    for (const Atom& atom : atoms) {
        if (atom.bonds.size() > 0) {
            std::cout << atom.bonds.size() << std::endl;
        }
    }
}

void Simulation::logMousePos() const {
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
    Vec2D world_pos = Tools::screenToWorld(mouse_pos, render->camera.getZoom());
    Vec2D local_pos(world_pos.x - sim_box.start.x, world_pos.y - sim_box.start.y);
    std::cout << "<Mouse pos>"
              << " Screen: "
              << "X " << mouse_pos.x
              << " Y " << mouse_pos.y
              << " | World: "
              << "X " << world_pos.x
              << " Y " << world_pos.y
              << " | Local: "
              << "X " << local_pos.x
              << " Y " << local_pos.y
              << std::endl;
}

Vec2D randomUnitVector2D() {
    double angle = (double)std::rand() / RAND_MAX * 2.0 * std::numbers::pi;
    return Vec2D(std::cos(angle), std::sin(angle));  // x = cos(θ), y = sin(θ)
}

Vec3D randomUnitVector3D(double amplitude) {
    double u = (double)std::rand() / RAND_MAX;       // in [0,1]
    double v = (double)std::rand() / RAND_MAX;       // in [0,1]

    double theta = 2.0 * std::numbers::pi * u;       // азимут (0..2π)
    double z = 2.0 * v - 1.0;                        // равномерно по [-1,1]
    double r = std::sqrt(1.0 - z * z);             // радиус проекции на xy

    double x = r * std::cos(theta);
    double y = r * std::sin(theta);

    return Vec3D(x, y, z) * amplitude;  // масштабируем вектор до нужной амплитуды
}

double randomInRange(int range) {
    return std::rand() % range + 1;
}

void Simulation::save(std::string_view path) const
{
    std::ofstream file(path.data());
    if (!file.is_open()) return;

    file << "box "
         << sim_box.start.x << " " << sim_box.start.y << " " << sim_box.start.z << " "
         << sim_box.end.x   << " " << sim_box.end.y   << " " << sim_box.end.z   << "\n";

    file << "step " << sim_step << "\n";

    file << "camera "
         << render->camera.getPosition().x << " "
         << render->camera.getPosition().y << " "
         << render->camera.getZoom()       << "\n";

    for (const Atom& atom : atoms) {
        file << "atom "
             << atom.coords.x << " " << atom.coords.y << " " << atom.coords.z << " "
             << atom.speed.x  << " " << atom.speed.y  << " " << atom.speed.z  << " "
             << atom.type     << " "
             << atom.a        << " "
             << atom.eps      << " "
             << atom.isFixed  << "\n";
    }
}

void Simulation::load(std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) return;

    clear();

    // временный буфер чтобы не было реаллокаций
    struct AtomData {
        Vec3D coords, speed;
        int type;
        float a, eps;
        bool fixed;
    };
    std::vector<AtomData> buffer;

    Vec3D boxStart, boxEnd;
    int cellSize = -1;

    std::string tag;
    while (file >> tag) {
        if (tag == "box") {
            file >> boxStart.x >> boxStart.y >> boxStart.z
                 >> boxEnd.x   >> boxEnd.y   >> boxEnd.z;
        }
        else if (tag == "step") {
            file >> sim_step;
        }
        else if (tag == "camera") {
            double cx, cy, zoom;
            file >> cx >> cy >> zoom;
            render->camera.setPosition(cx, cy);
            render->camera.setZoom(zoom);
        }
        else if (tag == "atom") {
            AtomData d;
            file >> d.coords.x >> d.coords.y >> d.coords.z
                 >> d.speed.x  >> d.speed.y  >> d.speed.z
                 >> d.type >> d.a >> d.eps >> d.fixed;
            buffer.emplace_back(d);
        }
    }

    setSizeBox(boxStart, boxEnd, cellSize);

    atoms.reserve(buffer.size());
    for (const AtomData& d : buffer) {
        Atom* atom = createAtom(d.coords, d.speed, d.type, d.fixed);
        atom->a   = d.a;
        atom->eps = d.eps;
    }
}

void Simulation::clear() {
    atoms.clear();
    Bond::bonds_list.clear();
    sim_step = 0;
}
