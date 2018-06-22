#include "ethernet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <linux/if_packet.h>
//#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/if_ether.h>

static const uint8_t ucMACAddress[ 6 ] = { 0x54, 0xe1, 0xad, 0x74, 0x33, 0xfd };

static const char *IF_NAME = "enp0s31f6";

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
    std::ios_base::fmtflags f( std::cout.flags() );
    for (int i=0; i<size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << " ";
    }
    std::cout << std::endl;
    std::cout.flags( f );
}


void EthernetDevice::init_raw_sockets() {
    send_sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    SYS_CHECK(send_sockfd, "send socket");

    recv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    SYS_CHECK(recv_sockfd, "recv socket");
}


void EthernetDevice::try_recv_raw_frame() {
    std::cout << "[ethernet] try recv raw frame" << std::endl;

    struct sockaddr_ll src_addr;
    socklen_t addrlen;

    ssize_t ans = recvfrom(recv_sockfd, recv_payload_buf, MTU_SIZE, MSG_DONTWAIT, (struct sockaddr *)&src_addr, &addrlen);
    assert (ans <= MTU_SIZE);
    if (ans == 0) {
        std::cout << "[ethernet] recv socket received zero bytes ... connection closed?" << std::endl;
    } else if (ans == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            std::cout << "[ethernet] recv socket no data available -> skip" << std::endl;
        else
            throw std::runtime_error("recvfrom failed");
    } else {
        std::cout << "[ethernet] recv socket " << ans << " bytes received" << std::endl;
        has_frame = true;
        dump_ethernet_frame(recv_payload_buf, ans);

        assert (ETH_ALEN == 6);
        // dst MAC addr (own)
        recv_frame_buf[0] = ucMACAddress[0];
        recv_frame_buf[1] = ucMACAddress[1];
        recv_frame_buf[2] = ucMACAddress[2];
        recv_frame_buf[3] = ucMACAddress[3];
        recv_frame_buf[4] = ucMACAddress[4];
        recv_frame_buf[5] = ucMACAddress[5];
        // src MAC addr
        memcpy(recv_frame_buf+6, src_addr.sll_addr, ETH_ALEN);
        // IPv4 family
        recv_frame_buf[12] = 0x80;
        recv_frame_buf[13] = 0x00;
        // payload (rest)
        memcpy(&recv_frame_buf[14], recv_payload_buf, ans);
        // total size of the frame
        receive_size = ans + 14;

        std::cout << "[ethernet] receive complete frame " << receive_size << " bytes, now active" << std::endl;
        dump_ethernet_frame(recv_frame_buf, receive_size);
    }
}


void EthernetDevice::send_raw_frame() {
    std::cout << "[ethernet] send operation" << std::endl;
    std::cout << "[ethernet] send source " << send_src << std::endl;
    std::cout << "[ethernet] send size " << send_size << std::endl;

    // 6 bytes DST MAC Address (ff:ff:ff:ff:ff:ff means broadcast)
    // 6 bytes SRC MAC Address
    // 2 bytes EtherType (0x0800 means IPv4)
    // rest is payload; it seems like a preamble (start) and CRC checksum (end) is not send by FreeRTOS-UDP

    uint8_t sendbuf[send_size];
    for (int i=0; i<send_size; ++i) {
        auto k = send_src + i;
        sendbuf[i] = mem[k - 0x80000000];
    }
    dump_ethernet_frame(sendbuf, send_size);


    struct sockaddr_ll socket_address;

    socket_address.sll_ifindex = get_interface_index(IF_NAME, send_sockfd);
    /* Address length*/
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
}