/**
 *
 * mainTurboFlow.p4 -- Simple switch with TurboFlow.
 * 
 */


#include "parser.p4"
#include "turboFlow.p4"
#include "statefulExterns.p4"

// #include "../../modules/p4/miscUtils.p4"
// #include "../../modules/p4/forwardL2.p4"


control ingress {
	// Port to index mapping. Always do this.
    if (ig_intr_md.resubmit_flag == 0){
        apply(tiPortToIndex);            
    }
    // Stage 0: apply L2 forwarding.
    // ciForwardPacket(); // (forwardL2.p4)
    // Next stages: apply TurboFlow (only to IPv4 packets)
    if (valid(ipv4)) {
	    ciTurboFlow();
	}

}

control egress {  
	// Strip TurboFlow headers unless its an eviction packet to the CPU.
	ceTurboFlow();  
}



control ciTurboFlow {

	// Setup TurboFlow headers.
	apply(tiAddTfHeaders);

    // Update key fields.
    apply(tiUpdateSrcAddr);
    apply(tiUpdateDstAddr);
    apply(tiUpdatePorts);
    apply(tiUpdateProtocol);

    // Set match flag if all key fields are equal.
    if (ipv4.srcAddr == tfExportKey.srcAddr) {
        if (ipv4.dstAddr == tfExportKey.dstAddr) {
            if (l4_ports.ports == tfExportKey.ports) {
                if (ipv4.protocol == tfExportKey.protocol) {
                    apply(tiSetMatch);
                }
            }
        }
    }

    // update features (depending on match flag).
    if (tfMeta.matchFlag == 1) {
        apply(tiIncrementPktCt);
        apply(tiIncrementByteCt);
    }
    else {
        apply(tiResetPktCt);
        apply(tiResetByteCt);
        apply(tiResetStartTs);
        // No table for endTs, endTs is now, set in tiInitFr.
    }

    // If match flag == 0, multicast to the TurboFlow monitoring port.
    if (tfMeta.matchFlag == 0) {
        apply(tiMcToCpu);
    }

}
