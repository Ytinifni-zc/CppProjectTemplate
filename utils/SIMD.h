//
// Created by inFinity on 2019-10-30.
//

#ifndef KALEIDO_SIMD_H
#define KALEIDO_SIMD_H

#include <immintrin.h>
#include <cassert>

#include <src/Graph.h>
#include <utils/Type.h>
#include <utils/Gang.h>

using SIMDFloat = __m256;
const UInt32 NUM = 256 / 8 / sizeof(Float32);

auto setSIMDFloat = [](const auto &unit) {
//        return _mm256_setr_ps(unit[0], unit[1], unit[2], unit[3],
//                              unit[4], unit[5], unit[6], unit[7]);
    return _mm256_load_ps(unit);
};

auto setSingleFloat = [](const Float32 &value) {
    return _mm256_broadcast_ss(&value);
};

auto dotProduct = [](const SIMDFloat &x, const SIMDFloat &y) {
    auto xy = _mm256_mul_ps(x, y);
    auto addr = (Float32 *) &xy;
    auto ret = 0.0;
    for (auto i = 0u; i < NUM; ++i) ret += addr[i];
    return ret;
};

auto vector2SIMDFloat = [](const std::vector<Float32> &vec) {
    auto size = ((vec.size() % NUM) ? 1 : 0) + vec.size() / NUM;
    std::vector<SIMDFloat> ret(size);
    GangUtil gu;
    gu.submit(size, [&](UInt32 i) {
        Float32 unit[NUM];
        std::fill(unit, unit + NUM, 0.0);
        for (auto j = 0u; j < NUM; ++j) {
            auto idx = i * NUM + j;
            if (idx < vec.size()) {
                unit[j] = vec[idx];
            }
        }
        ret[i] = setSIMDFloat(unit);
    });
    return ret;
};

struct SIMDMatrix {
    /// The atomic unit is a 8x1 __m256 (Float32)
    UInt32 vertex_num{};
    std::vector<std::vector<UInt32>> SIMD_offsets;
    using SIMDNgh = std::vector<SIMDFloat>;
    std::vector<SIMDNgh> SIMD_data;

    SIMDMatrix() = default;

    SIMDMatrix(Graph &g, bool in_or_out) {
        vertex_num = g.vertex_num;
        SIMD_offsets.resize(vertex_num);
        SIMD_data.resize(vertex_num);

        GangUtil gu;

        gu.submit(vertex_num, [&](UInt32 v) {
            const auto &ngh = in_or_out ? g.inList(v) : g.outList(v);
            if (ngh.empty()) return;
            Float32 unit[NUM];
            std::fill(unit, unit + NUM, 0.0);
            SIMD_offsets[v].emplace_back(ngh[0] / NUM);
            for (auto i = 0u; i < ngh.size(); ++i) {
                unit[i % NUM] += 1;
                if (i + 1 == ngh.size() || ngh[i + 1] - ngh[i] >= NUM) {
                    SIMD_data[v].emplace_back(setSIMDFloat(unit));
                    std::fill(unit, unit + NUM, 0.0);
                    if (i + 1 != ngh.size()) {
                        SIMD_offsets[v].emplace_back(ngh[i + 1] / NUM);
                    }
                }
            }
        });
    }
};

std::vector<SIMDFloat> matrixMultiplyVector(const SIMDMatrix &sm, const std::vector<SIMDFloat> &vec) {
    assert(((sm.vertex_num % NUM) ? 1 : 0) + sm.vertex_num / NUM == vec.size());
//    assert(sm.vertex_num/NUM == vec.size() || sm.vertex_num/NUM + 1 == vec.size());
    std::vector<Float32> ret(sm.vertex_num, 0.0);
    GangUtil gu;
    gu.submit(sm.vertex_num, [&](UInt32 v) {
        const auto &offset = sm.SIMD_offsets[v];
        const auto &data = sm.SIMD_data[v];
        for (auto i = 0u; i < offset.size(); ++i) {
            auto off = offset[i];
            const auto &mx = data[i];
            const auto &vy = vec[off];
            ret[v] += dotProduct(mx, vy);
        }
    });
    return vector2SIMDFloat(ret);
}

#endif //KALEIDO_SIMD_H
