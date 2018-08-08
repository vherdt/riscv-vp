#include "ethernet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
//for debug
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <linux/if_packet.h>
//#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/if_ether.h>

#define ARP_REQUEST 1
#define ARP_RESPONSE 2

using namespace std;

//static const char IF_NAME[] = "lo";
static const char IF_NAME[] = "vpeth1";
//static const char IF_NAME[] = "enp0s31f6";

#define SYS_CHECK(arg,msg)  \
    if ((arg) < 0) {      \
        perror(msg);        \
        throw runtime_error("error");   \
    }

// see: https://stackoverflow.com/questions/21411851/how-to-send-data-over-a-raw-ethernet-socket-using-sendto-without-using-sockaddr
int get_interface_index(const char interface_name[IFNAMSIZ], int sockfd) {
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
    SYS_CHECK (ioctl(sockfd, SIOCGIFINDEX, &if_idx), "ioctl.SIOCGIFINDEX");
    return if_idx.ifr_ifindex;
}

void printHex(const unsigned char* buf, const uint32_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
    	printf("%s%02X", i > 0 ? ":" : "", buf[i]);
    }
}

void printDec(const unsigned char* buf, const uint32_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
    	printf("%s%d", i > 0 ? "." : "", buf[i]);
    }
}


void dump_ethernet_frame(uint8_t *buf, size_t size) {
    ios_base::fmtflags f( cout.flags() );
    for (int i=0; i<size; ++i) {
        cout << hex << setw(2) << setfill('0') << (int)buf[i] << " ";
    }
    cout  << endl;
    cout.flags( f );

    struct ether_header *eh = (struct ether_header *)buf;
    cout << "destination MAC: ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_dhost), 6);
    cout << endl;
    cout << "source MAC     : ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_shost), 6);
    cout << endl;
    cout << "type           : " << ntohs(eh->ether_type)  << endl;

    buf += sizeof(struct ethhdr);
    switch(ntohs(eh->ether_type))
    {
    case ETH_P_IP:
    {
    	cout << "IP" << endl;
    	unsigned short iphdrlen;
    	struct in_addr source;
    	struct in_addr  dest;
    	struct iphdr *ip = (struct iphdr*)buf;
    	memset(&source, 0, sizeof(source));
    	source.s_addr = ip->saddr;
    	memset(&dest, 0, sizeof(dest));
    	dest.s_addr = ip->daddr;
    	cout << "\t|-Version : " << ip->version << endl;
		cout << "\t|-Internet Header Length : " << ip->ihl << " DWORDS or " << ip->ihl*4 << " Bytes" << endl;
		cout << "\t|-Type Of Service : " << (unsigned int)ip->tos << endl;
		cout << "\t|-Total Length : " << ntohs(ip->tot_len) << " Bytes" << endl;
		cout << "\t|-Identification : " << ntohs(ip->id) << endl;
		cout << "\t|-Time To Live : " << (unsigned int)ip->ttl << endl;
    	cout << "\t|-Protocol : " << (unsigned int) ip->protocol << endl;
    	cout << "\t|-Header Checksum : " << ntohs(ip->check) << endl;
    	cout << "\t|-Source IP : " <<  inet_ntoa(source) << endl;
    	cout << "\t|-Destination IP : " << inet_ntoa(dest) << endl;
    	buf += ip->ihl*4;
    	switch(ip->protocol)
    	{
    	case IPPROTO_UDP:
    	{
    		cout << "UDP" << endl;
    		struct udphdr *udp = (struct udphdr*)buf;
    		cout << "\t|-Source port: " << ntohs(udp->source) << endl;
    		cout << "\t|-Destination port: " << ntohs(udp->dest) << endl;
    		cout << "\t|-Length: " << ntohs(udp->len) << endl;
    		cout << "\t|-Checksum: " << ntohs(udp->check) << endl;
    		break;
    	}
    	case IPPROTO_TCP:
    		cout << "TCP" << endl;
    		cout << "\t|-Blah: " << "blah" << endl;
    		break;
    	}
    	break;
    }
    case ETH_P_ARP:
    {
    	cout << "ARP" << endl;
    	struct arp_eth_header *arp = (struct arp_eth_header*) buf;
    	cout << "\t|-Sender MAC: ";
    		printHex((uint8_t*)&arp->sender_mac, 6);
    		cout << endl;
		cout << "\t|-Sender IP: " ;
			printDec((uint8_t*)&arp->sender_ip, 4);
			cout << endl;
		cout << "\t|-DEST MAC: ";
			printHex((uint8_t*)&arp->target_mac, 6);
			cout << endl;
		cout << "\t|-DEST IP: " ;
			printDec((uint8_t*)&arp->target_ip, 4);
			cout << endl;
		cout << "\t|-Operation: " << (ntohs(arp->oper) == 1 ? "REQUEST" : ntohs(arp->oper) == 2 ? "REPLY" : "INVALID") << endl;
    	break;
    }
    default:
    	cout << "unknown protocol" << endl;
    }
}


const char ArpCache::arpLineFormat[] = "%1023s %*s %*s "
										"%1023s %*s "
										"%1023s";
const char ArpCache::arpCachePath[] = "/proc/net/arp";

void ArpCache::readKernelArpCache()
{
    FILE *arpCache = fopen(arpCachePath, "r");
    assert(arpCache && "Failed to open arpCache");

    //Ignore the first line, which contains the header
    char header[bufferLength];
    assert(fgets(header, sizeof(header), arpCache));

    char ipAddr[bufferLength], hwAddr[bufferLength], device[bufferLength];
    int count = 0;
    while (3 == fscanf(arpCache, arpLineFormat, ipAddr, hwAddr, device))
    {
        printf("%03d: Mac Address of [%s] on [%s] is \"%s\"\n",
                count, ipAddr, device, hwAddr);
        struct in_addr inaddr;
        inet_aton(ipAddr, &inaddr);
        uint8_t mac[8];	//same width as uint64_t
        assert(6 == sscanf(hwAddr, "%x:%x:%x:%x:%x:%x%*c",
            &mac[0], &mac[1], &mac[2],
            &mac[3], &mac[4], &mac[5] ) );
        cache[inaddr.s_addr] = *reinterpret_cast<uint64_t*>(mac);
        count ++;
    }
    fclose(arpCache);
}
bool ArpCache::getHwidByIp(const uint32_t* ip, uint8_t* hwid)
{
	if(cache.find(*ip) == cache.end())
	{
		readKernelArpCache();
	}
	if(cache.find(*ip) != cache.end())
	{
		memcpy(&cache[*ip], hwid, 6);
		cout << "ARP cache hit: ";
		printDec(reinterpret_cast<const uint8_t*>(ip), 4);
		cout << " is ";
		printHex(hwid, 6);
		cout << endl;
		return true;
	}
	cout << "ARP cache miss" << endl;
	return false;
}

bool ArpResponder::isArpReq(uint8_t* eth, uint16_t len)
{
    struct ether_header *eh = (struct ether_header *)eth;
    if(ntohs(eh->ether_type) != ETH_P_ARP)
    	return false;
    return ntohs(reinterpret_cast<struct arp_eth_header*>(eth + sizeof(ether_header))->oper) == ARPOP_REQUEST;
}
uint8_t* ArpResponder::buildResponseFrom(uint8_t* eth)
{
	memset(packet, 0, arpPacketSize);
	struct ether_header *requestEth = (struct ether_header *)eth;
	struct arp_eth_header *requestArp = (struct arp_eth_header *)(eth + sizeof(ether_header));
	struct ether_header *responseEth = (struct ether_header *)packet;
	struct arp_eth_header *responseArp = (struct arp_eth_header *)(packet + sizeof(ether_header));

	memcpy(responseEth->ether_dhost, requestEth->ether_shost, 4);
	memcpy(responseEth->ether_shost, requestEth->ether_dhost, 4);
	responseEth->ether_type = ETH_P_ARP;

	responseArp->htype = htons(ARPHRD_ETHER);
	responseArp->ptype = ETH_P_IP;
	responseArp->hlen  = htons(6);
	responseArp->plen  = htons(4);
	responseArp->oper  = htons(ARPOP_REPLY);

	uint8_t requestedHWID[6] = {0};
	if(!cache.getHwidByIp(reinterpret_cast<uint32_t*>(requestArp->target_ip), requestedHWID))
	{
		return nullptr;
	}
	cout << "MAC of requested ";
	printDec(requestArp->target_ip, 4);
	cout << " is ";
	printHex(requestedHWID, 6);
	cout << endl;

	memcpy(responseArp->sender_mac, requestedHWID, 6);
	memcpy(responseArp->sender_ip, requestArp->target_ip, 4);

	memcpy(responseArp->target_mac, requestArp->sender_mac, 6);
	memcpy(responseArp->target_ip, requestArp->sender_ip, 4);

	cout << "FORGED ARP RESPONSE: " << endl;
	dump_ethernet_frame(packet, arpPacketSize);

	return packet;
}

EthernetDevice::EthernetDevice(sc_core::sc_module_name, uint32_t irq_number, uint8_t *mem)
        : irq_number(irq_number), mem(mem) {
    tsock.register_b_transport(this, &EthernetDevice::transport);
    SC_THREAD(run);

    router.add_register_bank({
             {STATUS_REG_ADDR, &status},
             {RECEIVE_SIZE_REG_ADDR, &receive_size},
             {RECEIVE_DST_REG_ADDR, &receive_dst},
             {SEND_SRC_REG_ADDR, &send_src},
             {SEND_SIZE_REG_ADDR, &send_size},
             {MAC_HIGH_REG_ADDR, &mac[0]},
             {MAC_LOW_REG_ADDR, &mac[1]},
     }).register_handler(this, &EthernetDevice::register_access_callback);

    init_raw_sockets();
}

void EthernetDevice::init_raw_sockets() {
    send_sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    SYS_CHECK(send_sockfd, "send socket");

    recv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    SYS_CHECK(recv_sockfd, "recv socket");

	struct ifreq ifopts;

	/* Get the MAC address of the interface to send on */
	memset(&ifopts, 0, sizeof(struct ifreq));
	strncpy(ifopts.ifr_name, IF_NAME, IFNAMSIZ-1);
	if (ioctl(send_sockfd, SIOCGIFHWADDR, &ifopts) < 0)
	{
		perror("SIOCGIFHWADDR");
	}

	//Save own MAC in register
	memcpy(VIRTUAL_MAC_ADDRESS, ifopts.ifr_hwaddr.sa_data, 6);

	/* Get the index of the interface to send on */
	memset(&ifopts, 0, sizeof(struct ifreq));
	strncpy(ifopts.ifr_name, IF_NAME, IFNAMSIZ-1);
	if (ioctl(send_sockfd, SIOCGIFINDEX, &ifopts) < 0)
	{
		perror("SIOCGIFINDEX");
	}
	//save Index
	interfaceIdx = ifopts.ifr_ifindex;
	cout << "Interface index: " << interfaceIdx << endl;

	// Receive-Socket
	memset(&ifopts, 0, sizeof(struct ifreq));

	/* Set interface to promiscuous mode */
	strncpy(ifopts.ifr_name, IF_NAME, IFNAMSIZ-1);
	ioctl(recv_sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(recv_sockfd, SIOCSIFFLAGS, &ifopts);


	int sockopt;
	/* Allow the receive socket to be reused - in case connection is closed prematurely */
	if (setsockopt(recv_sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	/* Bind to device */
	if (setsockopt(recv_sockfd, SOL_SOCKET, SO_BINDTODEVICE, IF_NAME, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		exit(EXIT_FAILURE);
	}

}


bool EthernetDevice::try_recv_raw_frame() {
    socklen_t addrlen;

    ssize_t ans = recv(recv_sockfd, recv_frame_buf, FRAME_SIZE, MSG_DONTWAIT);
    assert (ans <= FRAME_SIZE);
    if (ans == 0) {
        cout << "[ethernet] recv socket received zero bytes ... connection closed?" << endl;
    } else if (ans == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
			//cout << "[ethernet] recv socket no data available -> skip" << endl;
        }
        else
            throw runtime_error("recvfrom failed");
    } else {
        assert (ETH_ALEN == 6);

        bool virtual_match = memcmp(recv_frame_buf, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;
        bool broadcast_match = memcmp(recv_frame_buf, BROADCAST_MAC_ADDRESS, ETH_ALEN) == 0;
        bool own_packet = memcmp(recv_frame_buf+ETH_ALEN, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;

        if (virtual_match || (broadcast_match && !own_packet)) {
            string recv_mode(virtual_match ? "(direct)" : "(broadcast)");
            //cout << "[ethernet] recv socket " << ans << " bytes received " << recv_mode << endl;
            has_frame = true;
            receive_size = ans;
            if(virtual_match)
            {
            	dump_ethernet_frame(recv_frame_buf, ans);
            }
        } else {
            //cout << "[ethernet] ignore ethernet packet to different MAC address (or own broadcast)" << endl;
            //dump_mac_address(src_addr.sll_addr);
            //dump_mac_address(recv_frame_buf);
            return false;
        }
    }
    return true;
}

void EthernetDevice::send_raw_frame() {
    uint8_t sendbuf[send_size < 60 ? 60 : send_size];
    memcpy(sendbuf, &mem[send_src - 0x80000000], send_size);
    if (send_size < 60) {
    	memset(&sendbuf[send_size], 0, 60 - send_size);
        send_size = 60;
    }
    dump_ethernet_frame(sendbuf, send_size);

    struct ether_header *eh = (struct ether_header *)sendbuf;

    assert (memcmp(eh->ether_shost, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0);

    struct sockaddr_ll socket_idx;
    memset(&socket_idx, 0, sizeof(sockaddr_ll));
    socket_idx.sll_ifindex = interfaceIdx;

    ssize_t ans = sendto(send_sockfd, sendbuf, send_size, 0, (struct sockaddr*)&socket_idx, sizeof(sockaddr_ll));
    if(ans != send_size)
    {
    	cout << strerror(errno) << endl;
    }
    assert (ans == send_size);

    if(arpResponder.isArpReq(sendbuf, send_size))
	{
    	uint8_t* response = arpResponder.buildResponseFrom(sendbuf);
    	if(response == nullptr)
    	{
    		//we cannot satisfy request
    		return;
    	}
        memset(&socket_idx, 0, sizeof(sockaddr_ll));
        socket_idx.sll_ifindex = interfaceIdx;
        ans = sendto(send_sockfd, response, ArpResponder::arpPacketSize, 0, (struct sockaddr*)&socket_idx, sizeof(sockaddr_ll));
        if(ans != ArpResponder::arpPacketSize)
        {
        	cout << strerror(errno) << endl;
        }
        assert (ans == ArpResponder::arpPacketSize);
	}

}
