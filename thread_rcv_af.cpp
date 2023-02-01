#include "thread_rcv_af.h"



#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/mman.h>
#include <poll.h>

#include <sys/syscall.h>   // gettid

#include "tini.h"
#include "tlog.h"

#include "tinterface.h"
#include "tchannel.h"
#include "tanalizerts.h"








//==========================================================================
void thread_recieve_af(TInterface *pInterface)
{
    // **************************************************** LOW-LEVEL STRUCT
//http://tomoyo.osdn.jp/cgi-bin/lxr/source/Documentation/networking/packet_mmap.txt?v=linux-4.0.9
    /*
    https://gist.github.com/pavel-odintsov/15b7435e484134650f20
    Parser files:
    https://github.com/FastVPSEestiOu/fastnetmon/blob/master/src/fastnetmon_packet_parser.c
    https://github.com/FastVPSEestiOu/fastnetmon/blob/master/src/fastnetmon_packet_parser.h
    */



    struct  _in_hash {
              uint32_t ip_addr;
              uint16_t port;
    } __attribute__((packed));



/*
    struct _eth_hdr {
        uint8_t  dst_mac[6];
        uint8_t  src_mac[6];
        uint16_t eth_type;
    } __attribute__((packed));

    struct ethhdr {
        unsigned char h_dest[ETH_ALEN]; // destination eth addr
        unsigned char h_source[ETH_ALEN]; // source ether addr
        u_int16_t h_proto; // packet type ID field
    } __attribute__((packed));

    struct eth_vlan_hdr {
        u_int16_t h_vlan_id; // Tag Control Information (QoS, VLAN ID)
        u_int16_t h_proto;  // packet type ID field
    };
*/


    struct _ip_hdr {
        uint8_t  ip_verlen;        // 4-bit IPv4 version
                                   // 4-bit header length (in 32-bit words)
        uint8_t  ip_tos;           // IP type of service
        uint16_t ip_totallength;   // Total length
        uint16_t ip_id;            // Unique identifier
        uint16_t ip_offset;        // Fragment offset field
        uint8_t  ip_ttl;           // Time to live
        uint8_t  ip_protocol;      // Protocol(TCP,UDP etc)
        uint16_t ip_checksum;      // IP checksum
        uint32_t ip_srcaddr;       // Source address
        uint32_t ip_destaddr;      // Destination address
    } __attribute__((packed));

    struct _udp_hdr {
        uint16_t src_portno;       // Source port no.
        uint16_t dst_portno;       // Dest. port no.
        uint16_t udp_length;       // Udp packet length
        uint16_t udp_checksum;     // Udp checksum
    } __attribute__((packed));

    struct _block_desc {
        uint32_t version;
        uint32_t offset_to_priv;
        struct tpacket_hdr_v1 h1;
    };
    // **************************************************** LOW-LEVEL STRUCT


    if (!pInterface) { ExitWithError("thread recieve bad parameter"); }

    if (!pInterface->list_channel_for_timer.size())   {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: no channels";
        return;
        }



    //cpu core affinity
    //if (g.RootPermission()) {
        if (pInterface->cpu_core) {
            if (BindThisThreadToCore(pInterface->cpu_core) <0) {
                pInterface->is_error = 1;
                pInterface->err_message = "AF: BindThisThreadToCore fail";
                return;
                }
            if (BindIrqToCore(pInterface->cpu_core,pInterface->irq)<0){
                pInterface->is_error = 1;
                pInterface->err_message = "AF: BindIrqToCore fail";
                return;
                }
            }
       // }


    //================================== fill hash table

    static unsigned offset_hash=0; //for start position in interfaces
    uint16_t base_hash=0x1234;
    uint16_t hash_table[0xffff];

    unsigned ip_count_if = pInterface->list_channel_for_timer.size(); //total ip stream to interface



    bool fill_ok=true;
 //   for (unsigned i=0; i<100; i++) {
        base_hash++;
        memset(hash_table,0,sizeof(hash_table));
        fill_ok=true;

//Log.Scr("index_first=%u %s\n",index_first, pAnalizersArray[0]->TsIP().c_str());

        for (unsigned j=0;j<(ip_count_if);j++) {

            TChannel *pChannel= pInterface->list_channel_for_timer[j];

            _in_hash v;
            v.ip_addr    = pChannel->Info.IP().bin();
            v.port       = pChannel->Info.Port().bin();
            uint16_t idx = crc16(base_hash,(uint8_t *)&v,sizeof(v));  // index in hash_table



            //add to hash table
            if (hash_table[idx] == 0) {
                uint16_t val = offset_hash + ((j | 0x8000) & 0xffff); //index channel[key] (ip+port) | 0x8000 for j==0
                hash_table[idx] = val;

 //Log.Scr("FILL hash table[%u]=%u %s:%s\n",idx,val,pChannel->Info.IP().c_str(),pChannel->Info.Port().c_str());

                continue;
                }
            else {
                fill_ok = false;
                break; // to next hash salt
                }
            } // for channels
     //   } //next base hash

    if (fill_ok==false) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Filled hash table failed..(IP:PORT Doubled ?)";
        return;
        }

    offset_hash += ip_count_if;  // for next interfaces

    /*
    // CHECK
    TIPAddress ip_dst;
    TIPPort    port_dst;

    for (unsigned j=0;j<(ip_count_if);j++) {
        TChannel *pChannel= pInterface->list_channel_for_timer[j];

        ip_dst =pChannel->Info.IP();
        port_dst = pChannel->Info.Port();

        _in_hash v;
        v.ip_addr      = ip_dst.bin();
        v.port         = port_dst.bin();
        uint16_t idx   = crc16(base_hash,(uint8_t *)&v,sizeof(v));
        unsigned ch_idx = hash_table[idx];

        if (ch_idx) {
            uint16_t val = ch_idx & 0x7fff;
            printf("TEST hash table[%u]==%u dst=[%s:%u]\n",idx,val,ip_dst.c_str(),port_dst.bin());
            }
        }
*/

    //================================== hash table

    //int packet_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    int packet_socket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
    if (packet_socket == -1) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't create AF_PACKET socket";
        return;
        }

    int version = TPACKET_V3;
    int setsockopt_packet_version = setsockopt(packet_socket, SOL_PACKET, PACKET_VERSION, &version, sizeof(version));
     if (setsockopt_packet_version < 0) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't set packet v3 version";
        return;
        }

    int interface_number = g.GetInterfaceIndex(pInterface->ip_if.c_str());
    if (!interface_number) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't get interface number by interface name";
        return;
        }

    // Switch to MULTICAST mode
    struct packet_mreq sock_params;
    memset(&sock_params, 0, sizeof(sock_params));
    sock_params.mr_type = PACKET_MR_ALLMULTI;
    sock_params.mr_ifindex = interface_number;

    int set_multicast = setsockopt(packet_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, (void *)&sock_params, sizeof(sock_params));
    if (set_multicast == -1) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't enable multicast mode";
        return;
        }
    //
    // set size buffers
    //
    unsigned int blocksiz = 1 << 22;// 4194304 bytes
    unsigned int framesiz = 1 << 11;// 2048 bytes
    unsigned int blocknum = 64;

    struct tpacket_req3 req;
    memset(&req, 0, sizeof(req));

    req.tp_block_size   = blocksiz;
    req.tp_frame_size   = framesiz;
    req.tp_block_nr     = blocknum;
    req.tp_frame_nr     = (blocksiz * blocknum) / framesiz;

    req.tp_retire_blk_tov = 60; // Timeout in msec
    req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

    int setsockopt_rx_ring = setsockopt(packet_socket, SOL_PACKET , PACKET_RX_RING , (void*)&req , sizeof(req));
    if (setsockopt_rx_ring == -1) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't enable RX_RING for AF_PACKET socket";
        return;
        }

    // We use per thread structures
    uint8_t* mapped_buffer = nullptr;
    struct iovec* rd = nullptr;

    mapped_buffer = (uint8_t*)mmap(nullptr, req.tp_block_size * req.tp_block_nr, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, packet_socket, 0);
    if (mapped_buffer == MAP_FAILED) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: mmap failed";
        return;
        }

    // Allocate iov structure for each block
    rd = (struct iovec*)malloc(req.tp_block_nr * sizeof(struct iovec));
    // Initilize iov structures
    for (unsigned int i = 0; i < req.tp_block_nr; ++i) {
        rd[i].iov_base = mapped_buffer + (i * req.tp_block_size);
        rd[i].iov_len  = req.tp_block_size;
        }

    struct sockaddr_ll bind_address;
    memset(&bind_address, 0, sizeof(bind_address));

    bind_address.sll_family   = AF_PACKET;
    bind_address.sll_protocol = htons(ETH_P_ALL);
    bind_address.sll_ifindex  = interface_number;
    bind_address.sll_pkttype  = PACKET_HOST | PACKET_MULTICAST; //???

    int bind_result = bind(packet_socket, (struct sockaddr *)&bind_address, sizeof(bind_address));
    if (bind_result == -1) {
        pInterface->is_error = 1;
        pInterface->err_message = "AF: Can't bind to AF_PACKET socket";
        return;
        }

    //get pthread id
    pid_t tid = syscall(__NR_gettid);
    int fanout_group_id = tid & 0xffff; //fanout to thread

    if (fanout_group_id) {
        int fanout_type = PACKET_FANOUT_CPU;// PACKET_FANOUT_CPU - send packets to CPU where packet arrived
        int fanout_arg = (fanout_group_id | (fanout_type << 16));
        int setsockopt_fanout = setsockopt(packet_socket, SOL_PACKET, PACKET_FANOUT, &fanout_arg, sizeof(fanout_arg));
        if (setsockopt_fanout < 0) {
            pInterface->is_error = 1;
            pInterface->err_message = "AF: Can't configure fanout";
            return;
            }
        }

    unsigned int current_block_num = 0;

    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));

    pfd.fd      = packet_socket;
    pfd.events  = POLLIN | POLLERR;
    pfd.revents = 0;

    Log.ScrFile("AF: %s interface configured ok\n",pInterface->ip_if.c_str());


    // ok, running
    pInterface->is_running = 1;

    // check interface running flag
    while(pInterface->th_running) {

        //*** real timer 1 sec tick
        if(pInterface->flag_timer) {

            //================================== TIMER
            pthread_mutex_lock(&g.channels_lock);


            for (unsigned int i = 0; i < ip_count_if; i++) {
                pInterface->list_channel_for_timer[i]->Callback_Timer();
                //pChannelsArray[index_first + i]->Callback_Timer();
                }

            pInterface->flag_timer=0;
            ++g.channels_ready;     // atomic
            pthread_mutex_unlock(&g.channels_lock);
            pthread_cond_signal(&g.channels_cond_end);    // send signal -> data ready
            //================================== TIMER
            }

        struct _block_desc *pbd = (struct _block_desc *) rd[current_block_num].iov_base;

        if ((pbd->h1.block_status & TP_STATUS_USER) == 0) {
            poll(&pfd, 1, 100);  //-1); // миллисекунд
            continue;
            }

//unsigned status = pbd->h1.block_status;
//printf("\n%4x ",status);






        //блок принятых пакетов
        //================================== walk_block(base_hash,pbd, current_block_num);
        {
        int num_pkts = pbd->h1.num_pkts, i;
        unsigned long bytes = 0;
        struct tpacket3_hdr *ppd;

        ppd = (struct tpacket3_hdr *) ((uint8_t *) pbd + pbd->h1.offset_to_first_pkt);


        for (i = 0; i < num_pkts; ++i) {
            bytes += ppd->tp_snaplen;

            //if SOCK_DGRAM (OK)


            //--------------------------------
            // int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
            // ppd->tp_mac   (with SOCK_RAW)
            // ppd->tp_net   (with SOCK_DGRAM)

            // frame_ptr == ppd
            //struct tpacket_hdr* tphdr = (struct tpacket_hdr*)frame_ptr;

            //struct sockaddr_ll* addr = (struct sockaddr_ll*)(frame_ptr + TPACKET_HDRLEN - sizeof(struct sockaddr_ll));
            //uint8_t* l2content = (uint8_t *) ppd + ppd->tp_mac;
            //uint8_t* l3content = (uint8_t *) ppd + ppd->tp_net;
            //handle_frame(tphdr, addr, l2content, l3content);


           //_eth_hdr *eth_hdr = (_eth_hdr *) ((uint8_t *) ppd);

           //PrintX(l2content,12);
           //PrintX(l2content+12,4);



           //PrintX(l3content,32);
           //PrintX(eth_hdr->dst_mac,6);
           //PrintX(eth_hdr->src_mac,6);


           /*
    memcpy(&hdr->extended_hdr.parsed_pkt.dmac, eh->h_dest, sizeof(eh->h_dest));
    memcpy(&hdr->extended_hdr.parsed_pkt.smac, eh->h_source, sizeof(eh->h_source));

    hdr->extended_hdr.parsed_pkt.eth_type = ntohs(eh->h_proto);
    hdr->extended_hdr.parsed_pkt.offset.eth_offset = 0;
    hdr->extended_hdr.parsed_pkt.offset.vlan_offset = 0;
    hdr->extended_hdr.parsed_pkt.vlan_id = 0; // Any VLAN

    if (hdr->extended_hdr.parsed_pkt.eth_type == 0x8100 ) {// 802.1q (VLAN)
        struct eth_vlan_hdr* vh;
        hdr->extended_hdr.parsed_pkt.offset.vlan_offset = sizeof(struct ethhdr) - sizeof(struct eth_vlan_hdr);
        while (hdr->extended_hdr.parsed_pkt.eth_type == 0x8100) { // 802.1q (VLAN)

            hdr->extended_hdr.parsed_pkt.offset.vlan_offset += sizeof(struct eth_vlan_hdr);
            vh = (struct eth_vlan_hdr*)&pkt[hdr->extended_hdr.parsed_pkt.offset.vlan_offset];
            hdr->extended_hdr.parsed_pkt.vlan_id = ntohs(vh->h_vlan_id) & 0x0fff;
            hdr->extended_hdr.parsed_pkt.eth_type = ntohs(vh->h_proto);
            displ += sizeof(struct eth_vlan_hdr);
        }
    }
    */

            //--------------------------------



            _ip_hdr  *ip_hdr = (_ip_hdr *) ((uint8_t *) ppd + ppd->tp_mac);
           //_ip_hdr  *ip_hdr = (_ip_hdr *) ((uint8_t *) ppd + ppd->tp_net);



            //uint32_t tci = ppd->hv1.tp_vlan_tci;
            //uint16_t vid = ppd->hv1.tp_vlan_tpid;
            //Log.Scr("tci  %08x vlan %4x\n",tci,vid);

            //struct sockaddr_ll *pll = (sockaddr_ll *)((uint8_t *) ppd + sizeof(struct tpacket3_hdr));
            //PrintX(&pll->sll_addr[0],pll->sll_halen); // mac addr

            //uint16_t total       = bswap_16(ip_hdr->ip_totallength);
            //uint8_t  ip_protocol = ip_hdr->ip_protocol;      // Protocol(TCP,UDP etc)

            // ***************** tpacket3_hdr  GET  TIMESTAMP OK
            //uint64_t tcap = (uint64_t(ppd->tp_sec) * 1000000000)+ uint64_t(ppd->tp_nsec);

//printf("rtp len=%u\n",1);

            // UDP ?
            if (ip_hdr->ip_protocol == 0x11) {

                uint32_t ip_src      = bswap_32(ip_hdr->ip_srcaddr);
                uint32_t ip_dst      = bswap_32(ip_hdr->ip_destaddr);

                _udp_hdr *udp_hdr   = (_udp_hdr *)(ip_hdr+1);
                //uint16_t port_src    = bswap_16(udp_hdr->src_portno);
                uint16_t port_dst      = bswap_16(udp_hdr->dst_portno);
                uint16_t udp_length = bswap_16(udp_hdr->udp_length);

                uint16_t data_len  = udp_length-sizeof(_udp_hdr);
                uint8_t *data      = (uint8_t *)(udp_hdr+1);

                _in_hash v;
                v.ip_addr      = ip_dst;
                v.port         = port_dst;
                uint16_t idx   = crc16(base_hash,(uint8_t *)&v,sizeof(v));
                unsigned ch_idx = hash_table[idx];

//printf("udp len=%u\n",data_len);


                if (ch_idx) {

//TIPAddress d = ip_dst;
//TIPAddress s = ip_src;
//printf("idx=%u dst=[%s:%u] src=[%s]\n",ch_idx,d.c_str(),port_dst,s.c_str());
//printf("idx=%u dst=[%s:%u] src=[%s]\n",ch_idx,d.c_str(),port_dst,s.c_str());

                    pChannelsArray[ch_idx & 0x7fff]->Callback_Parser(data,data_len,ip_src);
                    }

                } //0x11 UDP

            ppd = (struct tpacket3_hdr *) ((uint8_t *) ppd + ppd->tp_next_offset);
            } //for (i = 0; i < num_pkts; ++i)
        }


        //================================== walk_block(base_hash,pbd, current_block_num);

        pbd->h1.block_status = TP_STATUS_KERNEL;                //flush_block(pbd);
        current_block_num = (current_block_num + 1) % blocknum; //next block
        } //while(th_running)

    // done , reset is_running
    pInterface->is_running = 0;
}
//==========================================================================



