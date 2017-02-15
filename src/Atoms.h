#pragma once
#include <Eigen/Dense>
#include <vector>
#include "Elements.h"
#include <iostream>
#include <cassert>

typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> AtomMatrix;
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> NeighborList;

class Atoms {
public:
    Atoms() {}
    Atoms(std::vector<Element> elements, AtomMatrix& coordinates)
    :elements_(elements), coordinates_(coordinates), updated_(false)
    {
        assert(elements.size() == coordinates.rows());
    }

    AtomMatrix coordinates() {
        return coordinates_;
    }

    Eigen::VectorXf radii() {
        Eigen::VectorXf radii = Eigen::VectorXf::Zero(size());
        for (int i=0; i<size(); i++) {
            radii(i) = elements_[i].radius;
        }
        return radii;
    }

    NeighborList neighborList(int neighbor_count);

    AtomMatrix colors() {
        AtomMatrix colors = AtomMatrix::Zero(size(), 3);
        for (int i=0; i<size(); i++) {
            colors.row(i) = elements_[i].color;
        }
        return colors;
    }

    void setCoordinates(AtomMatrix& coordinates) {
        coordinates_ = coordinates;
        updated_ = false;
    }

    size_t size() {
        return coordinates_.rows();
    }

    void update();

    void saveXYZ(std::string filename);

    static Atoms readXYZ(std::string filename);

    AtomMatrix forces() {
        update();
        return forces_;
    }

    double energy() {
        update();
        return energy_;
    }

private:
    std::vector<Element> elements_;
    AtomMatrix coordinates_;
    AtomMatrix forces_;
    NeighborList neighbor_list_;
    double energy_;
    bool updated_;
};
