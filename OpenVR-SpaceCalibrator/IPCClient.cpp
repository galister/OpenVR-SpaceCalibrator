#include "IPCClient.h"

#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

IPCClient::~IPCClient()
{
	if (fd != -1)
		close(fd);
}

void IPCClient::Connect()
{
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		throw std::runtime_error("Could not create socket");
	}
	std::string socketPath = std::string(getenv("XDG_RUNTIME_DIR")) + "/openvr-spacecal.sock";
	struct sockaddr_un address;
	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, socketPath.data(), socketPath.size());
	int result = connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
	if (result == -1)
	{
		throw std::runtime_error("Space Calibrator driver unavailable. Make sure SteamVR is running, and the Space Calibrator addon is enabled in SteamVR settings.");
	}

	auto response = SendBlocking(protocol::Request(protocol::RequestHandshake));
	if (response.type != protocol::ResponseHandshake || response.protocol.version != protocol::Version)
	{
		throw std::runtime_error(
			"Incorrect driver version installed, try reinstalling OpenVR-SpaceCalibrator. (Client: " +
			std::to_string(protocol::Version) +
			", Driver: " +
			std::to_string(response.protocol.version) +
			")"
		);
	}
}

protocol::Response IPCClient::SendBlocking(const protocol::Request &request)
{
	Send(request);
	return Receive();
}

void IPCClient::Send(const protocol::Request &request)
{
	ssize_t bytesWritten = write(fd, &request, sizeof request);
	if (bytesWritten == -1)
	{
		throw std::runtime_error(std::string("Error writing IPC request. Error: ") + strerror(errno));
	}
}

protocol::Response IPCClient::Receive()
{
	protocol::Response response(protocol::ResponseInvalid);
	ssize_t bytesRead = read(fd, &response, sizeof response);
	if (bytesRead == -1)
	{
		throw std::runtime_error(std::string("Error reading IPC response. Error: ") + strerror(errno));
	}

	if (bytesRead != sizeof response)
	{
		throw std::runtime_error("Invalid IPC response with size " + std::to_string(bytesRead));
	}

	return response;
}
