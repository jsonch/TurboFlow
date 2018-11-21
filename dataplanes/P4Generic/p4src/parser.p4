/**
 *
 * Headers, metadata, and parser.
 *
 */

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_TURBOFLOW 0x081A

// Metadata for processing.
metadata tfMeta_t tfMeta;

// Headers for exporting an evicted flow record.
header tfExportStart_t tfExportStart;
header tfExportKey_t tfExportKey;
header tfExportFeatures_t tfExportFeatures;

/*==========================================
=            TurboFlow Headers.            =
==========================================*/

header_type tfMeta_t {
    fields {
        curTs : 32;
        hashVal : 16;
        matchFlag : 8;
        isClone : 8;
    }
}

header_type tfExportStart_t {    
    fields {
        realEtherType : 16;
    }
}

header_type tfExportKey_t {
    fields {
        srcAddr : 32;
        dstAddr : 32;
        ports : 32;
        protocol : 8; 
    }
}

header_type tfExportFeatures_t {
    fields {
        byteCt : 32;
        startTs : 32;
        endTs : 32;        
        pktCt : 16;
    }
}

/*=====  End of TurboFlow Headers.  ======*/

/*===========================================
=            Forwarding Headers.            =
===========================================*/

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;

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
        hdrChecksum : 16; // here
        srcAddr : 32;
        dstAddr: 32;
    }
}
header ipv4_t ipv4;

header_type l4_ports_t {
    fields {
        ports : 32;
        // srcPort : 16;
        // dstPort : 16;
    }
}
header l4_ports_t l4_ports;


/*=====  End of Forwarding Headers.  ======*/




parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_TURBOFLOW : parse_turboflow; 
        ETHERTYPE_IPV4 : parse_ipv4; 
        default : ingress;
    }
}

// IP.
parser parse_ipv4 {
    extract(ipv4);
    return parse_l4;
}

// TCP / UDP ports.
parser parse_l4 {
    extract(l4_ports);

    return ingress;
}

parser parse_turboflow {
    extract(tfExportStart);
    extract(tfExportKey);
    extract(tfExportFeatures);
    return select(tfExportStart.realEtherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default : ingress;
    }
}

// e2e mirrored is always (in this example) a ethernet TurboFlow packet.
@pragma packet_entry
parser start_e2e_mirrored {
    extract(ethernet);
    extract(tfExportStart);
    extract(tfExportKey);
    extract(tfExportFeatures);
    // set_metadata(tfMeta.isClone, 1);
    return select(tfExportStart.realEtherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default : ingress;
    }
}


// @pragma packet_entry
// parser start_coalesced {
//     // extract(ethernet);
//     set_metadata(tfMeta.isClone, 1);
//     return ingress;
// }