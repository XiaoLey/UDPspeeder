/*
 * fec_stats.cpp
 *
 * FEC 解码侧累计计数与快照输出
 */

#include "fec_stats.h"
#include "misc.h"
#include "log.h"

fec_decode_stat_t fec_decode_stat;

int stats_interval = 0;
char stats_file[1000] = "";

static ev_timer stats_timer;

void fec_stat_on_recovered(int x, int x_got) {
    if (x < 1) return;
    assert(x_got >= 0 && x_got <= x);
    fec_decode_stat.expected += x;
    fec_decode_stat.recovered += (x - x_got);
}

void fec_stat_on_lost(int x, int x_got) {
    if (x < 1) return;
    if (x_got < 0) x_got = 0;
    if (x_got > x) x_got = x;
    fec_decode_stat.expected += x;
    fec_decode_stat.lost += (x - x_got);
}

// 先写同目录临时文件再 rename，避免控制器读到写一半的快照
static int write_snapshot_atomic(const char *path, const char *content) {
    char tmp[sizeof(stats_file) + 16];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        mylog(log_warn, "open stats tmp file %s failed, errno=%s\n", tmp, get_sock_error());
        return -1;
    }
    int len = (int)strlen(content);
    int ret = (int)write(fd, content, len);
    close(fd);
    if (ret != len) {
        mylog(log_warn, "write stats tmp file %s failed\n", tmp);
        unlink(tmp);
        return -1;
    }
    if (rename(tmp, path) != 0) {
        mylog(log_warn, "rename %s to %s failed, errno=%s\n", tmp, path, get_sock_error());
        unlink(tmp);
        return -1;
    }
    return 0;
}

void stats_snapshot_write() {
    if (stats_file[0] == 0) return;

    // 本进程解码的方向: client 解下载(dn), server 解上传(up)
    const char *prefix = (program_mode == client_mode) ? "dn" : "up";

    // 预留足够空间容纳完整 fec 串(rs_par_str 上界为 rs_str_len)
    char buf[rs_str_len + 512];
    snprintf(buf, sizeof(buf),
             "ts=%llu\n"
             "fec='%s'\n"
             "interval='%d:%d'\n"
             "%s_expected=%llu\t%s_recovered=%llu\t%s_lost=%llu\n",
             (unsigned long long)time(NULL),
             rs_par_str,
             output_interval_min / 1000, output_interval_max / 1000,
             prefix, (unsigned long long)fec_decode_stat.expected,
             prefix, (unsigned long long)fec_decode_stat.recovered,
             prefix, (unsigned long long)fec_decode_stat.lost);

    write_snapshot_atomic(stats_file, buf);
}

static void stats_timer_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents) {
    assert(!(revents & EV_ERROR));
    stats_snapshot_write();
}

void stats_snapshot_init(struct ev_loop *loop) {
    if (stats_interval <= 0) return;

    // 缺省文件名按本地端口生成
    if (stats_file[0] == 0) {
        snprintf(stats_file, sizeof(stats_file), "/run/speederv2-%u.stats", local_addr.get_port());
    }

    // 先写一次，避免控制器在第一个周期内读不到文件
    stats_snapshot_write();

    ev_init(&stats_timer, stats_timer_cb);
    ev_timer_set(&stats_timer, stats_interval, stats_interval);
    ev_timer_start(loop, &stats_timer);

    mylog(log_info, "stats snapshot: file=%s interval=%ds\n", stats_file, stats_interval);
}
