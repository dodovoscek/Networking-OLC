#pragma once

#include "net_common.h"
#include "net_connection.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		class server_interface
		{
		public:
			server_interface(uint16_t port)
				: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{

			}

			virtual ~server_interface()
			{
				Stop();
			}

			bool Start()
			{
				try
				{
					// order is important so the thread doesn't die
					WaitForClientConnection();

					m_threadContext = std::thread([this]() {m_asioContext.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
					return false;
				}

				std::cout << "[SERVER] started!" << std::endl;
				return true;
			}

			void Stop()
			{
				m_asioContext.stop();

				if (m_threadContext.joinable())
					m_threadContext.join();

				std::cout << "[SERVER] stopped!" << std::endl;
			}

			// async - instructs asio to wait for connection
			void WaitForClientConnection()
			{
				m_asioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket) 
					{
						if (!ec)
						{
							std::cout << "[SERVER] New connection: " << socket.remote_endpoint() << std::endl;

							std::shared_ptr<connection<T>> newconn =
								std::make_shared<connection<T>>(connection<T>::owner::server, 
									m_asioContext, std::move(socket), m_qMessagesIn);

							// chance to deny the connection
							if (OnClientConnect(newconn))
							{
								// connection allowed
								m_deqConnections.push_back(std::move(newconn));
								m_deqConnections.back()->ConnectToClient(nIDCounter++);

								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection approved" << std::endl;
							}
							else
							{
								std::cout << "[SERVER] Connection denied." << std::endl;
							}
						}
						else
						{
							std::cout << "[SERVER] New connection error: " << ec.message() << std::endl;
						}

						// prime the asio context with more work - simply wait for another connection
						WaitForClientConnection();
					}
				);
			}

			// send message to a specific client 
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				if (client && client->IsConnected())
				{
					client->Send(msg);
				}
				else
				{
					OnClientDisconnect(client);
					client.reset();
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
				}
			}

			// send message to all clients
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;
				for (auto& client : m_deqConnections)
				{
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
							client->Send(msg);
					}
					else
					{
						OnClientDisconnect(client);
						client.reset();
						bInvalidClientExists = true;
					}
				}

				if (bInvalidClientExists)
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
			}

			// size_t is unsigned therefore -1 is the maximum number
			// size is able to be specified in order to be able to return from update when many messages are present
			void Update(size_t nMaxMessages = -1, bool bWait = false)
			{
				if (bWait) m_qMessagesIn.wait();

				size_t nMessageCount = 0;
				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
				{
					// grab the front message, it gets erased from the queue
					auto msg = m_qMessagesIn.pop_front();

					// pass the message
					OnMessage(msg.remote, msg.msg);

					nMessageCount++;
				}
			}

		protected:
			// called when a client connects, you can veto the connection by returning false
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// called when a client appears to have disconnected
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{
			}

			// called when a message arrives
			virtual void OnMessage(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
			}

		protected:
			// queue for incoming packets
			tsqueue<owned_message<T>> m_qMessagesIn;

			// active validated connections
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			// order of declaration is important - it is also the order of initialisation
			asio::io_context m_asioContext;
			std::thread m_threadContext;

			// these things need an asio context
			asio::ip::tcp::acceptor m_asioAcceptor; // socket of the connected clients

			// clients will be identified in the "wider system" via an ID
			uint32_t nIDCounter = 10000;
		};

	}
}
