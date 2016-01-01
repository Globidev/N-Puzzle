#pragma once

#include "parsing/forwards.hpp"

#include "options/Options.hpp"

#include "exceptions.hpp"
#include "heuristics.hpp"

#include "path_finding/astar.hpp"
#include "path_finding/idastar.hpp"

#include "tools/Benchmark.hpp"
#include "tools/ansi.hpp"

namespace algorithm {

    template <HClass Heuristic, bool uniform, bool ida>
    class Solver {

        using MaybeState = boost::optional<parsing::ParsedPuzzle>;

    public:

        Solver(const MaybeState & initial, const MaybeState & goal):
            initial { initial },
            goal    { goal } {

            if (initial)
                modelState = *initial;
            else if (goal)
                modelState = *goal;
        }

        template <class F>
        void solve(F onSolved) const {
            // Generate both
            if (!initial && !goal) {
                if (Options::generationSize == 0)
                    throw error::ZeroGeneration { };

                findAndApplyStaticSolver<1>(Options::generationSize, onSolved);
            }
            // Or use the model
            else {
                if (modelState.size() == 0)
                    throw error::EmptyPuzzle { };

                findAndApplyStaticSolver<1>(modelState.size(), onSolved);
            }
        }

    private:

        MaybeState initial, goal;
        parsing::ParsedPuzzle modelState;

        // This is the solver endpoint
        template <uint size, class F>
        void solve(Puzzle<size> start, Puzzle<size> end, F onSolved) const {
            std::cout << "==============\n\n";
            std::cout << "Initial state:\n\n"
                      << ansi::BOLD << start << ansi::RESET << "\n\n";
            std::cout << "Final state:\n\n"
                      << ansi::BOLD << end << ansi::RESET << "\n";
            std::cout << "==============\n\n";

            if (!isSolvable(start, end))
                throw error::PuzzleNotSolvable { };

            std::cout << "A* variant:      " << ansi::BOLD << ansi::YELLOW
                      << Options::astarVariant << ansi::RESET << "\n";
            std::cout << "Search strategy: " << ansi::BOLD << ansi::YELLOW
                      << Options::searchStrategy << ansi::RESET << "\n";
            prettyPrintHeuristics();
            if (!initial || (!goal && Options::randomGoal))
                std::cout << "Random seed:     " << ansi::BOLD
                          << Options::randomSeed << ansi::RESET << "\n";
            std::cout << "\n==============\n" << std::endl;

            tools::Benchmark bench { "Computation time" };
            if (ida)
                onSolved(idastar<Heuristic, uniform>(start, end));
            else
                onSolved(astar<Heuristic, uniform>(start, end));

            std::cout << "==============\n\n";
        }

        static void prettyPrintHeuristics() {
            using namespace algorithm::heuristics;
            std::set<std::string> heuristics {
                Options::heuristics.begin(),
                Options::heuristics.end()
            };

            std::cout << "Heuristics:      " << ansi::BOLD << ansi::YELLOW
                      << prettyNames[*heuristics.begin()];
            auto it = std::next(heuristics.begin());
            for (; it != heuristics.end(); ++it)
                std::cout << " + " << prettyNames[*it];
            std::cout << ansi::RESET << "\n";

            // All of our heuristics are admissible in standalone
            bool admissible = (heuristics.size() == 1);
            // Special case for Manhattan + Linear which is also admissible
            if (heuristics.size() == 2
                && heuristics.count(manhattanName)
                && heuristics.count(linearName))
                admissible = true;

            std::cout << "Admissible:      " << ansi::BOLD
                      << (admissible ? ansi::GREEN : ansi::RED)
                      << (admissible ? "Yes" : "No")
                      << ansi::RESET << "\n";
        }

        // Calls the endpoint with static puzzles generated from either parsed
        // puzzles or a random generator
        template <uint size, class F>
        void solveParsed(F onSolved) const {
            Puzzle<size> start, end;

            // Building the puzzles that are fixed
            if (initial)
                start = puzzle::buildStaticPuzzle<size>(*initial);
            if (goal) {
                if (goal->size() != size)
                    throw error::GoalSizeMismatch { goal->size(), size };
                end = puzzle::buildStaticPuzzle<size>(*goal);
            }

            // Handling all the cases in order to always have a solvable pair
            auto snail = puzzle::makeSnail<size>();
            if (!initial) {
                if (goal) // Fixed goal -> match it
                    start = puzzle::generateMatch(end);
                else if (Options::randomGoal) // Random goal -> no constraints
                    start = puzzle::generate<size>();
                else // Snail goal -> match it
                    start = puzzle::generateMatch(snail);
            }
            if (!goal) {
                if (Options::randomGoal) // Fixed or not, match the start
                    end = puzzle::generateMatch(start);
                else // Start matches the snail so it is ok
                    end = snail;
            }

            solve(start, end, onSolved);
        }

        // Runtime value unrolling
        // -> iterating over the puzzle size (determined at runtime) to apply a
        // solving function with a static size (i.e. known at compile time)
        // This should allow some badass optimizations
        template <uint size, class F>
        std::enable_if_t<(size > MAX_PUZZLE_SIZE)>
        findAndApplyStaticSolver(uint runtimeSize, F) const {
            throw error::PuzzleSizeTooLarge { runtimeSize };
        }

        template <uint size, class F>
        std::enable_if_t<size <= MAX_PUZZLE_SIZE>
        findAndApplyStaticSolver(uint runtimeSize, F onSolved) const {
            if (runtimeSize == size)
                return solveParsed<size>(onSolved);

            findAndApplyStaticSolver<size + 1>(runtimeSize, onSolved);
        }

    };

}
