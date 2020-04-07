#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <glog/logging.h>
#include "../common/ali_yun_log.h"

struct glogger
{
};

extern "C"
{
#include "skynet.h"
#include "skynet_env.h"

    struct glogger* glogger_create(void)
    {
        struct glogger* inst = (glogger*)skynet_malloc(sizeof(*inst));
        return inst;
    }

    void glogger_release(struct glogger* inst)
    {
        skynet_free(inst);
    }

    static int _logger(struct skynet_context* context, void *ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        char temp[100];
        if (source != 0)
        {
            sprintf(temp, "[%08x] ", source);
            LOG(INFO) << temp << (const char*)msg;
        }
        else
        {
            LOG(INFO) << (const char*)msg;
        }
        return 0;
    }

    int glogger_init(struct glogger* inst, struct skynet_context *ctx, const char* parm)
    {
        const char* dir = skynet_getenv("log_dir");
        const char* name = skynet_getenv("log_name");
        if (strcmp(name, "stdout") == 0)
        {
            FLAGS_logtostderr = true;   // 输出到控制台，不输出到文件
        }
        int logbufsecs = atoi(skynet_getenv("logbufsecs"));
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = logbufsecs;
        FLAGS_max_log_size = 10;    // 单位M
        google::InitGoogleLogging(name);
        log_producer_post_logs();
        skynet_callback(ctx, inst, _logger);
        skynet_command(ctx, "REG", ".logger");
        return 0;
    }
}
