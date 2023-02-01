# monprobe-pub
Multichannel mpeg-ts analizer

linux multicast setup:
--------------------------
/etc/sysctl.conf

net.ipv4.conf.[eth0].force_igmp_version=2

net.ipv4.conf.[eth0].rp_filter=0

net.core.rmem_max = 1048576

net.core.rmem_default=1048576

net.ipv4.udp_mem = 8388608 12582912 16777216

sudo sysctl -p


/etc/rc.local

route add -net 224.0.0.0/16 dev [eth0]

route add -net 239.255.0.0/16 dev [eth0]

--------------------------


build for linux x86-64:

g++ -std=gnu++14      -Wall -Wextra -Wshadow -Wformat=2 -Wlogical-op -O2 -I ./ -o monprobe main.cpp tmain.cpp tglobal.cpp tconfig.cpp ttime.cpp tdecode.cpp thread_rcv_af.cpp tini.cpp trawes.cpp trawts.cpp tinterface.cpp tanalizeres.cpp tanalizerts.cpp tstatistic.cpp theader.cpp tchannel.cpp tchannelmeasure.cpp tui.cpp thttp.cpp twsconnection.cpp sha256.cpp taccount.cpp tlog.cpp ES_AAC.cpp ES_AC3.cpp ES_h264.cpp ES_hevc.cpp ES_MPEGAudio.cpp ES_MPEGVideo.cpp ES_Subtitle.cpp ES_Teletext.cpp bitstream.cpp -s -Bstatic -L./lib64 -lcivetweb19 -ldl -static-libstdc++ -pthread 

build for linux arm (RPi4):

arm-linux-gnueabihf-g++ -mcpu=cortex-a72 -mtune=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits -std=gnu++14 -Wall -Wextra -Wshadow -Wformat=2 -Wlogical-op -O2 -I ./ -o mon96arm main.cpp tmain.cpp tglobal.cpp tconfig.cpp ttime.cpp tdecode.cpp thread_rcv_af.cpp tini.cpp trawes.cpp trawts.cpp tinterface.cpp tanalizeres.cpp tanalizerts.cpp tstatistic.cpp theader.cpp tchannel.cpp tchannelmeasure.cpp tui.cpp thttp.cpp twsconnection.cpp sha256.cpp taccount.cpp tlog.cpp ES_AAC.cpp ES_AC3.cpp ES_h264.cpp ES_hevc.cpp ES_MPEGAudio.cpp ES_MPEGVideo.cpp ES_Subtitle.cpp ES_Teletext.cpp bitstream.cpp -s -Bstatic -L./lib64 -lcivetweb19arm -ldl -static-libstdc++ -pthread 

or use cmake



