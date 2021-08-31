#pragma once

#include "IGlWindow.hpp"

#include <memory>

std::unique_ptr< IGlWindow >
makeGlfwWindow() noexcept( false ); // may throw ErrorString
