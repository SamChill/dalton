#include "AtomMatrix.h"
#include "Potential.h"

class MolecularDynamics {
public:
    MolecularDynamics(
        AtomMatrix & coordinates,
        AtomMatrix & velocities,
        Potential * potential,
        double temperature,
        double timeStep) :
    coordinates_(coordinates),
    velocities_(velocities),
    potential_(potential),
    temperature_(temperature),
    timeStep_(timeStep)
    {};

    ~MolecularDynamics();

    AtomMatrix & step();

private:
    AtomMatrix coordinates_;
    AtomMatrix velocities_;
    Potential *potential;
    double temperature_;
    double timeStep_;
};
