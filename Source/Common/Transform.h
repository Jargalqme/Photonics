#pragma once
#include <SimpleMath.h>

struct Transform
{
    DirectX::SimpleMath::Vector3 scale    = { 1.0f, 1.0f, 1.0f };
    DirectX::SimpleMath::Vector3 rotation = { 0.0f, 0.0f, 0.0f };  // オイラー角（ラジアン: pitch, yaw, roll）
    DirectX::SimpleMath::Vector3 position = { 0.0f, 0.0f, 0.0f };

    DirectX::SimpleMath::Matrix getMatrix() const
    {
        const DirectX::SimpleMath::Matrix S = DirectX::SimpleMath::Matrix::CreateScale(scale);

        const DirectX::SimpleMath::Matrix R = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(
            rotation.y,  // yaw   (Y軸) — 左右回転
            rotation.x,  // pitch (X軸) — 上下回転
            rotation.z   // roll  (Z軸) — 傾き
        );

        const DirectX::SimpleMath::Matrix T = DirectX::SimpleMath::Matrix::CreateTranslation(position);

        return S * R * T;
    }

    void reset()
    {
        scale    = DirectX::SimpleMath::Vector3::One;
        rotation = DirectX::SimpleMath::Vector3::Zero;
        position = DirectX::SimpleMath::Vector3::Zero;
    }
};
