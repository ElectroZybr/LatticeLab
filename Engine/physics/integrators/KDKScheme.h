#pragma once

class Atom;
struct StepContext;

class KDKScheme {
public:
    void pipeline(StepContext& ctx) const;

    static void halfKick(Atom& atom, double dt);
    static void drift(Atom& atom, double dt);
};
