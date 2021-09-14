#pragma once

#include "IRawImage.hpp"

#include <memory>

std::unique_ptr< IRawImage >
makeRawImage_StbImage( const char *filename )
noexcept( false ); // throws ErrorString


