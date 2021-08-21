#include "readFile.hpp"

#include <fstream>
#include <stdexcept>

std::vector<char>
readFile( const char *filePath )
{
  if( std::ifstream file{ filePath, std::ios::ate | std::ios::binary }; !file.is_open())
    throw std::runtime_error( std::string( "failed to open file: " ) + filePath );
  else
  {
    const auto fileSize = file.tellg();
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data(), fileSize );

    return buffer;
  }
}
