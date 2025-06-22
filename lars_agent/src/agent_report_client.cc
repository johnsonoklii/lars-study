#include "agent_report_client.h"

static report_client s_g_report_client;

void start_report_client() {
    s_g_report_client.start();
}