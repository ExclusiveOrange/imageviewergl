#pragma once

#include "IRawImage.hpp"

#include <memory>

std::unique_ptr< IRawImage >
loadImageFile( const char *filename )
noexcept( false ); // throws ErrorString


