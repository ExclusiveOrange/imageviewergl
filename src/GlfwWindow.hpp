#pragma once

#include "IGlRendererMaker.hpp"
#include "IGlWindow.hpp"

#include <future>
#include <memory>

std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< std::unique_ptr< IGlRendererMaker >>
) noexcept( false ); // may throw ErrorString
