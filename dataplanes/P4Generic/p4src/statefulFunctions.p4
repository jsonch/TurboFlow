/**
 *
 * placeholders for stateful functions. 
 * These must be re implemented using platform specific logic.
 * (in BMV2, these should be custom primitives that extend the simple_switch target)
 *
 */


/*====================================
=            Key Updates.            =
====================================*/

action sUpdateSrcAddr(){
    idx = flowKeyHashCalc();
    tfExportKey.srcAddr = rSrcAddr[idx];
    if (ipv4.srcAddr != rSrcAddr[idx]){
        rSrcAddr[idx] = ipv4.srcAddr;
    }
}

action sUpdateDstAddr(){
    idx = flowKeyHashCalc();
    tfExportKey.dstAddr = rDstAddr[idx];
    if (ipv4.dstAddr != rDstAddr[idx]){
        rDstAddr[idx] = ipv4.dstAddr;
    }
}

action sUpdatePorts(){
    idx = flowKeyHashCalc();
    tfExportKey.ports = rPorts[idx];
    if (l4_ports.ports != rPorts[idx]){
        rPorts[idx] = l4_ports.ports;
    }
}

action sUpdateProtocol(){
    idx = flowKeyHashCalc();
    tfExportKey.ports = rProtocol[idx];
    if (ipv4.protocol != rProtocol[idx]){
        rProtocol[idx] = ipv4.protocol;
    }
}

/*=====  End of Key Updates.  ======*/


/*=============================================
=            Flow Feature Updates.            =
=============================================*/

action sIncrementPktCt(){
    idx = flowKeyHashCalc();
    rPktCt[idx] += 1;
}

action sResetPktCt(){
    idx = flowKeyHashCalc();
    tfExportFeatures.pktCt = rPktCt[idx];
    rPktCt[idx] = 1;
}

action sIncrementByteCt(){
    idx = flowKeyHashCalc();
    rByteCt[idx] += ipv4.totalLen;
}

action sResetByteCt(){
    idx = flowKeyHashCalc();
    tfExportFeatures.byteCt = rByteCt[idx];
    rByteCt = ipv4.totalLen;
}

action sResetStartTs() {
    idx = flowKeyHashCalc();
    tfExportFeatures.startTs = rStartTs[idx];
    rStartTs[idx] = tfMeta.curTs;
}

/*=====  End of Flow Feature Updates.  ======*/
