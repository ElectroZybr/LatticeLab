#include <SFML/Graphics.hpp>
#include "Renderer.h"

#include "physics/Atom.h"
#include "physics/SpatialGrid.h"
#include "SimBox.h"

class Simulation {
public:
    Simulation(sf::RenderWindow& window, SimBox& sim_box);

    void update(float dt);

    void renderShot(float dt);
    void pollEvents();
    void event();
    void setSizeBox(Vec3D newStart, Vec3D newEnd, int cellSize = -1);

    void createRandomAtoms(int type, int quantity);
    Atom* createAtom(Vec3D start_coords, Vec3D start_speed, int type, bool fixed = false);
    void addBond(Atom* a1, Atom* a2);

    double AverageEnegry() const;
    void logEnergies() const;
    void logAtomPos() const;
    void logMousePos() const;
    void logBondList() const;

    void drawGrid(bool flag = true);
    void drawBonds(bool flag = true);

    void speedGradient(bool flag = true);
    void setCameraPos(double x, double y);
    void setCameraZoom(float new_zoom);
    int getSimStep() const { return sim_step; }

    void save(const std::string_view path) const;
    void load(const std::string_view path);
    void clear();

    SimBox& sim_box;
    Renderer render;
    std::vector<Atom> atoms;
private:
    sf::RenderWindow& window;
    sf::View gameView;
    sf::View uiView;

    bool atomMoveFlag = false;
    bool selectionFrameMoveFlag = false;
    Atom* selectedMoveAtom;
    sf::Vector2i start_mouse_pos;
    int sim_step = 0;
    
    bool checkNeighbor(Vec3D coords, float delta);
};

Vec2D randomUnitVector2D();
Vec3D randomUnitVector3D(double amplitude = 1.0);
double randomInRange(int range);
