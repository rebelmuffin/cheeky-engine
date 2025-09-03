#pragma once

namespace Game
{
    /// Structure that defines all the information necessary to process a tick update.
    struct GameTime
    {
        /// Time passed since last update.
        float delta_time_seconds;

        /// Time passed since the game started.
        float game_time_seconds;
    };
} // namespace Game