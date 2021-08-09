#include <algorithm>
#include <cstdio>
#include <memory>

int main()
{
  const size_t len = 32*32*59;
  auto buf = std::make_unique<float[]>(len);
  const size_t half_len = 32*32*32;

  std::fill( buf.get(), buf.get() + half_len, 3.14 );
  std::fill( buf.get() + half_len, buf.get() + len, -3.14 );

  auto* f = std::fopen( "buf.bin", "wb" );
  std::fwrite( buf.get(), sizeof(float), len, f );
  std::fclose( f );
}
