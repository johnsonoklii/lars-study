#include "agent_dns_client.h"

dns_client g_dns_client;

void start_dns_client(void) {
    g_dns_client.start();
}