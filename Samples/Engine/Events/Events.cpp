#include <iostream>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Managers/EventManager.h>



struct ExampleEvent
{
	int x;
};



class ExampleEventTriggeringClass
{
public:
	ExampleEventTriggeringClass() = default;

	void SetExampleEventDataX(const int _x) { m_x = _x; }
	
	void Trigger() const
	{
		ExampleEvent event{};
		event.x = m_x;
		NK::EventManager::Trigger(event);
	}


private:
	int m_x;
};



class ExampleEventListeningClass
{
public:
	ExampleEventListeningClass() = default;
	~ExampleEventListeningClass()
	{
		Unsubscribe();
	}
	
	void Subscribe()
	{
		m_subscriptionID = NK::EventManager::Subscribe<ExampleEvent>(std::bind(&ExampleEventListeningClass::ExampleEventCallback, this, std::placeholders::_1));
	}
	
	void Unsubscribe() const
	{
		NK::EventManager::Unsubscribe<ExampleEvent>(m_subscriptionID);
	}
	
	void ExampleEventCallback(const ExampleEvent& _data) const
	{
		std::cout << _data.x;
	}


private:
	NK::EventSubscriptionID m_subscriptionID;
};



class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(0)
	{
		ExampleEventTriggeringClass trigger{};

		{
			ExampleEventListeningClass listen{};
		
			trigger.SetExampleEventDataX(5);
			std::cout << "Nothing should be outputted: ";
			trigger.Trigger();
			std::cout << '\n';

			listen.Subscribe();
			std::cout << "5 should be outputted: ";
			trigger.Trigger();
			std::cout << '\n';

			listen.Unsubscribe();
			std::cout << "Nothing should be outputted: ";
			trigger.Trigger();
			std::cout << '\n';

			listen.Subscribe();
		}

		//At this point, listen's destructor has been called, and it should be unsubscribed
		std::cout << "Nothing should be outputted: ";
		trigger.Trigger();
		std::cout << '\n';
	}


	virtual void Update() override {}
};



class GameApp final : public NK::Application
{
public:
	explicit GameApp()
	{
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_shutdown = true;
	}



	virtual void Update() override {}
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	const NK::ContextConfig config{ loggerConfig, NK::ALLOCATOR_TYPE::TRACKING_VERBOSE };
	return config;
}



[[nodiscard]] NK::EngineConfig CreateEngine()
{
	const NK::EngineConfig config{ NK_NEW(GameApp) };
	return config;
}