#pragma once

// Energy-preserving coupling between the network's resonators.
//
// Coupling is described by a skew-symmetric generator S (S_ij = -S_ji = theta_e for
// each edge e = (i, j)). The scattering matrix is its Cayley transform
//
//      Q = (I - S/2)^-1 (I + S/2)
//
// which is orthogonal for every S: Q^T Q = I, ||Q||_2 = 1. A single edge with
// generator theta rotates the two node signals by phi = 2 atan(theta / 2); the energy
// moved per pass is sin^2(phi). Orthogonality means scattering can neither create nor
// destroy energy, independent of the order or number of edges — see
// docs/NETWORK_COUPLING.md for the stability argument.

#include <array>

namespace arc::dsp
{

inline constexpr int kNumNodes = 5; // 0 = CORE, 1 = A, 2 = B, 3 = C, 4 = D
inline constexpr int kNumEdges = 10; // every pair

enum NodeIndex : int
{
    kCore = 0,
    kNodeA = 1,
    kNodeB = 2,
    kNodeC = 3,
    kNodeD = 4
};

struct Edge
{
    int a, b;
};

// Spokes 0..3, ring 4..7 (A-B, B-D, D-C, C-A: around the square A B / C D), cross 8..9.
inline constexpr std::array<Edge, kNumEdges> kEdges { { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 },
                                                        { 1, 2 }, { 2, 4 }, { 4, 3 }, { 3, 1 },
                                                        { 1, 4 }, { 2, 3 } } };

enum class Topology : int
{
    star = 0,  // CORE <-> each node
    ring,      // star + outer ring (default)
    web,       // every pair
    chain,     // CORE - A - B - D - C  (energy must travel node to node)
    numTopologies
};

/** 1 where the topology contains the edge. */
std::array<float, kNumEdges> topologyMask (Topology t) noexcept;

using Matrix5 = std::array<std::array<float, kNumNodes>, kNumNodes>;

/** Orthogonal scattering matrix from per-edge generator values (Cayley transform). */
Matrix5 scatteringFromEdges (const std::array<float, kNumEdges>& theta) noexcept;

/** Generator value giving a single-edge rotation of phi radians. */
inline float generatorForRotation (float phi) noexcept;

/** ||Q^T Q - I||_max, for tests. */
float orthogonalityError (const Matrix5& q) noexcept;

} // namespace arc::dsp

#include <cmath>

inline float arc::dsp::generatorForRotation (float phi) noexcept
{
    return 2.0f * std::tan (0.5f * phi);
}
