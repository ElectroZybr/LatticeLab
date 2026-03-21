#pragma once

class Atom;

struct StepContext;

class VerletScheme {
public:
    void pipeline(StepContext& ctx) const;

    static void predict(Atom& atom, double dt);
    static void correct(Atom& atom, double dt);
};
