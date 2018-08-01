#include "ethernet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include <linux/if_packet.h>
//#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/if_ether.h>

#define ARP_REQUEST 1
#define ARP_RESPONSE 2

struct arp_header {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];
};

static const uint8_t REAL_MAC_ADDRESS[ 6 ] = { 0x54, 0xe1, 0xad, 0x74, 0x33, 0xfd };
//static const uint8_t VIRTUAL_MAC_ADDRESS[ 6 ] = { 0x72, 0x5c, 0xb4, 0x2c, 0xac, 0x4a };
static const uint8_t VIRTUAL_MAC_ADDRESS[ 6 ] = { 0x00, 0x11, 0x11, 0x11, 0x11, 0x11 };
static const uint8_t BROADCAST_MAC_ADDRESS[ 6 ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const char *IF_NAME = "vpeth1";

#define SYS_CHECK(arg,msg)  \
    if ((arg) < 0) {      \
        perror(msg);        \
        throw std::runtime_error("error");   \
    }

// see: https://stackoverflow.com/questions/21411851/how-to-send-data-over-a-raw-ethernet-socket-using-sendto-without-using-sockaddr
int get_interface_index(const char interface_name[IFNAMSIZ], int sockfd) {
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
    SYS_CHECK (ioctl(sockfd, SIOCGIFINDEX, &if_idx), "ioctl.SIOCGIFINDEX");
    return if_idx.ifr_ifindex;
}


void dump_ethernet_frame(uint8_t *buf, size_t size) {
	return;
    std::ios_base::fmtflags f( std::cout.flags() );
    for (int i=0; i<size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << " ";
    }
    std::cout << std::endl;
    std::cout.flags( f );
}

void dump_mac_address(uint8_t *p) {
    std::ios_base::fmtflags f( std::cout.flags() );
    for (int i=0; i<ETH_ALEN; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)p[i] << ":";
    }
    std::cout << std::endl;
    std::cout.flags( f );
}


void EthernetDevice::init_raw_sockets() {
    send_sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    SYS_CHECK(send_sockfd, "send socket");

    recv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    SYS_CHECK(recv_sockfd, "recv socket");

    /*
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, IF_NAME, IFNAMSIZ-1);
    auto ans = setsockopt(send_sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
    SYS_CHECK(ans, "setsockopt.send_socket");

    ans = setsockopt(recv_sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
    SYS_CHECK(ans, "setsockopt.recv_socket");
     */
}


bool EthernetDevice::try_recv_raw_frame() {
    //std::cout << "[ethernet] try recv raw frame" << std::endl;

    struct sockaddr_ll src_addr;
    socklen_t addrlen;

    ssize_t ans = recvfrom(recv_sockfd, recv_frame_buf, FRAME_SIZE, MSG_DONTWAIT, (struct sockaddr *)&src_addr, &addrlen);
    assert (ans <= FRAME_SIZE);
    if (ans == 0) {
        std::cout << "[ethernet] recv socket received zero bytes ... connection closed?" << std::endl;
    } else if (ans == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            std::cout << "[ethernet] recv socket no data available -> skip" << std::endl;
        else
            throw std::runtime_error("recvfrom failed");
    } else {
        assert (ETH_ALEN == 6);

        bool virtual_match = memcmp(recv_frame_buf, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;
        bool broadcast_match = memcmp(recv_frame_buf, BROADCAST_MAC_ADDRESS, ETH_ALEN) == 0;
        bool own_packet = memcmp(recv_frame_buf+ETH_ALEN, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;

        if (virtual_match || (broadcast_match && !own_packet)) {
            std::string recv_mode(virtual_match ? "(direct)" : "(broadcast)");
            std::cout << "[ethernet] recv socket " << ans << " bytes received " << recv_mode << std::endl;
            has_frame = true;
            dump_ethernet_frame(recv_frame_buf, ans);
            receive_size = ans;
        } else {
            std::cout << "[ethernet] ignore ethernet packet to different MAC address (or own broadcast)" << std::endl;
            //dump_mac_address(src_addr.sll_addr);
            dump_mac_address(recv_frame_buf);
            return false;
        }
    }
    return true;
}


void EthernetDevice::send_raw_frame() {
    std::cout << "[ethernet] send operation" << std::endl;
    std::cout << std::defaultfloat << "[ethernet] send source 0x" << std::hex << send_src << std::endl;
    std::cout << "[ethernet] send size " << send_size << std::endl;

    // 6 bytes DST MAC Address (ff:ff:ff:ff:ff:ff means broadcast)
    // 6 bytes SRC MAC Address
    // 2 bytes EtherType (0x0800 means IPv4)
    // rest is payload; it seems like a preamble (start) and CRC checksum (end) is not send by FreeRTOS-UDP

    uint8_t sendbuf[send_size < 60 ? 60 : send_size];
    memcpy(sendbuf, &mem[send_src - 0x80000000], send_size);
    if (send_size < 60) {
    	memset(&sendbuf[send_size], 0, 60 - send_size);
        send_size = 60;
    }
    dump_ethernet_frame(sendbuf, send_size);

    struct ether_header *eh = (struct ether_header *)sendbuf;
    std::cout << "destination MAC: ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_dhost), 6);
    std::cout << std::endl;
    std::cout << "source MAC     : ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_shost), 6);
    std::cout << std::endl;
    std::cout << "type           : " << std::hex << eh->ether_type  << std::endl;


    assert (memcmp(eh->ether_shost, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0);

    std::cout << "[arp] ether-type=" << eh->ether_type << std::endl;
    if (eh->ether_type == htons(0x0806)) {
        // ARP packet
        std::cout << "[arp] type detected" << std::endl;
        arp_header *arp = (arp_header *)(sendbuf + sizeof(struct ether_header));
        if (arp->oper == htons(ARP_REQUEST)) {
            std::cout << "[arp] request detected" << std::endl;
            //TODO: parse the linux ARP cache, instead using a hardcoded IP here
            uint8_t own_IP[4] = {0xC0, 0xA8, 0x00, 0x5A};
            if (memcmp(arp->target_ip, own_IP, sizeof(own_IP)) == 0) {
                std::cout << "[arp] IP matches" << std::endl;
                // ARP request for host ethernet adapter -> construct result ARP and inject it back
                std::array<uint8_t, 60> response;
                memset(response.data(), 0, 60);

                struct ether_header *r_eh = (struct ether_header *)response.data();
                arp_header *r_arp = (arp_header *)(response.data() + sizeof(struct ether_header));
                memcpy(r_eh->ether_dhost, eh->ether_shost, ETH_ALEN);
                memcpy(r_eh->ether_shost, REAL_MAC_ADDRESS, ETH_ALEN);
                r_eh->ether_type = eh->ether_type;
                r_arp->htype = arp->htype;
                r_arp->ptype = arp->ptype;
                r_arp->hlen = arp->hlen;
                r_arp->plen = arp->plen;
                r_arp->oper = htons(ARP_RESPONSE);
                memcpy(r_arp->sender_mac, REAL_MAC_ADDRESS, ETH_ALEN);
                memcpy(r_arp->sender_ip, own_IP, sizeof(own_IP));
                memcpy(r_arp->target_mac, REAL_MAC_ADDRESS, ETH_ALEN);  //TODO: use sender MAC
                memcpy(r_arp->target_ip, own_IP, sizeof(own_IP));       //TODO: use sender IP

                std::cout << "[ethernet] inject arp response" << std::endl;
                dump_ethernet_frame(response.data(), 60);
                arp_responses.push_back(response);
                return;
            }
        }
    }


    /*
     * example ARP request packet
     *
    72 5c b4 2c ac 4a       // DST MAC addr
    5c 35 3b 87 27 86       // SRC MAC addr
    08 06                   // ethernet packet type (ARP)
    00 01                   // HTYPE
    08 00                   // PTYPE (ethernet IP)
    06 04                   // HLEN (size of MAC address = 6), PLEN (packet address length = 4, IP)
    00 01                   // ARP operation, request=1, response=2
    5c 35 3b 87 27 86       // sender MAC addr
    c0 a8 00 01             // sender IP addr
    00 00 00 00 00 00       // target MAC addr (requested in this case)
    c0 a8 00 bc             // target IP
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00       // pad with 0 to fill 60 bytes (minimum packet size, not sure this is enforced/required)
     */


    // passthrough all other packets to the host virtual ethernet adapter

    struct sockaddr_ll socket_address;

    socket_address.sll_ifindex = get_interface_index(IF_NAME, send_sockfd);
    /* Address length*/
    assert (ETH_ALEN == 6);
    socket_address.sll_halen = ETH_ALEN;
    /* Destination MAC */
    /*
    socket_address.sll_addr[0] = sendbuf[0];
    socket_address.sll_addr[1] = sendbuf[1];
    socket_address.sll_addr[2] = sendbuf[2];
    socket_address.sll_addr[3] = sendbuf[3];
    socket_address.sll_addr[4] = sendbuf[4];
    socket_address.sll_addr[5] = sendbuf[5];
     */
    memcpy(socket_address.sll_addr, sendbuf, ETH_ALEN);

    auto ans = sendto(send_sockfd, sendbuf, send_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
    assert (ans == send_size);

    std::cout << "[ethernet] normal send finished" << std::endl;
}
