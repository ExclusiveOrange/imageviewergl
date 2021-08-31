#pragma once

#include "IGlWindow.hpp"

#include <memory>

std::unique_ptr< IGlWindow >
makeGlfwWindow();
