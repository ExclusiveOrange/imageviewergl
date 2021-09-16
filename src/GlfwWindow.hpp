#pragma once

#include "IFunctor.hpp"
#include "IGlWindow.hpp"
#include "IGlRenderer.hpp"

#include <functional>
#include <future>
#include <memory>

// This works around the limitation that std::function cannot contain a non-copyable closure.
// A mutable lambda which captures by moving a value, is a non-copyable closure.
// There is a reasonable use case for a mutable lambda which captures by moving a value,
// specifically for this application at least.
// IFunctor is a custom container that can hold a non-copyable closure, or any other closure
// or functor or function.
// See: makeUniqueFunctor.hpp for the convenience function that takes a lambda and gives you
// a unique_ptr< IFunctor >.
using makeGlRenderer_t = std::unique_ptr< IFunctor< std::unique_ptr< IGlRenderer > >>;

// Takes a future which should eventually return a unique_ptr to a make function (see above).
// The make function should promptly return an instance of IGlRenderer;
//   NOTE: the make function may be called from a different thread than makeGlfwWindow was called from;
//   NOTE: the OpenGL context will be active when the make function is called.
//   NOTE: the make function will only be called once.
std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< makeGlRenderer_t > futureMakeGlRenderer
) noexcept( false ); // may throw ErrorString
