#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <string>
#include <cstdint>
#include <vector>
#include <queue>
#include <memory>

struct EndpointData;

class Endpoint
{
private:
    std::string host;
    uint16_t port;
    std::shared_ptr<EndpointData> data;

public:
    const static uint16_t AnyPort = 0;
    Endpoint();
    Endpoint(const std::string& host, uint16_t port);

    bool operator==(const Endpoint& rhs) const;

    std::shared_ptr<EndpointData> GetData() const;

    const std::string& GetHost() const { return host; }
    uint16_t GetPort() const  { return port; }

    friend bool operator<(const Endpoint& lhs, const Endpoint& rhs);
};

bool operator<(const Endpoint& lhs, const Endpoint& rhs);

class NetMessage;
typedef std::unique_ptr<NetMessage> NetMessagePtr;

class NetMessage
{
private:
    Endpoint recipient;
    Endpoint sender;
    std::vector<uint8_t> data;
    NetMessage(const Endpoint& sender);

public:
    static NetMessagePtr Create(const Endpoint& sender);
    virtual ~NetMessage() = default;

    NetMessage Reply();

    void SetRecipient(const Endpoint& ep);
    void SetSender(const Endpoint& ep);
    const Endpoint& GetRecipient(void) const;
    const Endpoint& GetSender(void) const;
    void SetData(const void* data, uint32_t size);
    void SetData(std::vector<uint8_t>& data);
    const std::vector<uint8_t>& GetData() const;

    template<typename DataType>
    void AddData(const DataType& field)
    {
        data.insert(data.end(), &field, static_cast<uint8_t*>(&field) + sizeof(field));
    }
protected:
    friend class Mailbox;
};

class Mailbox;
typedef std::shared_ptr<Mailbox> MailboxPtr;

class Mailbox
{
private:
    Endpoint id;
    std::queue<NetMessagePtr> messages;
    int32_t sock;
    bool isValid;
    uint32_t maxMessageSize;

    Mailbox(const Endpoint& endpoint, uint32_t max_message_size);

public:
    static MailboxPtr Create(const Endpoint& endpoint, uint32_t max_message_size = 1024);
    ~Mailbox();

    static int32_t UpdateMailboxes(std::vector<MailboxPtr>& boxes, int32_t timeout = -1);

    uint32_t GetMessageCount() const;
    NetMessagePtr GetMessage();
    bool SendMessage(NetMessagePtr& message);

    Mailbox(const Mailbox& other) = delete;
    Mailbox& operator= (const Mailbox& other) = delete;

    int32_t GetSocket();

    bool GetIsValid() const;
};


#endif // NETWORK_H_INCLUDED
