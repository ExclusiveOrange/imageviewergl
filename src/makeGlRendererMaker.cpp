#include "makeGlRendererMaker.hpp"

#include "GlRenderer_ImageRenderer.hpp"
#include "loadImageFile.hpp"

std::unique_ptr< IGlRendererMaker >
makeGlRendererMaker( std::string imageFilename )
{
  std::unique_ptr< IRawImage > rawImage = loadImageFile( imageFilename.c_str());

  struct GlRendererMaker : public IGlRendererMaker
  {
    std::unique_ptr< IRawImage > rawImage;

    GlRendererMaker( std::unique_ptr< IRawImage > rawImage )
        : rawImage{ std::move( rawImage ) } {}

    virtual
    std::unique_ptr< IGlRenderer >
    makeGlRenderer() override
    {
      return makeGlRenderer_ImageRenderer( std::move( rawImage ));
    }
  };

  return std::make_unique< GlRendererMaker >( std::move( rawImage ));
}

