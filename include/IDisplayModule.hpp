#ifndef ARCADE_IDISPLAYMODULE_HPP_
#define ARCADE_IDISPLAYMODULE_HPP_

#include <string>
#include "Types.hpp"

namespace Arcade {

    class IDisplayModule {
    public:
        virtual ~IDisplayModule() = default;
        virtual void init() = 0;
        virtual void close() = 0;
        virtual const std::string &getName() const = 0;
        virtual void clear() = 0;
        virtual void render(const RenderData &data) = 0;
        virtual void display() = 0;
        virtual Key pollEvent() = 0;
    };
}

#endif
