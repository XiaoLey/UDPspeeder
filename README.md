# UDPspeeder

![](/images/en/udpspeeder.PNG)

UDPspeeder is a bilateral network accelerator. By itself it accelerates UDP traffic, but when combined with a VPN it can accelerate all traffic (including TCP/UDP/ICMP). With well-tuned parameters it can accelerate games by lowering their packet loss and latency, and it can also speed up high-bandwidth applications such as downloading and video streaming. Spending 1.5x the bandwidth, it can bring a 10% packet loss rate down to less than 0.01%. Compared to existing solutions such as kcptun/finalspeed/BBR, its main advantage is that it accelerates UDP and ICMP — nearly all existing solutions can only accelerate TCP.

I have been using it steadily for months to accelerate Brawl Stars (US server) and Mobile Legends (Asia server), and it works well — before acceleration the lag made both games nearly unplayable, and afterwards I barely notice any lag. Video streaming runs at close to full speed too.

The current version is v2, which adds FEC support on top of v1 and uses bandwidth more efficiently. If you are still using v1 (versions baked into router firmware are very likely v1), see the [v1 homepage](/doc/README.zh-cn.v1.md).

How it works with a VPN to accelerate all traffic (VPNs tested and confirmed to work: OpenVPN, L2TP, ShadowVPN):

![image0](/images/Capture2.PNG)

[简体中文](/doc/README.zh-cn.md)

[UDPspeeder Wiki](https://github.com/wangyu-/UDPspeeder/wiki)

# Efficacy
tested on a link with 100ms latency and 10% packet loss at both direction

### Ping Packet Loss
![](/images/en/ping_compare_mode1.png)

### SCP Copy Speed
![](/images/en/scp_compare2.PNG)

# Supported Platforms

Any Linux host — desktop Linux, Android phone/tablet, OpenWRT router, or Raspberry Pi. The releases include prebuilt binaries for `amd64`, `x86`, `arm`, `mips_be` and `mips_le`.

For Windows and macOS users, it runs stably inside a virtual machine (the speeder runs in Linux while your other applications keep running in Windows as usual; bridged mode has been tested and works). You can use [this](https://github.com/wangyu-/udp2raw-tunnel/releases/download/20171108.0/lede-17.01.2-x86_virtual_machine_image.zip) VM image, which is only 7.5mb in size and saves you the trouble of installing an OS in a VM. The image comes with an SSH server, so you can scp files into it, ssh into it, and copy-paste. The root password is 123456.

The Android version must be run from a terminal.

##### Note
When using a virtual machine, it is recommended to manually specify which network adapter to bridge to instead of leaving it on automatic, otherwise it may bridge to the wrong adapter.

# How does it work

The basic idea is to fight packet loss with redundant data. Two ways of sending redundant data are supported: FEC (Forward Error Correction) and plain multi-copy sending. The FEC algorithm used is Reed-Solomon.

How FEC works:

![image0](/images/en/fec.PNG)

### Reed-Solomon

`
In coding theory, the Reed–Solomon code belongs to the class of non-binary cyclic error-correcting codes. The Reed–Solomon code is based on univariate polynomials over finite fields.
`

`
It is able to detect and correct multiple symbol errors. By adding t check symbols to the data, a Reed–Solomon code can detect any combination of up to t erroneous symbols, or correct up to ⌊t/2⌋ symbols. As an erasure code, it can correct up to t known erasures, or it can detect and correct combinations of errors and erasures. Reed–Solomon codes are also suitable as multiple-burst bit-error correcting codes, since a sequence of b + 1 consecutive bit errors can affect at most two symbols of size b. The choice of t is up to the designer of the code, and may be selected within wide limits.
`

![](/images/en/rs.png)

Check wikipedia for more info, https://en.wikipedia.org/wiki/Reed–Solomon_error_correction

# Other Features

Packet contents and lengths are randomized (think of it as obfuscation), so a packet capture will not reveal that you are sending redundant data — no need to worry about your VPS getting banned.

A configurable delay can be inserted between the redundant copies to counter burst packet loss, so that a router whose buffer momentarily overflows does not drop every copy in a row.

A configurable amount of latency jitter can be simulated, which makes the RTT variance seen by upper-layer applications larger so that they wait for the redundant packets to arrive instead of triggering a retransmission before they get there.

Send/recv reports can be printed, from which you can read off the packet loss rate.

Packet loss, latency and jitter can be simulated, which is handy for experimenting to find out why an application stutters.

The client supports multiple UDP connections, and the server supports multiple clients.

# Getting Started

### Installing
Download the prebuilt binaries and extract them to any directory on your local machine and your server.

https://github.com/wangyu-/UDPspeeder/releases

### Running
Assume you have a server at 44.55.66.77, with a service listening on UDP port 7777, and you want to accelerate traffic from your local machine to 44.55.66.77:7777.

```
Run at server side:
./speederv2 -s -l0.0.0.0:4096 -r127.0.0.1:7777  -f20:10 -k "passwd" --mode 0

Run at client side:
./speederv2 -c -l0.0.0.0:3333 -r44.55.66.77:4096 -f20:10 -k "passwd" --mode 0
```

A tunnel is now established between the client and the server. To reach 44.55.66.77:7777, just connect to 127.0.0.1:3333 instead — all the UDP traffic in both directions will be accelerated.

##### Note

`-f20:10` means sending 10 redundant packets for every 20 original packets. Both `-f20:10` and `-f 20:10` work — the space may be omitted, as with all single-dash options. For double-dash options such as `--mode 0` below, the space is required.

`-k` specifies a string to enable simple XOR encryption.

Using `--mode 0` is recommended, otherwise you may have to care about MTU issues.

The parameters recommended here are for everyday, non-gaming use. For gaming, use the parameters recommended in [Usage Tips](https://github.com/wangyu-/UDPspeeder/wiki).

##### Tip

On some ISPs, combining UDPspeeder with udp2raw gives even better speed — udp2raw disguises UDP traffic as TCP to bypass UDP throttling by the ISP.

udp2raw repo:

https://github.com/wangyu-/udp2raw-tunnel

# Improves all traffic with OpenVPN + UDPspeeder

See [UDPspeeder + openvpn config guide](https://github.com/wangyu-/UDPspeeder/wiki/UDPspeeder-openvpn-config-guide).

# Advanced Topic

### Full Options
```
usage:
    run as client: ./this_program -c -l local_listen_ip:local_port -r server_ip:server_port  [options]
    run as server: ./this_program -s -l server_listen_ip:server_port -r remote_ip:remote_port  [options]

common options, must be same on both sides:
    -k,--key              <string>        key for simple xor encryption. if not set, xor is disabled
main options:
    -f,--fec              x:y             forward error correction, send y redundant packets for every x packets
    --timeout             <number>        how long could a packet be held in queue before doing fec, unit: ms, default: 8ms
    --report              <number>        turn on send/recv report, and set a period for reporting, unit: s
    --stats-interval      <number>        write a stats snapshot file every <number> s, 0 to disable, default: 0
    --stats-file          <string>        stats snapshot file path, default: /run/speederv2-<local_port>.stats
advanced options:
    --mode                <number>        fec-mode,available values: 0,1; mode 0(default) costs less bandwidth,no mtu problem.
                                          mode 1 usually introduces less latency, but you have to care about mtu.
    --mtu                 <number>        mtu. for mode 0, the program will split packet to segment smaller than mtu value.
                                          for mode 1, no packet will be split, the program just check if the mtu is exceed.
                                          default value: 1250. you typically shouldnt change this value.
    -j,--jitter           <number>        simulated jitter. randomly delay first packet for 0~<number> ms, default value: 0.
                                          do not use if you dont know what it means.
    -i,--interval         <number>        scatter each fec group to a interval of <number> ms, to defend burst packet loss.
                                          default value: 0. do not use if you dont know what it means.
    -f,--fec              x1:y1,x2:y2,..  similiar to -f/--fec above,fine-grained fec parameters,may help save bandwidth.
                                          example: "-f 1:3,2:4,10:6,20:10". check repo for details
    --random-drop         <number>        simulate packet loss, unit: 0.01%. default value: 0.
    --disable-obscure                     disable obscure, to save a bit bandwidth and cpu
    --disable-checksum                    disable checksum to save a bit bandwidth and cpu
developer options:
    --fifo                <string>        use a fifo(named pipe) for sending commands to the running program, so that you
                                          can change fec encode parameters dynamically, check readme.md in repository for
                                          supported commands.
    -j ,--jitter          jmin:jmax       similiar to -j above, but create jitter randomly between jmin and jmax
    -i,--interval         imin:imax       similiar to -i above, but scatter randomly between imin and imax
    -q,--queue-len        <number>        fec queue len, only for mode 0, fec will be performed immediately after queue is full.
                                          default value: 200. 
    --decode-buf          <number>        size of buffer of fec decoder,unit: packet, default: 2000
    --fix-latency                         try to stabilize latency, only for mode 0
    --delay-capacity      <number>        max number of delayed packets, 0 means unlimited, default: 0
    --disable-fec                         completely disable fec, turn the program into a normal udp tunnel
    --sock-buf            <number>        buf size for socket, >=10 and <=10240, unit: kbyte, default: 1024
    --out-addr            ip:port         force all output packets of '-r' end to go through this address, port 0 for random port.
    --out-interface       <string>        force all output packets of '-r' end to go through this interface.
log and help options:
    --log-level           <number>        0: never    1: fatal   2: error   3: warn 
                                          4: info (default)      5: debug   6: trace
    --log-position                        enable file name, function name, line number in log
    --disable-color                       disable log color
    -h,--help                             print this help message
```

### Packet sending options, can differ between sides

These options only affect how packets are sent locally.

##### `-f` option
Sets the FEC parameters, which control how much redundancy is added to the data.

##### `--timeout` option
Sets the maximum delay that the FEC encoder may introduce while encoding. A higher value makes FEC more effective; lowering it reduces latency at the cost of effectiveness.

##### `--mode` and `--mtu` options

In short: `--mode 0` costs less bandwidth and has no MTU issues; `--mode 1` can slightly lower latency but requires you to care about MTU. There is also a `--mode 0 -q1` recipe for multi-copy sending, which adds no delay and has no MTU issues — suitable for gaming, but it costs the most bandwidth.

For details, see https://github.com/wangyu-/UDPspeeder/wiki/mode和mtu选项

If you are new to this, don't obsess over the exact meaning of these parameters — just use the settings recommended in `Usage Tips` and don't change parameters randomly, especially not `--mtu`.

##### `--report` option
Send/recv report. With it enabled you can use the data to estimate characteristics like packet rate and packet loss rate. `--report 10` means generating a report every 10 seconds.

##### `--stats-interval` and `--stats-file` options
Write the FEC decode counters to a snapshot file for an external controller to read. `--stats-interval 5` rewrites the file every 5 seconds; `0` (the default) disables it. `--stats-file` sets the path, defaulting to `/run/speederv2-<local_port>.stats`.

The file is rewritten atomically (write to a temp file, then rename), so a reader never sees a half-written file. It contains only `key=value` lines:

```
ts=1721836800
fec='1:1,10:2,20:4,40:8,80:16,100:20'
interval='5:10'
up_expected=12345	up_recovered=40	up_lost=5
```

`expected` = data packets that should have arrived, `recovered` = lost but recovered by FEC, `lost` = lost and unrecoverable. Identity: `expected = received + recovered + lost`. The counters are cumulative (monotonically increasing); the controller diffs two readings taken N seconds apart to get the loss/recovery over that interval.

Two things to note:

- Direction is split across files: each process decodes only one direction — the client's file has only `dn_*` (server→client), the server's file has only `up_*` (client→server). To see both directions, read the file on each side (keyed by its local port).
- `lost` lags: a lost group is only counted when its slot in the decoder's ring buffer (`--decode-buf`, default 2000 packets) is recycled by later traffic. Under steady traffic it converges quickly; right after traffic stops, the last few incomplete groups are counted only when new packets arrive.

##### `-i` option
Specifies a time window of n milliseconds. The packets of one FEC group are spread evenly across those n milliseconds when being sent, which defends against burst packet loss. The default value is 0 (i.e. the feature is off). This feature is quite useful — if the recommended parameters don't work well, try turning it on, e.g. `-i 10` or `-i 20`. This option works on roughly the same principle as what is usually called `interleaved FEC` in communication theory.

##### `-j` option
Adds latency jitter to the sending of original packets. This makes the RTT variance seen by upper-layer applications larger, so that they wait for the redundant packets to arrive instead of triggering a retransmission before the redundant packets get there. Under normal circumstances a cross-border network already has plenty of latency jitter of its own, so you usually don't need to set `-j`.

The `-j` option can simulate latency itself, not just jitter.

##### `--random-drop` option
Randomly drops packets. Used to simulate a high-loss network environment. Combined with the `-j` option, it can simulate a network with high latency (or high latency jitter) and high packet loss, which is useful for testing how FEC parameters behave under various network conditions.

##### `-q` option
Only useful in mode 0. Sets the maximum queue length of the FEC encoder. For example, `-q5` means: as soon as 5 data packets have accumulated in the encoder, send them immediately. Used well, it can improve latency. The `Usage Tips` section below describes using `--mode 0 -q1` for multi-copy sending.

`-q` and `--timeout` serve a similar purpose. `-q` decides how many data packets the FEC encoder may accumulate before sending immediately; `--timeout` decides how many milliseconds the encoder may wait at most after receiving the first packet before sending.

The default value is 200, i.e. accumulate as many packets as possible.

Don't tune this parameter yourself unless you are using it in the form recommended in `Usage Tips`.

#### `--fifo` option
Use a fifo (named pipe) to send commands to the running program. For example, with `--fifo fifo.file` the following commands are available:
```
echo fec 19:9 > fifo.file
echo mtu 1100 > fifo.file
echo timeout 5 > fifo.file
echo queue-len 100 > fifo.file
echo mode 0 > fifo.file
echo interval 5 > fifo.file
echo interval 3:8 > fifo.file
echo jitter 5 > fifo.file
echo jitter 3:8 > fifo.file
```
This lets you change running parameters on the fly. Check the program log to see whether a command was sent successfully.

### Options that must be the same on both sides

##### `-k` option
Specifies a string. Every packet sent and received between the server and client is XORed with it, which changes the protocol signature and keeps ISPs from targeting the UDPspeeder protocol.

##### `--disable-obscure`
By default UDPspeeder randomly pads and XORs a few bytes (4–32 bytes) into each outgoing packet, which makes it hard to tell from a packet capture that you are sending redundant data and keeps your VPS from getting banned. This feature is just an extra precaution — it is generally fine to turn it off, and doing so saves a bit of bandwidth and CPU. `--disable-obscure` turns it off.

# Recommended Parameters

https://github.com/wangyu-/UDPspeeder/wiki

# Usage Tips

https://github.com/wangyu-/UDPspeeder/wiki

# Build Guide

For now, refer to udp2raw's build guide — the process is nearly identical (note that the guide is written in Chinese).

https://github.com/wangyu-/udp2raw-tunnel/blob/master/doc/build_guide.zh-cn.md

# Wiki

Check the wiki for more info:

https://github.com/wangyu-/UDPspeeder/wiki

# Related Repos

You can also try tinyfecVPN, a lightweight high-performance VPN with UDPspeeder's function built-in, repo:

https://github.com/wangyu-/tinyfecVPN

You can use udp2raw with UDPspeeder together to get better speed on some ISP with UDP QoS(UDP throttling), repo:

https://github.com/wangyu-/udp2raw-tunnel
