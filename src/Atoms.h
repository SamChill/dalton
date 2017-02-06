#include <Eigen/Dense>
#include <iostream>
#include <fstream>

typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> AtomMatrix;

class Atoms {
public:
    Atoms(AtomMatrix coordinates) : coordinates_(coordinates), updated_(false) {}

    AtomMatrix coordinates() {
        return coordinates_;
    }

    void setCoordinates(AtomMatrix coordinates) {
        coordinates_ = coordinates;
        updated_ = false;
    }

    size_t size() {
        return coordinates_.rows();
    }

    void update() {
        using std::pow;

        if (updated_) {
            return;
        }

        energy_ = 0.0;
        forces_ = AtomMatrix::Zero(size(), 3);
        for (size_t i=0; i < size()-1; i++) {
            for (size_t j=i+1; j < size(); j++) {
                Eigen::Vector3f diff = coordinates_.row(i) - coordinates_.row(j);

                float r = diff.norm();
                if (r < 0.2) {
                    r = 0.2;
                }
                float a = pow(r, -6);
                float b = 4.0 * a;
                energy_ += b * (a - 1.0);
                float dU = -6.0 * b / r * (2.0 * a - 1.0);
                Eigen::Vector3f f = dU*diff/r;
                forces_.row(i) -= f;
                forces_.row(j) += f;
            }
        }
        updated_ = true;
    }

    void xyz(std::string filename) {
        std::ofstream file(filename);
        using std::endl;
        using std::to_string;
        file << to_string(size()) << endl;
        file << "Made by Dalton" << endl;
        for (size_t i=0; i<size(); i++)
            file << "C " << coordinates_.row(i) << endl;
    }

    AtomMatrix forces() {
        update();
        return forces_;
    }

    double energy() {
        update();
        return energy_;
    }

private:
    AtomMatrix coordinates_;
    AtomMatrix velocities_;
    AtomMatrix forces_;
    double energy_;
    bool updated_;
};
