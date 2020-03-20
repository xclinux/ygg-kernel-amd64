#include "net/packet.h"
#include "user/errno.h"
#include "sys/debug.h"
#include "sys/panic.h"
#include "net/util.h"
#include "net/inet.h"
#include "net/icmp.h"
#include "net/arp.h"
#include "net/eth.h"
#include "net/udp.h"
#include "net/if.h"

uint16_t inet_checksum(const void *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len / 2; ++i) {
        sum += ((const uint16_t *) data)[i];
    }
    if (len % 2) {
        sum += ((const uint8_t *) data)[len - 1];
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    return ~(sum & 0xFFFF) & 0xFFFF;
}

int inet_send_wrapped(struct netdev *src, uint32_t inaddr, uint8_t proto, void *data, size_t len) {
    if (!(src->flags & IF_F_HASIP)) {
        kwarn("%s: has no inaddr\n", src->name);
        return -EINVAL;
    }

    struct inet_frame *ip = data + sizeof(struct eth_frame);
    const uint8_t *hwaddr = arp_resolve(src, inaddr);
    if (!hwaddr) {
        panic("TODO: handle this\n");
    }

    ip->dst_inaddr = htonl(inaddr);
    ip->src_inaddr = htonl(src->inaddr);
    ip->checksum = 0;
    ip->tos = 0;
    ip->ttl = 64;
    ip->ihl = sizeof(struct inet_frame) / 4;
    ip->version = 4;
    ip->tot_len = htons(len - sizeof(struct eth_frame));
    ip->flags = 0;
    ip->proto = proto;

    ip->checksum = inet_checksum(ip, sizeof(struct inet_frame));

    return eth_send_wrapped(src, hwaddr, ETH_T_IP, data, len);
}

void inet_handle_frame(struct packet *p, struct eth_frame *eth, void *data, size_t len) {
    if (len < sizeof(struct inet_frame)) {
        kwarn("%s: dropping undersized frame\n", p->dev->name);
        return;
    }

    struct inet_frame *ip = data;

    if (ip->version != 4) {
        kwarn("%s: version != 4\n", p->dev->name);
        return;
    }

    size_t frame_size = ip->ihl * 4;

    if (len < frame_size) {
        kwarn("%s: dropping undersized frame\n", p->dev->name);
    }

    len -= frame_size;
    data += frame_size;

    switch (ip->proto) {
    case INET_P_ICMP:
        icmp_handle_frame(p, eth, ip, data, len);
        break;
    case INET_P_UDP:
        udp_handle_frame(p, eth, ip, data, len);
        break;
    default:
        kwarn("%s: dropping unknown protocol %02x\n", p->dev->name, ip->proto);
        break;
    }
}
