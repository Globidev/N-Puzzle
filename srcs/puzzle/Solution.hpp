#pragma once

#include "Puzzle.hpp"

namespace puzzle {

    template <uint size>
    struct Solution: public std::vector<Puzzle<size>> {

        Solution(std::size_t timeComplexity, std::size_t spaceComplexity):
            timeComplexity  { timeComplexity },
            spaceComplexity { spaceComplexity }
        { }

        std::size_t timeComplexity;
        std::size_t spaceComplexity;

        friend std::ostream & operator<<(std::ostream & os, const Solution & solution) {
            for (const auto & puzzle: solution)
                os << puzzle << "\n";
            os  << "A* iterations: " << solution.timeComplexity << "\n"
                << "Total number of states visited: " << solution.spaceComplexity << "\n"
                << "Number of moves required: " << solution.size() << "\n";
            return os;
        }
    };

}