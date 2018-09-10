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
#include <net/if.h>
//#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <netdb.h>
#include <ifaddrs.h>

using namespace std;

//static const char IF_NAME[] = "mvl0";
static const char IF_NAME[] = "tap0";
//static const char IF_NAME[] = "macvtap0";
//static const char IF_NAME[] = "enp0s31f6";

#define SYS_CHECK(arg,msg)  \
    if ((arg) < 0) {      \
        perror(msg);        \
        throw runtime_error("error");   \
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
	return;
	uint8_t* readbuf = buf;
    struct ether_header *eh = (struct ether_header *)readbuf;
    cout << "destination MAC: ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_dhost), 6);
    cout << endl;
    cout << "source MAC     : ";
    printHex(reinterpret_cast<unsigned char*>(&eh->ether_shost), 6);
    cout << endl;
    cout << "type           : " << ntohs(eh->ether_type)  << endl;

    readbuf += sizeof(struct ethhdr);
    switch(ntohs(eh->ether_type))
    {
    case ETH_P_IP:
    {
    	cout << "IP" << endl;
    	unsigned short iphdrlen;
    	struct in_addr source;
    	struct in_addr  dest;
    	struct iphdr *ip = (struct iphdr*)readbuf;
    	memset(&source, 0, sizeof(source));
    	source.s_addr = ip->saddr;
    	memset(&dest, 0, sizeof(dest));
    	dest.s_addr = ip->daddr;
    	cout << "\t|-Version               : " << ip->version << endl;
		cout << "\t|-Internet Header Length: " << ip->ihl << " DWORDS or " << ip->ihl*4 << " Bytes" << endl;
		cout << "\t|-Type Of Service       : " << (unsigned int)ip->tos << endl;
		cout << "\t|-Total Length          : " << ntohs(ip->tot_len) << " Bytes" << endl;
		cout << "\t|-Identification        : " << ntohs(ip->id) << endl;
		cout << "\t|-Time To Live          : " << (unsigned int)ip->ttl << endl;
    	cout << "\t|-Protocol              : " << (unsigned int) ip->protocol << endl;
    	cout << "\t|-Header Checksum       : " << ntohs(ip->check) << endl;
    	cout << "\t|-Source IP             : " <<  inet_ntoa(source) << endl;
    	cout << "\t|-Destination IP        : " << inet_ntoa(dest) << endl;
    	readbuf += ip->ihl*4;
    	switch(ip->protocol)
    	{
    	case IPPROTO_UDP:
    	{
    		cout << "UDP" << endl;
    		struct udphdr *udp = (struct udphdr*)readbuf;
    		cout << "\t|-Source port     : " << ntohs(udp->source) << endl;
    		cout << "\t|-Destination port: " << ntohs(udp->dest) << endl;
    		cout << "\t|-Length          : " << ntohs(udp->len) << endl;
    		cout << "\t|-Checksum        : " << ntohs(udp->check) << endl;
    		readbuf += sizeof(udphdr);
    		switch(ntohs(udp->dest))
    		{
    		case 67:
    			cout << "DHCP ";
    			switch(ntohs(readbuf[0]))
    			{
    			case 1:
    				cout << "DISCOVER/REQUEST";
    				break;
    			case 2:
    				cout << "ACK";
    			default:
    				cout << "UNKNOWN (" << to_string(ntohs(readbuf[0])) << ")";
    			}
    			cout << endl;
    			break;
    		default:
    			break;
    		}
    		return;
		}
    	case IPPROTO_TCP:
    		cout << "TCP" << endl;
    		//fall-through
    	default:
    		return;
    	}
    	break;
    }
    case ETH_P_ARP:
    {
    	cout << "ARP" << endl;
    	struct arp_eth_header *arp = (struct arp_eth_header*) readbuf;
    	cout << "\t|-Sender MAC: ";
    		printHex((uint8_t*)&arp->sender_mac, 6);
    		cout << endl;
		cout << "\t|-Sender IP : " ;
			printDec((uint8_t*)&arp->sender_ip, 4);
			cout << endl;
		cout << "\t|-DEST MAC  : ";
			printHex((uint8_t*)&arp->target_mac, 6);
			cout << endl;
		cout << "\t|-DEST IP   : " ;
			printDec((uint8_t*)&arp->target_ip, 4);
			cout << endl;
		cout << "\t|-Operation : " << (ntohs(arp->oper) == 1 ? "REQUEST" : ntohs(arp->oper) == 2 ? "REPLY" : "INVALID") << endl;
		return;
    	break;
    }
    default:
    	cout << "unknown protocol" << endl;
    	return;
    }

    ios_base::fmtflags f( cout.flags() );
    for (int i=0; i<size; ++i) {
        cout << hex << setw(2) << setfill('0') << (int)buf[i] << " ";
    }
    cout  << endl;
    cout.flags( f );
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

    init_network();
}

void EthernetDevice::init_network() {
	struct ifreq ifr;
	int err;
	char clonedev[] = "/dev/net/tun";

	if( (sockfd = open(clonedev , O_RDWR)) < 0 )
	{
		perror("Opening /dev/net/tun");
		assert(sockfd >= 0);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	strncpy(ifr.ifr_name, IF_NAME, IFNAMSIZ);

	if( (err = ioctl(sockfd, TUNSETIFF, (void *)&ifr)) < 0 )
	{
		perror("ioctl(TUNSETIFF)");
		close(sockfd);
		assert(err >= 0);
	}

	/* Get the MAC address of the interface to send on */
	struct ifreq ifopts;
	memset(&ifopts, 0, sizeof(struct ifreq));
	strncpy(ifopts.ifr_name, IF_NAME, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifopts) < 0)
	{
		perror("SIOCGIFHWADDR");
		assert(sockfd >= 0);
	}
	//Save own MAC in register
	memcpy(VIRTUAL_MAC_ADDRESS, ifopts.ifr_hwaddr.sa_data, 6);

	fcntl(sockfd, F_SETFL, O_NONBLOCK);
}

void EthernetDevice::send_raw_frame() {
    uint8_t sendbuf[send_size < 60 ? 60 : send_size];
    memcpy(sendbuf, &mem[send_src - 0x80000000], send_size);
    if (send_size < 60) {
    	memset(&sendbuf[send_size], 0, 60 - send_size);
        send_size = 60;
    }

    cout << "\nSEND FRAME --->--->--->--->--->--->" << endl;
    dump_ethernet_frame(sendbuf, send_size);

    struct ether_header *eh = (struct ether_header *)sendbuf;

    assert (memcmp(eh->ether_shost, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0);

    ssize_t ans = write(sockfd, sendbuf, send_size);
    if(ans != send_size)
    {
    	cout << strerror(errno) << endl;
    }
    assert (ans == send_size);
}

bool EthernetDevice::isPacketForUs(uint8_t* packet, ssize_t size)
{
    ether_header *eh = reinterpret_cast<ether_header*>(packet);
    bool virtual_match = memcmp(eh->ether_dhost, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;
    bool broadcast_match = memcmp(eh->ether_dhost, BROADCAST_MAC_ADDRESS, ETH_ALEN) == 0;
    bool own_packet = memcmp(eh->ether_shost, VIRTUAL_MAC_ADDRESS, ETH_ALEN) == 0;

    if (!virtual_match && !(broadcast_match && !own_packet))
    {
    	return false;
    }

    return true;	//receive everything

	if(ntohs(eh->ether_type) != ETH_P_IP)
	{
		return false; //Filter all non-IP!

		if(ntohs(eh->ether_type) != ETH_P_ARP)
		{	//Not ARP
			//cout << "dumped non-ARP packet" << endl;
			return false;
		}

		arp_eth_header *arp = reinterpret_cast<arp_eth_header*>(packet + sizeof(iphdr));
		uint8_t ownIp[4] = {0x86, 0x66, 0xDA, 0xBE};
		if(memcmp(arp->target_ip, ownIp, 4))
		{	//not to us directly
			return false;
		}
	}
	else
	{
		iphdr *ip = reinterpret_cast<iphdr*>(packet + sizeof(ether_header));
		if(ip->protocol != IPPROTO_UDP)
		{	//not UDP
			//cout << "dumped non-UDP packet" << endl;
			return false;
		}

		udphdr *udp = reinterpret_cast<udphdr*>(packet + sizeof(ether_header) + sizeof(iphdr));
		if(ntohs(udp->uh_dport) != 67 && ntohs(udp->uh_dport) != 68)
		{	//not DHCP
			//cout << "dumped non-DHCP packet" << endl;
			return false;
		}
	}
	return true;
}

bool EthernetDevice::try_recv_raw_frame() {
    socklen_t addrlen;
    ssize_t ans;

	ans = read(sockfd, recv_frame_buf, FRAME_SIZE);
	assert (ans <= FRAME_SIZE);
	if (ans == 0) {
		cerr << "[ethernet] recv socket received zero bytes ... connection closed?" << endl;
		throw runtime_error("read failed");
	} else if (ans == -1) {
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			return false;
		}
		else
			throw runtime_error("recvfrom failed");
	}
	assert (ETH_ALEN == 6);

	if(!isPacketForUs(recv_frame_buf, ans))
	{
		return false;
	}

	has_frame = true;
	receive_size = ans;
	cout << "\nRECEIVED FRAME <---<---<---<---<---" << endl;
	dump_ethernet_frame(recv_frame_buf, ans);

	return true;
}


