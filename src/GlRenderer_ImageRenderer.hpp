#pragma once

#include "IGlRenderer.hpp"
#include "IRawImage.hpp"

#include <memory>

std::unique_ptr< IGlRenderer >
makeGlRenderer_ImageRenderer( std::unique_ptr< IRawImage > );
