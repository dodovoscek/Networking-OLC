#pragma once
#include "net_common.h"

namespace olc
{
	namespace net
	{
		// start of all messages, template used for enum class => messages valid in compile time
		template <typename T>
		struct message_header
		{
			T id{};
			uint32_t size = 0;
		};

		template <typename T>
		struct message
		{
			message_header<T> header{};
			std::vector<uint8_t> body; // always working with bytes

			size_t size() const
			{
				return sizeof(message_header<T>) + body.size();
			}

			friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
			{
				os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}

			// pushes any POD like data into message buffer
			template <typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				// check that the type of the data is trivially copyable
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// cache current size of vector, as this will be the poinet we insert the data
				size_t i = msg.body.size;

				// resize te vector by the size of the data being pushed
				msg.body.resize(msg.body.size + sizeof(DataType));

				// pshysically copy the data into the newly allocated vector space
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// update message size
				msg.header.size = msg.size();

				// allows chaining
				return msg;
			}

			template <typename DataType>
			friend message<T>& operator >> (message<T>& msg, const DataType& data)
			{
				// check that the type of the data is trivially copyable
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// cache the location towards the end of the vector where the pulled data starts
				size_t i = msg.body.size() - sizeof(DataType);

				// pshysically copy the data from the vector into the usere variable
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// shrink the vector to remove read bytes, and reset end position
				msg.body.resize(i);

				// update message size
				msg.header.size = msg.size();

				// allows chaining
				return msg;
			}
		};
	}
}