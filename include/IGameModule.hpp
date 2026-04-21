#ifndef ARCADE_IGAMEMODULE_HPP_
#define ARCADE_IGAMEMODULE_HPP_

#include <string>
#include "Types.hpp"

namespace Arcade {

    class IGameModule {
    public:
        virtual ~IGameModule() = default;
        virtual void init() = 0;
        virtual void reset() = 0;
        virtual void close() = 0;
        virtual const std::string &getName() const = 0;
        virtual void handleInput(Key key) = 0;
        virtual void update() = 0;
        virtual const RenderData &getRenderData() = 0;
        virtual int getScore() const = 0;
        virtual bool isGameOver() const = 0;
        virtual void setContext(const GameContext &) {}
        virtual GameRequest pullRequest() { return {}; }
        virtual const std::string &getPlayerName() const
        {
            static const std::string empty;
            return empty;
        }
    };

}

#endif
