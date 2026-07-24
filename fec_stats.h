/*
 * fec_stats.h
 *
 * FEC 解码侧累计计数与快照输出
 */

#ifndef FEC_STATS_H_
#define FEC_STATS_H_

#include "common.h"

// FEC 解码侧累计计数，只增不减，供外部控制器读取后自行 diff
// 恒等式: expected = 实收 + recovered + lost
struct fec_decode_stat_t {
    u64_t expected = 0;   // 应到的数据包
    u64_t recovered = 0;  // 丢了但被 FEC 救回的
    u64_t lost = 0;       // 丢了且救不回的
};

extern fec_decode_stat_t fec_decode_stat;

extern int stats_interval;     // 快照生成周期(秒), 0=关闭(默认)
extern char stats_file[1000];  // 快照文件路径, 留空则用缺省名

// 一个 FEC 组解码成功终结时调用
// x 为该组数据包总数(data_num)，x_got 为实际收到的数据包数
void fec_stat_on_recovered(int x, int x_got);

// 一个 FEC 组未解码成功即终结(被挤出/解码出垃圾)时调用
// 参数含义同上
void fec_stat_on_lost(int x, int x_got);

// stats_interval>0 时在 loop 上启动周期定时器，覆盖写原子快照文件
// 快照只含 key=value 行，按 program_mode 输出本进程解码的方向(client=dn, server=up)
void stats_snapshot_init(struct ev_loop *loop);

// 立即写一次快照(定时器周期到时内部也调用它)
void stats_snapshot_write();

#endif /* FEC_STATS_H_ */
