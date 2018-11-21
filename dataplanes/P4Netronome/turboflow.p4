
// Header defs.
header_type extra_metadata_t {
    fields {
        hash16: 16;
        evictFlag: 32;
        evictMask: 32;
        tmp: 32;
        evictPos: 16;
        toCpu : 8;
        tmpPacketCt : 32;
        tmpByteCt : 32;
        }
}

header_type microflowRecord_t {
    fields {
        srcAddr : 32;
        dstAddr : 32;
        ports : 32;
        packetCount : 32;
        byteCount : 32;
    }
}

metadata extra_metadata_t em;

metadata microflowRecord_t temp_mf;


#define HASH_SIZE 65535
#define REC_SIZE 5
#define TOTAL_SIZE 327675

@pragma netro reglocked
register src_reg { width: 32; instance_count : TOTAL_SIZE; }

@pragma netro reglocked
register dst_reg { width: 32; instance_count : TOTAL_SIZE; }

@pragma netro reglocked
register ports_reg { width: 32; instance_count : TOTAL_SIZE; }

@pragma netro reglocked
register pktCt_reg { width: 32; instance_count : TOTAL_SIZE; }

@pragma netro reglocked
register byteCt_reg { width: 32; instance_count : TOTAL_SIZE; }

field_list flow{
    ipv4.srcAddr;
    ipv4.dstAddr;
    tcp.ports;
}
field_list_calculation hashfcn{
    input{
        flow;
    }
    algorithm: crc16;
    output_width: 16;
}


control tracking_logic {
    apply(update_entire_entry_table);
}

control old_tracking_logic {
    // Load key.
    apply(load_key_table);
    // Update if there's a match.
    if (temp_mf.srcAddr == ipv4.srcAddr){
        if (temp_mf.dstAddr == ipv4.dstAddr) {
            if (temp_mf.ports == tcp.ports) {
                apply(update_table);
            }
        }
    }
    // Replace entry if there's not.
    else {
        apply(replace_table);
    }

}


table load_key_table {
    actions {load_key_action;}
}

table update_table {
    actions {update_action;}
}

table replace_table {
    actions {replace_action;}
}

action load_key_action(){
    // compute hash.
    modify_field_with_hash_based_offset(em.hash16, 0, hashfcn, 16);
    // read key.
    register_read(temp_mf.srcAddr, src_reg, em.hash16);
    register_read(temp_mf.dstAddr, dst_reg, em.hash16);
    register_read(temp_mf.ports, ports_reg, em.hash16);
    // replace with yours. 
    register_write(src_reg, em.hash16, ipv4.srcAddr);    
    register_write(dst_reg, em.hash16, ipv4.dstAddr);    
    register_write(ports_reg, em.hash16, tcp.ports);    

}

action update_action(){
    // read, update, and write back features of the microflow being updated.
    register_read(temp_mf.packetCount, pktCt_reg, em.hash16);
    register_read(temp_mf.byteCount, byteCt_reg, em.hash16);
    modify_field(temp_mf.packetCount, temp_mf.packetCount+1);
    modify_field(temp_mf.byteCount, temp_mf.byteCount+ipv4.totalLen);
    register_write(pktCt_reg, em.hash16, temp_mf.packetCount);
    register_write(byteCt_reg, em.hash16, temp_mf.byteCount);
}

action replace_action(){
    // read the features of the microflow being evicted.
    register_read(temp_mf.packetCount, pktCt_reg, em.hash16);
    register_read(temp_mf.byteCount, byteCt_reg, em.hash16);

    // overwrite entry with new record.
    register_write(pktCt_reg, em.hash16, 1);
    register_write(byteCt_reg, em.hash16, ipv4.totalLen);

    // to CPU tag.
    modify_field(em.toCpu, 1);
}

table update_entire_entry_table {
    actions {update_entire_entry_action;}
}


action update_entire_entry_action(){
    // compute hash.
    modify_field_with_hash_based_offset(em.hash16, 0, hashfcn, 16);

    // read key.
    register_read(temp_mf.srcAddr, src_reg, em.hash16);
    register_read(temp_mf.dstAddr, dst_reg, em.hash16);
    register_read(temp_mf.ports, ports_reg, em.hash16);

    // replace with yours. 
    register_write(src_reg, em.hash16, ipv4.srcAddr);    
    register_write(dst_reg, em.hash16, ipv4.dstAddr);    
    register_write(ports_reg, em.hash16, tcp.ports);    

    // read features of the microflow
    register_read(temp_mf.packetCount, pktCt_reg, em.hash16);
    register_read(temp_mf.byteCount, byteCt_reg, em.hash16);

    // compute an evict mask.
    computeConditionalMask();

    // use evict mask to conditionally update features *without returning to control flow*
    modify_field(em.tmpPacketCt, (em.evictFlag & 1) | (~em.evictFlag & temp_mf.packetCount+1)); 
    modify_field(em.tmpByteCt, (em.evictFlag & ipv4.totalLen) | (~em.evictFlag & temp_mf.byteCount+ipv4.totalLen)); 
    
    // write back updated features. 
    register_write(pktCt_reg, em.hash16, em.tmpPacketCt);
    register_write(byteCt_reg, em.hash16, em.tmpByteCt);
}

#define WORDBITS 32

action computeConditionalMask(){
    // compute dirty flag. 0 = no evict.
    modify_field(em.evictFlag, (temp_mf.srcAddr ^ ipv4.srcAddr) | (temp_mf.dstAddr ^ ipv4.dstAddr) | (temp_mf.ports ^ tcp.ports));
    // Convert evict flag into a clean mask: 0xff... if evict, 0x00... if not.
    // http://homolog.us/blogs/blog/2014/12/04/the-entire-world-of-bit-twiddling-hacks/
    modify_field(em.evictMask, (em.evictFlag | -em.evictFlag));
    modify_field(em.evictMask, (em.evictMask >> (WORDBITS -1 )));
    modify_field(em.evictMask, (em.evictMask-1));

}    