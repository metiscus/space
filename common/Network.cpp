#include "Network.h"
#include <cassert>
#include <cstdio>
#include <cstring>

#ifdef WIN32

#elif defined(__linux__) || defined(__unix__)
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

struct EndpointData
{
    struct sockaddr_in address;
};

//////////[ Endpoint ]//////////
Endpoint::Endpoint()
    : host("")
    , port(0u)
    , data(new EndpointData())
{
    memset(&data->address, 0, sizeof(data->address));
}

Endpoint::Endpoint(const std::string& host, uint16_t port)
{
    this->host = host;
    this->port = port;
    inet_pton(AF_INET, host.c_str(), &data->address.sin_addr);
}

bool Endpoint::operator==(const Endpoint& rhs) const
{
    return (host == rhs.host && port == rhs.port);
}

std::shared_ptr<EndpointData> Endpoint::GetData() const
{
    return data;
}

//////////[ NetMessage ]//////////
NetMessage::NetMessage(const Endpoint& sender)
{
    this->sender = sender;
}

NetMessage NetMessage::Reply()
{
    NetMessage message (this->recipient);
    message.SetRecipient(this->sender);
    return message;
}

void NetMessage::SetRecipient(const Endpoint& ep)
{
    this->recipient = ep;
}

void NetMessage::SetSender(const Endpoint& ep)
{
    this->sender = ep;
}

const Endpoint& NetMessage::GetRecipient(void) const
{
    return recipient;
}

const Endpoint& NetMessage::GetSender(void) const
{
    return sender;
}

void NetMessage::SetData(const uint8_t* data, uint32_t size)
{
    this->data.reserve(this->data.size() + size);
    this->data.clear();
    this->data.insert(this->data.end(), data, data + size);
}

void NetMessage::SetData(std::vector<uint8_t>& data)
{
    this->data.reserve(data.size());
    this->data.clear();
    this->data = std::move(data);
}

const std::vector<uint8_t>& NetMessage::GetData() const
{
    return data;
}

//////////[ Mailbox ]//////////
Mailbox::Mailbox(const Endpoint& endpoint, uint32_t max_message_size)
    : id(endpoint)
    , sock(-1)
    , isValid(false)
    , maxMessageSize(max_message_size)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    int32_t err = bind(sock, (const sockaddr*)&endpoint.GetData()->address, sizeof(endpoint.GetData()->address));
    if(err == -1)
    {
        fprintf(stderr, "Mailbox failed to bind to endpoint. (%s)\n", strerror(errno));
        close(sock);
        sock = -1;
    }
    else
    {
        isValid = true;
    }

    assert(sock != -1);
}

Mailbox::~Mailbox()
{
    close(sock);
    sock = -1;
}

uint32_t Mailbox::GetMessageCount() const
{
    return messages.size();
}

NetMessagePtr Mailbox::GetMessage()
{
    NetMessagePtr ret;
    if(messages.size() > 0)
    {
        // This is a little tricky since we are using a unique_ptr. In order
        // to safely remove the message, we have to use std::move to convert
        // the l-value returned by .front() into an r-value suitable for
        // assignment. Also the unique_ptr on the front of the queue is still
        // there but invalid. We still have to .pop()
        ret = std::move(messages.front());
        messages.pop();
    }
    return ret;
}

bool Mailbox::SendMessage(NetMessagePtr& message)
{
    assert(sock != -1);
    uint32_t ret = sendto(sock, (void*)&message->GetData()[0], message->GetData().size(), 0,
        (const sockaddr*)&message->GetRecipient().GetData()->address, sizeof(struct sockaddr_in));
    if(ret != message->GetData().size())
    {
        fprintf(stderr, "Failed to send message. %s\n", strerror(errno));
        return false;
    }
    return true;
}

int32_t Mailbox::GetSocket()
{
    return sock;
}

bool Mailbox::GetIsValid() const
{
    return isValid;
}

int32_t Mailbox::UpdateMailboxes(std::vector<MailboxPtr>& boxes, int32_t timeout)
{
    fd_set read_set;
    fd_set error_set;

    // As per POSIX fd_set is fixed size and can not exceed FD_SETSIZE
    assert(boxes.size() < FD_SETSIZE);

    int32_t highest_fd = -1;

    FD_ZERO(&read_set);
    FD_ZERO(&error_set);
    for(auto& box : boxes)
    {
        if(box->GetSocket() > highest_fd)
        {
            highest_fd = box->GetSocket();
        }

        FD_SET(box->GetSocket(), &read_set);
        FD_SET(box->GetSocket(), &error_set);
    }

    int32_t updated = 0;
    if(timeout == -1)
    {
        updated = select(highest_fd, &read_set, nullptr, &error_set, nullptr);
    }
    else{
        struct timeval tv;
        tv.tv_sec  = timeout / (int32_t)1e6;
        tv.tv_usec = timeout % (int32_t)1e6;
        updated = select(highest_fd, &read_set, nullptr, &error_set, &tv);
    }

    for(auto& box : boxes)
    {
        //check for read
        if(FD_ISSET(box->GetSocket(), &read_set) == 1)
        {
            // process a read
            NetMessagePtr message(new NetMessage(box->id));
            std::vector<uint8_t> data(box->maxMessageSize);
            struct sockaddr_in remote;
            socklen_t length = sizeof(remote);
            int32_t result = recvfrom(box->sock, &data[0], box->maxMessageSize, 0, (sockaddr*)&remote, &length);
            if(result != -1)
            {
                const int addr_size = 100;
                char remote_addr_str[addr_size];
                inet_ntop(AF_INET, &remote.sin_addr, (char*)remote_addr_str, addr_size);

                Endpoint sender(std::string(remote_addr_str), ntohs(remote.sin_port));
                message->SetSender(sender);
            }
            else
            {
                fprintf(stderr, "Failed to receive message.\n");
            }
        }
        else if(FD_ISSET(box->GetSocket(), &error_set) == 1)
        {
            // connection dropped out
            box->isValid = false;
        }
    }

    return updated;
}
