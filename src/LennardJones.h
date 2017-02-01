#include "Potential.h"

class LennardJones : public Potential {
public:
    LennardJones(AtomMatrix & coordinates);

    ~LennardJones();

    double energy() { return energy_; }

    AtomMatrix & forces() { return forces_; }

private:
    double energy_;
    AtomMatrix forces_;
};
