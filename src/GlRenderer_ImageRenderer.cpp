#include "GlRenderer_ImageRenderer.hpp"

namespace
{
struct GlRenderer : public IGlRenderer
{
  virtual ~GlRenderer() override = default;

  virtual void render() override
  {
    // TODO
  }
};
} // namespace

std::unique_ptr< IGlRenderer >
makeGlRenderer_ImageRenderer()
{
  return std::make_unique< GlRenderer >();
}
