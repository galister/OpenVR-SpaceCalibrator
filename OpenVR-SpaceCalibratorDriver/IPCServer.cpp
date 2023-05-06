#include "IPCServer.h"
#include "Logging.h"
#include "ServerTrackedDeviceProvider.h"

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void IPCServer::HandleRequest(const protocol::Request &request, protocol::Response &response)
{
	switch (request.type)
	{
	case protocol::RequestHandshake:
		response.type = protocol::ResponseHandshake;
		response.protocol.version = protocol::Version;
		break;

	case protocol::RequestSetDeviceTransform:
		driver->SetDeviceTransform(request.setDeviceTransform);
		response.type = protocol::ResponseSuccess;
		break;

	default:
		LOG("Invalid IPC request: %d", request.type);
		break;
	}
}

IPCServer::~IPCServer()
{
	Stop();
}

void IPCServer::Run()
{
	int fds[2];
	int result = pipe(fds);
	if (result == -1) {
		LOG("Could not create stop pipe: %s", strerror(errno));
		return;
	}
	stopRead = fds[0];
	stopWrite = fds[1];
	mainThread = std::thread(RunThread, this);
}

void IPCServer::Stop()
{
	TRACE("IPCServer::Stop()");
	if (!running)
		return;

	stop = true;
	mainThread.join();
	running = false;
	write(stopWrite, &stop, 1);
	close(stopRead);
	close(stopWrite);
	TRACE("IPCServer::Stop() finished");
}

void IPCServer::RunThread(IPCServer *_this)
{
	_this->running = true;

	int serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (serverSocket == -1) {
		LOG("socket failed in RunThread. Error: %s", strerror(errno));
		return;
	}
	std::string socketPath = std::string(getenv("XDG_RUNTIME_DIR")) + "/openvr-spacecal.sock";
	unlink(socketPath.c_str());
	struct sockaddr_un serverAddress;
	memset(&serverAddress, 0, sizeof(struct sockaddr_un));
	serverAddress.sun_family = AF_UNIX;
	strncpy(serverAddress.sun_path, socketPath.data(), socketPath.size());
	int result = bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
	if (result == -1)
	{
		close(serverSocket);
		LOG("bind failed in RunThread. Error: %s", strerror(errno));
		return;
	}
	result = listen(serverSocket, 5);
	if (result == -1)
	{
		close(serverSocket);
		LOG("listen failed in RunThread. Error: %s", strerror(errno));
		return;
	}

	while (!_this->stop)
	{
		if (_this->stop)
		{
			break;
		}

		int clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == -1) {
			LOG("accept failed in RunThread. Error: %s", strerror(errno));
			continue;
		}

		LOG("IPC client connected");

		struct pollfd fds[2];
		fds[0].fd = _this->stopRead;
		fds[0].events = POLLIN | POLLRDBAND;
		fds[1].fd = clientSocket;
		fds[1].events = POLLIN | POLLRDBAND;

		while (poll(fds, 2, -1) != -1) {
			if (fds[0].revents != 0) {
				break;
			}
			if (fds[1].revents == 0) {
				continue;
			}
			protocol::Request request;
			ssize_t bytesRead = read(clientSocket, &request, sizeof(request));
			if (bytesRead != sizeof(request)) {
				LOG("IPC client disconnecting due to error (via CompletedWriteCallback), error: %s, bytesRead: %zd", strerror(errno), bytesRead);
				break;
			}
			protocol::Response response;
			_this->HandleRequest(request, response);
			ssize_t bytesWritten = write(clientSocket, &response, sizeof(response));
			if (bytesWritten != sizeof(response)) {
				if (bytesWritten == -1 && errno == ECONNRESET) {
					LOG("IPC client disconnecting normally");
				} else {
					LOG("IPC client disconnecting due to error (via CompletedReadCallback), error: %s, bytesWritten: %zd", strerror(errno), bytesWritten);
				}
				break;
			}
		}
	}

	close(serverSocket);
}
