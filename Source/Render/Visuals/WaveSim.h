#pragma once

#include <vector>

using WaveGrid = std::vector<std::vector<double>>;

// 波動方程式に基づく状態の更新関数
inline void updateWave(const WaveGrid& h_curr, const WaveGrid& h_prev, WaveGrid& h_next, double c)
{
    int height = static_cast<int>(h_curr.size());
    int width = static_cast<int>(h_curr[0].size());

    // 境界（端）は計算から除外するため、1から size-1 までループします
    // （境界条件は固定端、つまり常に0として扱っています）
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // 提供された数式通りの実装
            h_next[y][x] = 2.0 * h_curr[y][x]
                + c * (h_curr[y][x + 1] + h_curr[y][x - 1] + h_curr[y + 1][x] + h_curr[y - 1][x] - 4.0 * h_curr[y][x])
                - h_prev[y][x];
        }
    }
}
