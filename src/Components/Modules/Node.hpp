#pragma once

#define NODE_HALFLIFE (3 * 60 * 1000) //3min
#define NODE_MAX_NODES_TO_SEND 64
#define NODE_SEND_RATE 500ms

#ifdef NODE_LOG_MESSAGES
#define NODE_LOG(x, ...) Logger::Print(x, __VA_ARGS__)
#else
#define NODE_LOG(x, ...)
#endif

namespace Components
{
	class Node : public Component
	{
	public:
		class Data
		{
		public:
			uint64_t protocol;
		};

		class Entry
		{
		public:
			Network::Address address;
			Data data;

			std::optional<Utils::Time::Point> lastRequest;
			std::optional<Utils::Time::Point> lastResponse;

			bool isValid();
			bool isDead();

			bool requiresRequest();
			void sendRequest();

			void reset();
			json11::Json to_json() const;
		};

		Node();
		~Node();

		static void Add(Network::Address address);
		static std::vector<Entry> GetNodes();
		static void RunFrame();
		static void Synchronize();

		static void LoadNodeRemotePreset();

	private:
		static std::recursive_mutex Mutex;
		static std::vector<Entry> Nodes;
		static bool wasIngame;

		static void HandleResponse(Network::Address address, const std::string& data);

		static void SendList(Network::Address address);

		static void LoadNodePreset();
		static void LoadNodes();
		static void StoreNodes(bool force);

		static unsigned short GetPort();
	};
}
