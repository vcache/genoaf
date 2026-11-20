#pragma once

namespace genoaf::scene
{
    enum class RevalidationPass
    {
        kStructure  = 0,
        kProperties = 1,
        kAnimation  = 2,
        kOrigins    = 3,
        kTransforms = 4,
        kOutlines   = 5,
        kVisibility = 6,

        kCount // Must be the last one
    };

    constexpr static size_t kRevalidationPassCount =
        static_cast<size_t>(RevalidationPass::kCount);
}