// Headers: Eth, IP, and UDP headers.
header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list : 32;
        resubmit_flag : 16;
        recirculate_flag : 16;
        ingress_global_timestamp : 32;
    }
}
metadata intrinsic_metadata_t intrinsic_metadata;


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
// 20 byte header.
header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

// 20 byte tcp header. 20 + 20 + 14 = 54 bytes of network headers.
header_type tcp_t {
    fields {
        ports : 32;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

parser parse_ethernet {
    extract(ethernet);
    return parse_ipv4;
}
parser parse_ipv4 {
    extract(ipv4);
    // return parse_udp;
    return parse_tcp;
}

parser parse_tcp {
    // return ingress;
    extract(tcp);
    return parse_custom;
}
