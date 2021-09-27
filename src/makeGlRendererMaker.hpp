#pragma once

#include "IGlRendererMaker.hpp"

#include <memory>
#include <string>

std::unique_ptr< IGlRendererMaker >
makeGlRendererMaker( const std::string &imageFilename );
