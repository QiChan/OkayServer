#include "client.h"

extern "C"
{
#include "skynet.h"
#include "skynet_server.h"
}

using namespace std;

extern "C"
{
	static int robot_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		Client* client = (Client*)ud;
        client->ServicePoll((const char*)msg, sz, source, session, type);
		return 0;
	}

	Client* robot_create()
	{
		Client* client = new Client();
		return client;
	}

	void robot_release(Client* client)
	{
		delete client;
	}

	int robot_init(Client* client, struct skynet_context* ctx, char* parm)
	{
		
		skynet_callback(ctx, client, robot_cb);

		if (!client->VInitService(ctx, parm, strlen(parm)))
        {
            return -1;
        }

		return 0;
	}
}





