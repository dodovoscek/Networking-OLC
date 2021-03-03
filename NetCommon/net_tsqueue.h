#pragma once
// net thread safe queue
#include "net_common.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class tsqueue
		{
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete; // not copyable because of mutexes
			virtual ~tsqueue() { clear(); }

			const T& front()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.front();
			}

			void push_back(const T& item)
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_back(std::move(item));
			}

			void push_front(const T& item)
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_front(std::move(item));
			}

			const T& back()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.back();
			}

			bool empty()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}

			size_t count()
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.size();
			}

			void clear()
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.clear();
			}

			void wait()
			{
				while(empty())
				{
					std::unique_lock<std::mutex> ul(muxBlocking);
					cvBlocking.wait(ul);
				}
			}

			T pop_front()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.front()); // locally caching object so it is possible to return it
				deqQueue.pop_front();
				return t;
			}

			T pop_back()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.back());
				deqQueue.pop_back();
				return t;
			}
		protected:
			std::mutex muxQueue;
			std::deque<T> deqQueue;
			std::condition_variable cvBlocking;
			std::mutex muxBlocking;
		};
	}
}