/**
 *
 * turboFlow.p4 -- Turboflow data plane.
 * 
 */

// Max number of flows to track at once.
#define TF_HASH_TBL_SIZE 65536
#define TF_HASH_BIT_WIDTH 16

#define TF_MC_GID 666
#define TF_CLONE_MID 66
#define TF_COAL_MID  1016


// Setup TurboFlow Header. Needs controller rule.
@pragma stage 2
table tiAddTfHeaders { 
    reads {ethernet.etherType : exact;}
    actions {aiAddTfHeaders; aeDoNothing;}
    default_action : aeDoNothing();
}

action aiAddTfHeaders() {
    // TurboFlow Header.
    add_header(tfExportStart);
    modify_field(tfExportStart.realEtherType, ethernet.etherType);
    modify_field(ethernet.etherType, ETHERTYPE_TURBOFLOW);

    // TurboFlow data -- the exported flow's key.
    add_header(tfExportKey);
    modify_field(tfExportKey.srcAddr, 0);
    modify_field(tfExportKey.dstAddr, 0);
    modify_field(tfExportKey.ports, 0);
    modify_field(tfExportKey.protocol, 0);

    // TurboFlow data -- the exported flow's features.
    add_header(tfExportFeatures);

    // Compute hash of key.
    modify_field_with_hash_based_offset(tfMeta.hashVal, 0, flowKeyHashCalc, 65536);
    // Get 32 bit timestamp. 
    modify_field(tfMeta.curTs, ig_intr_md.ingress_mac_tstamp);
    modify_field(tfExportFeatures.endTs, ig_intr_md.ingress_mac_tstamp);
}

field_list flKeyFields {
    ipv4.srcAddr;
    ipv4.dstAddr;
    l4_ports.ports;
    ipv4.protocol;
}

field_list_calculation flowKeyHashCalc {
    input { flKeyFields; }
    algorithm : crc16;
    output_width : TF_HASH_BIT_WIDTH;
}

/*=========================================================
=            Update stateful keys, load evict FR.            =
=========================================================*/


table tiUpdateSrcAddr {
    actions {aiUpdateSrcAddr;}
    default_action : aiUpdateSrcAddr();
}
action aiUpdateSrcAddr() {
    sUpdateSrcAddr.execute_stateful_alu(tfMeta.hashVal);
}

// evictFr.srcAddr = entry
// If new != entry:
//      entry = new.srcAddr
blackbox stateful_alu sUpdateSrcAddr {
    reg : rSrcAddr;
    condition_lo : ipv4.srcAddr == register_lo;

    update_lo_1_predicate : not condition_lo;
    update_lo_1_value : ipv4.srcAddr;

    // output_predicate : not condition_lo;
    output_dst : tfExportKey.srcAddr;
    output_value : register_lo;
}

register rSrcAddr {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}

table tiUpdateDstAddr {
    actions {aiUpdateDstAddr;}
    default_action : aiUpdateDstAddr();
}
action aiUpdateDstAddr() {
    sUpdateDstAddr.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sUpdateDstAddr {
    reg : rDstAddr;
    condition_lo : ipv4.dstAddr == register_lo;

    update_lo_1_predicate : not condition_lo;
    update_lo_1_value : ipv4.dstAddr;

    // output_predicate : not condition_lo;
    output_dst : tfExportKey.dstAddr;
    output_value : register_lo;
}

register rDstAddr {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}


table tiUpdatePorts {
    actions {aiUpdatePorts;}
    default_action : aiUpdatePorts();
}
action aiUpdatePorts() {
    sUpdatePorts.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sUpdatePorts {
    reg : rPorts;
    condition_lo : l4_ports.ports == register_lo;

    update_lo_1_predicate : not condition_lo;
    update_lo_1_value : l4_ports.ports;

    // output_predicate : not condition_lo;
    output_dst : tfExportKey.ports;
    output_value : register_lo;
}

register rPorts {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}


table tiUpdateProtocol {
    actions {aiUpdateProtocol;}
    default_action : aiUpdateProtocol();
}
action aiUpdateProtocol() {
    sUpdateProtocol.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sUpdateProtocol {
    reg : rProtocol;
    condition_lo : ipv4.protocol == register_lo;

    update_lo_1_predicate : not condition_lo;
    update_lo_1_value : ipv4.protocol;

    // output_predicate : not condition_lo;
    output_dst : tfExportKey.protocol;
    output_value : register_lo;
}

register rProtocol {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}


/*=====  End of Update stateful keys, load evict FR.  ======*/


/*=======================================
=            Set evict flag.            =
=======================================*/

@pragma stage 5
table tiSetMatch {
    actions { aiSetMatch;}
    default_action : aiSetMatch();
}
action aiSetMatch(){
    modify_field(tfMeta.matchFlag, 1);
}

/*=====  End of Set evict flag.  ======*/


/*=======================================
=            Update Features            =
=======================================*/

// Packet count.
table tiIncrementPktCt {
    actions {aiIncrementPktCt;}
    default_action : aiIncrementPktCt();
}
action aiIncrementPktCt() {
    sIncrementPktCt.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sIncrementPktCt {
    reg : rPktCt;
    update_lo_1_value : register_lo + 1;
}
register rPktCt {
    width : 16;
    instance_count : TF_HASH_TBL_SIZE;
}


table tiResetPktCt {
    actions {aiResetPktCt;}
    default_action : aiResetPktCt();
}
action aiResetPktCt() {
    sResetPktCt.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sResetPktCt {
    reg : rPktCt;
    update_lo_1_value : 1;
    output_dst : tfExportFeatures.pktCt;
    output_value : register_lo;
}


// Byte count.
table tiIncrementByteCt {
    actions {aiIncrementByteCt;}
    default_action : aiIncrementByteCt();
}
action aiIncrementByteCt() {
    sIncrementByteCt.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sIncrementByteCt {
    reg : rByteCt;
    update_lo_1_value : register_lo + ipv4.totalLen;
}
register rByteCt {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}


table tiResetByteCt {
    actions {aiResetByteCt;}
    default_action : aiResetByteCt();
}
action aiResetByteCt() {
    sResetByteCt.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sResetByteCt {
    reg : rByteCt;
    update_lo_1_value : ipv4.totalLen;
    output_dst : tfExportFeatures.byteCt;
    output_value : register_lo;
}

// Start timestamp.
table tiResetStartTs {
    actions {aiResetStartTs;}
    default_action : aiResetStartTs();
}
action aiResetStartTs() {
    sResetStartTs.execute_stateful_alu(tfMeta.hashVal);
}

blackbox stateful_alu sResetStartTs {
    reg : rStartTs;
    update_lo_1_value : tfMeta.curTs;
    output_dst : tfExportFeatures.startTs;
    output_value : register_lo;
}
register rStartTs {
    width : 32;
    instance_count : TF_HASH_TBL_SIZE;
}


// End timestamp -- no state needed, end Ts is just when the eviction happens.



/*=====  End of Update Features  ======*/


// Multicast to CPU port using TurboFlow GID.
table tiMcToCpu {
    actions {aiMcToCpu;}
    default_action : aiMcToCpu();
}
action aiMcToCpu() {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, TF_MC_GID);  
}


/*==================================================
=            TurboFlow Egress Pipeline.            =
==================================================*/

control ceTurboFlow {
    if (ethernet.etherType == ETHERTYPE_TURBOFLOW) {
        apply(teProcessTfHeader);
    }
}


// default: (port == other) --> removeTfHeader()
// (port == cpuPort, isMirror == false) --> clone_e2e to truncator and drop.
// (port == cpuPort, isMirror == true) --> do nothing.
table teProcessTfHeader {
    reads {
        eg_intr_md.egress_port : exact;
        tfMeta.isClone : exact;
    }
    actions { aeDoNothing; aeRemoveTfHeader; aeCloneToTruncator;}
    default_action : aeRemoveTfHeader();
}

action aeDoNothing() {
    no_op();
}

action aeRemoveTfHeader() {
    modify_field(ethernet.etherType, tfExportStart.realEtherType);
    remove_header(tfExportStart);
    remove_header(tfExportKey);
    remove_header(tfExportFeatures);
}

action aeCloneToTruncator() {
    modify_field(tfMeta.isClone, 1);
    clone_e2e(TF_CLONE_MID, flCloneMeta);
    // sample_e2e(TF_COAL_MID, 72);
    drop();
}
field_list flCloneMeta {
    tfMeta.isClone;
}

/*=====  End of TurboFlow Egress Pipeline.  ======*/


