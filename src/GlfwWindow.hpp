#pragma once

#include "IFunctor.hpp"
#include "IGlWindow.hpp"
#include "IGlRenderer.hpp"

#include <functional>
#include <future>
#include <memory>

// Must be a unique_ptr instead of a function because a function must be copyable
// to be assigned to a promise but a mutable function cannot be copied,
// and there is a reasonable use case for a mutable function.
//using makeGlRenderer_fn_t = std::function< std::unique_ptr< IGlRenderer >( void ) >;
//using makeGlRenderer_t = std::unique_ptr< makeGlRenderer_fn_t >;

using makeGlRenderer_t = std::unique_ptr< IFunctor< std::unique_ptr< IGlRenderer >, void >( void )>;

// Takes a future which should eventually return a unique_ptr to a make function (see above).
// The make function should promptly return an instance of IGlRenderer;
//   NOTE: the make function may be called from a different thread than makeGlfwWindow was called from;
//   NOTE: the OpenGL context will be active when the make function is called.
//   NOTE: the make function will only be called once.
std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< makeGlRenderer_t > futureMakeGlRenderer
) noexcept( false ); // may throw ErrorString
