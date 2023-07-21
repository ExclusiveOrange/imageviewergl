#include "makeGlRendererMaker.hpp"

#include "GlRenderer_ImageRenderer.hpp"
#include "loadImageFile.hpp"

// DELETE
#include <iostream>

std::unique_ptr< IGlRendererMaker >
makeGlRendererMaker( const std::string &imageFilename )
{
  std::unique_ptr< IRawImage > rawImage = loadImageFile( imageFilename.c_str());

  struct GlRendererMaker : public IGlRendererMaker
  {
    std::unique_ptr< IRawImage > rawImage;

    explicit
    GlRendererMaker( std::unique_ptr< IRawImage > rawImage )
        : rawImage{ std::move( rawImage ) } {}

    std::unique_ptr< IGlRenderer >
    makeGlRenderer() override
    {
      return makeGlRenderer_ImageRenderer( std::move( rawImage ));
    }
  };

  return std::make_unique< GlRendererMaker >( std::move( rawImage ));
}

