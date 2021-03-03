#include <iostream>

// defining windows 10
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

std::vector<char> vBuffer(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket& socket)
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
		[&](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				std::cout << "\n\nRead " << length << " bytes\n\n";

				for (uint32_t i = 0; i < length; i++)
					std::cout << vBuffer[i];

				// read leftover data
				GrabSomeData(socket);
			}
		}
	);
}

int main()
{
	asio::error_code ec;

	// platform specific interface
	asio::io_context context;

	// fake task to asio so the context doesn't finish
	asio::io_context::work idleWork(context);

	// start the context
	std::thread thrContext = std::thread([&]() {context.run(); });

	// address with a port where we want to connect to
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);

	// context delivers the implementation
	asio::ip::tcp::socket socket(context);
	socket.connect(endpoint, ec);

	if (!ec)
	{
		std::cout << "Connected" << std::endl;
	}
	else
	{
		std::cout << "Failed to connect to address:" << ec.message() << std::endl;
	}

	if (socket.is_open())
	{
		// priming asio context to read buffer before sending some data
		GrabSomeData(socket);

		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n\r\n";

		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

		// program does something else while asio handles data transfer in background
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(20000ms);

		context.stop();
		if (thrContext.joinable()) 
			thrContext.join();
	}

	system("pause");
	return 0;
}