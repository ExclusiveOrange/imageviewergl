#pragma once

#include "IGlRenderer.hpp"

#include <memory>

std::unique_ptr< IGlRenderer >
makeGlRenderer_ImageRenderer();
