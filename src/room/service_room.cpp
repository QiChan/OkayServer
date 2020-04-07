#include "cash_room.h"
#include <arpa/inet.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
	static int room_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		RoomShell* shell = (RoomShell*)ud;
        if (shell == nullptr || shell->room_ == nullptr)
        {
            return -1;
        }

		shell->room_->ServicePoll((const char*)msg, sz, source, session, type);
		return 0; //0表示成功
	}

	RoomShell* room_create()
	{
		RoomShell* shell = new RoomShell();
		return shell;
	}

	void room_release(RoomShell* shell)
	{
		delete shell;
	}

	int room_init(RoomShell* shell, struct skynet_context* ctx, char* parm)// parm: [数据长度] [类型长度] [类型名] [数据]
	{
		if (parm == nullptr)
			return -1;

		char* c = (char*)parm;

		uint32_t parml = *(uint32_t*)c;
		parml = ntohl(parml);

		InPack pack;
		pack.ParseFromNetData(PackContext(), c + PACK_HEAD, parml);
		MessagePtr data = pack.CreateMessage();
		if (data == nullptr)
		{
			LOG(ERROR) << "room init parm error";
			return -1;
		}
		auto msg = std::dynamic_pointer_cast<pb::iCreateRoomREQ>(data);

        std::shared_ptr<Room>   room = nullptr;
        if (msg->req().room_mode() == pb::CLUB_CASH_ROOM)
        {
            room = std::make_shared<CashRoom>();
        }

        if (room == nullptr)
        {
            LOG(ERROR) << "create room failed. " << msg->ShortDebugString();
            return -1;
        }

        shell->room_ = room;
        room->SetCreateParam(*msg);
        if (!room->VInitService(ctx, nullptr, 0))
        {
            return -1;
        }

		skynet_callback(ctx, shell, room_cb);
        return 0;
	}
}
