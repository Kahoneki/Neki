#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <stdexcept>
#include <typeindex>


namespace NK
{

	typedef std::uint64_t EventSubscriptionID;
	
	
	class EventManager final
	{
	public:
		template<typename EventPacket>
		static inline EventSubscriptionID Subscribe(std::function<void(const EventPacket&)> _callback)
		{
			const EventSubscriptionID id{ m_nextID++ };

			//Provided _callback parameter is of EventPacket type but needs to be std::any to be in the map
			//Create a small wrapper over the function that casts the parameter
			const std::function<void(const std::any&)> wrapper
			{
				[_callback](const std::any& _data)
				{
					_callback(std::any_cast<const EventPacket&>(_data));
				}
			};
			m_callbacks[std::type_index(typeid(EventPacket))][id] = wrapper;
			return id;
		}


		//For non-static class methods
		template<typename Class, typename EventPacket>
		static inline EventSubscriptionID Subscribe(Class* _classInstance, std::function<void(Class*, const EventPacket&)> _memberCallback)
		{
			return Subscribe<EventPacket>(std::bind(_callback, _classInstance, std::placeholders::_1));
		}

		
		template<typename EventPacket>
		static inline void Unsubscribe(const EventSubscriptionID _subscriptionID)
		{
			std::unordered_map<EventSubscriptionID, std::function<void(const std::any&)>>& callbacks{ m_callbacks[std::type_index(typeid(EventPacket))] };
			if (callbacks.erase(_subscriptionID) == 0)
			{
				//No elements were removed
				throw std::runtime_error("EventManager::Unsubscribe() - provided subscription id is not subscribed to the provided event type");
			}
		}

		
		template<typename EventPacket>
		static inline void Trigger(const EventPacket& _packet)
		{
			//Get all callbacks for this event
			const std::unordered_map<std::type_index, std::unordered_map<EventSubscriptionID, std::function<void(const std::any&)>>>::iterator it{ m_callbacks.find(std::type_index(typeid(EventPacket))) };
			if (it == m_callbacks.end())
			{
				//No callbacks subscribed to this event
				return;
			}

			//Call all callbacks subscribed to this event
			for (std::unordered_map<EventSubscriptionID, std::function<void(const std::any&)>>::const_iterator callbacksIt{ it->second.begin() }; callbacksIt != it->second.end(); ++callbacksIt)
			{
				callbacksIt->second(_packet);
			}
		}


	private:
		//Atomic to maintain uniqueness between threads
		static inline std::atomic<EventSubscriptionID> m_nextID{ 0 };
		
		//Map from event (identified by its unique packet type) to all callbacks subscribed to the event
		//"All callbacks subscribed to the event" is stored as a map from subscription id to the corresponding callback to make lookup easier
		static inline std::unordered_map<std::type_index, std::unordered_map<EventSubscriptionID, std::function<void(const std::any&)>>> m_callbacks;
	};

}