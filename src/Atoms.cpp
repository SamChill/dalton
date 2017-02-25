#include "Atoms.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <iomanip>

void Atoms::update()
{
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

void Atoms::saveXYZ(std::string filename)
{
    std::ofstream file(filename);
    using std::endl;
    using std::to_string;
    file << to_string(size()) << endl;
    file << "Made by Dalton" << endl;
    for (size_t i=0; i<size(); i++)
        file << "C " << coordinates_.row(i) << endl;
}

std::vector<Atoms> Atoms::readXYZ(std::string filename)
{
    std::ifstream file(filename);
    std::string line;
    std::vector<Atoms> traj;

    while (true) {
        std::getline(file, line);
        if (file.bad()) {
            break;
        } else if (file.eof()) {
            break;
        }
        int number_of_atoms = std::stoi(line);
        std::vector<Element> elements;
        AtomMatrix coordinates = AtomMatrix::Zero(number_of_atoms, 3);

        // blank line
        std::getline(file, line);

        for (int i=0; i < number_of_atoms; i++) {
            std::getline(file, line);
            std::istringstream iss(line);
            std::vector<std::string> fields{std::istream_iterator<std::string>(iss), {}};
            elements.push_back(Elements::symbol_to_element[fields[0]]);
            coordinates(i, 0) = std::stof(fields[1]);
            coordinates(i, 1) = std::stof(fields[2]);
            coordinates(i, 2) = std::stof(fields[3]);
        }

        Atoms atoms(elements, coordinates);
        traj.push_back(atoms);
    }

    return traj;
}

NeighborList Atoms::neighborList(int neighbor_count)
{
    NeighborList neighbor_list = NeighborList::Zero(size(), neighbor_count);
    Eigen::MatrixXf distance_matrix = Eigen::MatrixXf::Zero(size(), size());
    for (int i=0; i<size(); i++) {
        for (int j=i+1; j<size(); j++) {
            distance_matrix(i,j) = (coordinates_.row(i) - coordinates_.row(j)).squaredNorm();
            distance_matrix(j,i) = distance_matrix(i,j);
        }
    }

    for (int i=0;i<size();i++) {
        Eigen::VectorXf neighor_distances = Eigen::VectorXf::Constant(neighbor_count, 99999.0);
        for (int j=0;j<size();j++) {
            if (i == j) continue;
            float d = distance_matrix(i,j);
            Eigen::VectorXf::Index max_idx;
            neighor_distances.maxCoeff(&max_idx);
            if (d < neighor_distances(max_idx)) {
                neighbor_list(i,max_idx) = j;
                neighor_distances(max_idx) = d;
            }
        }
    }

    for (int i=0; i<size(); i++) {
        std::sort(
            neighbor_list.row(i).data(),
            neighbor_list.row(i).data() + neighbor_list.row(i).size(),
            [i, &distance_matrix](int a, int b){return distance_matrix(i,a) < distance_matrix(i,b);}
        );
    }

    return neighbor_list;
}
