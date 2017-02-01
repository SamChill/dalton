#include "LennardJones.h"
#include <cmath>
#include <Eigen/Core>

using std::pow;

LennardJones::LennardJones(AtomMatrix & coordinates)
{
    energy_ = 0.0;
    forces_ = AtomMatrix::Zero(coordinates.rows(), 3);
    for (size_t i=0; i < coordinates.rows(); i++) {
        for (size_t j=i+1; j < coordinates.rows(); j++) {
            Eigen::Vector3d diff = coordinates.row(j) - coordinates.row(i);
            double r = diff.norm();
            double a = pow(r, -6);
            double b = 4.0 * a;
            energy_ += b * (a - 1.0);
            double dU = -6.0 * b / r * (2.0 * a - 1);
            Eigen::Vector3d f = dU*diff/r;
            forces_.row(i) += f;
            forces_.row(j) -= f;
        }
    }
}
