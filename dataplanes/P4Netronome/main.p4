#include "headers.p4"
#include "turboflow_v1.p4"



header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;

parser start { return parse_ethernet; }
parser parse_custom { return ingress; }


control ingress {
    forwarding_logic();
    tracking_logic();
    apply(profiling_table);
}

control forwarding_logic {
    apply(forward_table);
}

// forward packets out of a port.
table forward_table {
    reads { standard_metadata.ingress_port: exact; }
    actions { do_forward; }
}
action do_forward(egress_port) {   
    modify_field(standard_metadata.egress_spec, egress_port); 
}

table profiling_table {
    actions {do_nothing;}
}

action do_nothing() {
    modify_field(ipv4.ttl, ipv4.ttl);
}