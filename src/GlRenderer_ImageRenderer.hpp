#pragma once

#include "IGlRenderer.hpp"
#include "IGlWindowAppearance.hpp"
#include "IRawImage.hpp"

#include <memory>

std::unique_ptr< IGlRenderer >
makeGlRenderer_ImageRenderer(
    IGlWindowAppearance &,
    std::unique_ptr< IRawImage > )
noexcept( false ); // may throw std::exception
