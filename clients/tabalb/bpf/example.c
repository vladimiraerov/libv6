#include <stdio.h>
#include <linux/bpf.h>

  /* bpf+sockets example:
        * 1. create array map of 256 elements
        * 2. load program that counts number of packets received
        *    r0 = skb->data[ETH_HLEN + offsetof(struct iphdr, protocol)]
        *    map[r0]++
        * 3. attach prog_fd to raw socket via setsockopt()
        * 4. print number of received TCP/UDP packets every second
        */
       int
       main(int argc, char **argv)
       {
           int sock, map_fd, prog_fd, key;
           long long value = 0, tcp_cnt, udp_cnt;

           map_fd = bpf_create_map(BPF_MAP_TYPE_ARRAY, sizeof(key),
                                   sizeof(value), 256);
           if (map_fd < 0) {
               printf("failed to create map '%s'\n", strerror(errno));
               /* likely not run as root */
               return 1;
           }

           struct bpf_insn prog[] = {
               BPF_MOV64_REG(BPF_REG_6, BPF_REG_1),        /* r6 = r1 */
               BPF_LD_ABS(BPF_B, ETH_HLEN + offsetof(struct iphdr, protocol)),
                                       /* r0 = ip->proto */
               BPF_STX_MEM(BPF_W, BPF_REG_10, BPF_REG_0, -4),
                                       /* *(u32 *)(fp - 4) = r0 */
               BPF_MOV64_REG(BPF_REG_2, BPF_REG_10),       /* r2 = fp */
               BPF_ALU64_IMM(BPF_ADD, BPF_REG_2, -4),      /* r2 = r2 - 4 */
               BPF_LD_MAP_FD(BPF_REG_1, map_fd),           /* r1 = map_fd */
               BPF_CALL_FUNC(BPF_FUNC_map_lookup_elem),
                                       /* r0 = map_lookup(r1, r2) */
               BPF_JMP_IMM(BPF_JEQ, BPF_REG_0, 0, 2),
                                       /* if (r0 == 0) goto pc+2 */
               BPF_MOV64_IMM(BPF_REG_1, 1),                /* r1 = 1 */
               BPF_XADD(BPF_DW, BPF_REG_0, BPF_REG_1, 0, 0),
                                       /* lock *(u64 *) r0 += r1 */
               BPF_MOV64_IMM(BPF_REG_0, 0),                /* r0 = 0 */
               BPF_EXIT_INSN(),                            /* return r0 */
           };

           prog_fd = bpf_prog_load(BPF_PROG_TYPE_SOCKET_FILTER, prog,
                                   sizeof(prog), "GPL");

           sock = open_raw_sock("lo");

           assert(setsockopt(sock, SOL_SOCKET, SO_ATTACH_BPF, &prog_fd,
                             sizeof(prog_fd)) == 0);

           for (;;) {
               key = IPPROTO_TCP;
               assert(bpf_lookup_elem(map_fd, &key, &tcp_cnt) == 0);
               key = IPPROTO_UDP
               assert(bpf_lookup_elem(map_fd, &key, &udp_cnt) == 0);
               printf("TCP %lld UDP %lld packets0, tcp_cnt, udp_cnt);
               sleep(1);
           }

           return 0;
       }

