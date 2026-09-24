#include "Synthesis/CouplingMatrix.h"

#include <algorithm>
#include <cmath>

namespace arc::dsp
{

std::array<float, kNumEdges> topologyMask (Topology t) noexcept
{
    std::array<float, kNumEdges> m {};
    switch (t)
    {
        case Topology::star:
            for (int e = 0; e < 4; ++e)
                m[static_cast<size_t> (e)] = 1.0f;
            break;
        case Topology::ring:
            for (int e = 0; e < 8; ++e)
                m[static_cast<size_t> (e)] = 1.0f;
            break;
        case Topology::web:
            m.fill (1.0f);
            break;
        case Topology::chain:
            m[0] = 1.0f; // CORE-A
            m[4] = 1.0f; // A-B
            m[5] = 1.0f; // B-D
            m[6] = 1.0f; // D-C
            break;
        case Topology::numTopologies:
            break;
    }
    return m;
}

Matrix5 scatteringFromEdges (const std::array<float, kNumEdges>& theta) noexcept
{
    // A = I - S/2, B = I + S/2, solve A Q = B. The symmetric part of A is I, so A is
    // positive real and LU without pivoting is stable.
    double A[kNumNodes][kNumNodes] {};
    double B[kNumNodes][kNumNodes] {};
    for (int i = 0; i < kNumNodes; ++i)
    {
        A[i][i] = 1.0;
        B[i][i] = 1.0;
    }
    for (int e = 0; e < kNumEdges; ++e)
    {
        const auto& edge = kEdges[static_cast<size_t> (e)];
        const double s = 0.5 * static_cast<double> (theta[static_cast<size_t> (e)]);
        // S_ab = +theta, S_ba = -theta
        A[edge.a][edge.b] -= s;
        A[edge.b][edge.a] += s;
        B[edge.a][edge.b] += s;
        B[edge.b][edge.a] -= s;
    }

    for (int k = 0; k < kNumNodes; ++k)
        for (int i = k + 1; i < kNumNodes; ++i)
        {
            A[i][k] /= A[k][k];
            for (int j = k + 1; j < kNumNodes; ++j)
                A[i][j] -= A[i][k] * A[k][j];
        }

    Matrix5 q {};
    for (int c = 0; c < kNumNodes; ++c)
    {
        double y[kNumNodes];
        for (int i = 0; i < kNumNodes; ++i)
        {
            double v = B[i][c];
            for (int j = 0; j < i; ++j)
                v -= A[i][j] * y[j];
            y[i] = v;
        }
        for (int i = kNumNodes - 1; i >= 0; --i)
        {
            double v = y[i];
            for (int j = i + 1; j < kNumNodes; ++j)
                v -= A[i][j] * y[j];
            y[i] = v / A[i][i];
        }
        for (int i = 0; i < kNumNodes; ++i)
            q[static_cast<size_t> (i)][static_cast<size_t> (c)] = static_cast<float> (y[i]);
    }
    return q;
}

float orthogonalityError (const Matrix5& q) noexcept
{
    float worst = 0.0f;
    for (int i = 0; i < kNumNodes; ++i)
        for (int j = 0; j < kNumNodes; ++j)
        {
            double s = 0.0;
            for (int k = 0; k < kNumNodes; ++k)
                s += static_cast<double> (q[static_cast<size_t> (k)][static_cast<size_t> (i)])
                     * static_cast<double> (q[static_cast<size_t> (k)][static_cast<size_t> (j)]);
            worst = std::max (worst, static_cast<float> (std::abs (s - (i == j ? 1.0 : 0.0))));
        }
    return worst;
}

} // namespace arc::dsp
