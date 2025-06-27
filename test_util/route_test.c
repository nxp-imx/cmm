/* SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 * Copyright 2025 NXP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>

int main() {
    // Create a Netlink socket for routing operations
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Buffer for the Netlink request message
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    // Set up the Netlink message header
    struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;           // Request route information
    nlh->nlmsg_flags = NLM_F_REQUEST;         // Indicate this is a request
    nlh->nlmsg_seq = 1;                       // Sequence number for tracking
    nlh->nlmsg_pid = getpid();                // Process ID for uniqueness

    // Set up the routing message header
    struct rtmsg *rtm = (struct rtmsg *)(buf + NLMSG_HDRLEN);
    rtm->rtm_family = AF_INET;                // IPv4 address family
    rtm->rtm_dst_len = 32;                    // Destination prefix length (host route)
    rtm->rtm_src_len = 32;                    // Source prefix length (host route)
    rtm->rtm_table = RT_TABLE_MAIN;           // Use the main routing table
    rtm->rtm_protocol = RTPROT_UNSPEC;        // Unspecified protocol
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;       // Global scope
    rtm->rtm_type = RTN_UNSPEC;               // Unspecified route type
    rtm->rtm_flags = 0;                       // No additional flags

    // Pointer to add attributes after the rtmsg structure
    char *ptr = (char *)rtm + sizeof(struct rtmsg);

    // Add destination IP attribute (RTA_DST)
    struct rtattr *rta_dst = (struct rtattr *)ptr;
    rta_dst->rta_type = RTA_DST;              // Attribute type: destination
    rta_dst->rta_len = RTA_LENGTH(4);         // Length of IPv4 address
    inet_pton(AF_INET, "1.1.1.1", RTA_DATA(rta_dst)); // Convert and set destination IP
    ptr += RTA_ALIGN(rta_dst->rta_len);       // Move pointer, aligning to boundary

    // Add source IP attribute (RTA_SRC)
    struct rtattr *rta_src = (struct rtattr *)ptr;
    rta_src->rta_type = RTA_SRC;              // Attribute type: source
    rta_src->rta_len = RTA_LENGTH(4);         // Length of IPv4 address
    inet_pton(AF_INET, "2.1.1.1", RTA_DATA(rta_src)); // Convert and set source IP
    ptr += RTA_ALIGN(rta_src->rta_len);       // Move pointer, aligning to boundary

    // Update the total message length
    nlh->nlmsg_len = ptr - buf;

    // Set up the kernel address for sending the message
    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;                // Netlink family
    sa.nl_pid = 0;                            // Kernel PID (0 for kernel)
    sa.nl_groups = 0;                         // No multicast groups

    // Send the request to the kernel
    if (sendto(sock, buf, nlh->nlmsg_len, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("Failed to send message");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Buffer for the response
    char resp_buf[4096];
    memset(resp_buf, 0, sizeof(resp_buf));

    // Receive the response from the kernel
    struct sockaddr_nl sa_resp;
    socklen_t sa_len = sizeof(sa_resp);
    int len = recvfrom(sock, resp_buf, sizeof(resp_buf), 0, (struct sockaddr *)&sa_resp, &sa_len);
    if (len < 0) {
        perror("Failed to receive response");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Process the response
    struct nlmsghdr *nlh_resp = (struct nlmsghdr *)resp_buf;
    if (nlh_resp->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nlh_resp);
        fprintf(stderr, "Netlink error: %d\n", err->error);
    } else if (nlh_resp->nlmsg_type == RTM_NEWROUTE) {
        // Parse the route information
        struct rtmsg *rtm_resp = (struct rtmsg *)NLMSG_DATA(nlh_resp);
        struct rtattr *rta_resp = (struct rtattr *)((char *)rtm_resp + sizeof(struct rtmsg));
        int rta_len = nlh_resp->nlmsg_len - NLMSG_HDRLEN - sizeof(struct rtmsg);

        printf("Route from src 2.1.1.1 to dst 1.1.1.1:\n");
        for (; RTA_OK(rta_resp, rta_len); rta_resp = RTA_NEXT(rta_resp, rta_len)) {
            switch (rta_resp->rta_type) {
                case RTA_OIF: // Output interface index
                    {
                        int ifindex = *(int *)RTA_DATA(rta_resp);
                        char ifname[IF_NAMESIZE];
                        if (if_indextoname(ifindex, ifname)) {
                            printf("  Output interface: %s\n", ifname);
                        } else {
                            printf("  Output interface index: %d (name unavailable)\n", ifindex);
                        }
                    }
                    break;
                case RTA_GATEWAY: // Next hop gateway
                    {
                        char gw[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, RTA_DATA(rta_resp), gw, sizeof(gw));
                        printf("  Gateway: %s\n", gw);
                    }
                    break;
                default:
                    break;
            }
        }
    } else {
        fprintf(stderr, "Unexpected response type: %d\n", nlh_resp->nlmsg_type);
    }

    // Clean up
    close(sock);
    return 0;
}
