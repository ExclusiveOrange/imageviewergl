#pragma once

#include "IGlWindow.hpp"

#include "IGlRenderer.hpp"

#include <functional>
#include <future>
#include <memory>

using makeGlRenderer_t = std::function< std::unique_ptr< IGlRenderer >( void ) >;

// takes a future which should eventually return a make function;
// the make function should promptly return an instance of IGlRenderer;
//   NOTE: the make function may be called from a different thread than makeGlfwWindow was called from;
//   NOTE: the OpenGL context will be active when the make function is called.

std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< makeGlRenderer_t > futureMakeGlRenderer
) noexcept( false ); // may throw ErrorString
