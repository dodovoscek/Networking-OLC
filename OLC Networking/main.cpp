// defining windows 10
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

int main()
{
	asio::error_code ec;

	// platform specific interface
	asio::io_context context;

	return 0;
}