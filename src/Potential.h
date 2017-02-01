#include "AtomMatrix.h"

class Potential {
public:
    virtual double energy() = 0;
    virtual AtomMatrix & forces() = 0;
};
