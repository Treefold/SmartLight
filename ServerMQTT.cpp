/*
   Rares Cristea, 12.03.2021
   Example of a REST endpoint with routing
   using Mathieu Stefani's example, 07 février 2016
*/
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>
#include <signal.h>
#include "smartlight.cpp"
#include <mosquitto.h>

using namespace std;
using namespace Pistache;

// General advice: pay atetntion to the namespaces that you use in various contexts. Could prevent headaches.

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.


void onConnect(struct mosquitto *mosq, void *obj, int rc) {
	std :: cout << "ID: " << *(int *) obj << "\n";
	if (rc) {
		std :: cout << "[error] Error with result code: " << rc << "\n";
		exit(-1);
	}
	mosquitto_subscribe(mosq, NULL, "test/t1", 0);
}


void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    string s = (char *) msg->payload;
    int value = 0;
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            value = value * 10 + s[i] - '0';
        }
    }

    if (value != 0) {
        std :: cout << "[info] New message with topic " << msg->topic << ": " << value << "\n";
        alertCounter ++;
        if (value >= 50 || alertCounter >= 3) {
            std :: cout << "[error] Stop tempering with the installation NOW! Shut down the installation first!\n";
            alertCounter = 0;
        } else {
            std :: cout << "[error] Danger!!\n";
        }
    } else {
        std :: cout << "[info] New message with topic " << msg->topic << ": " << s << "\n";
    }   
}


int main(int argc, char *argv[]) {

    // This code is needed for gracefull shutdown of the server when no longer needed.
    sigset_t signals;
    if (sigemptyset(&signals) != 0
            || sigaddset(&signals, SIGTERM) != 0
            || sigaddset(&signals, SIGINT) != 0
            || sigaddset(&signals, SIGHUP) != 0
            || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
        perror("install signal handler failed");
        return 1;
    }

    // Set a port on which your server to communicate
    Port port(9080);

    // Number of threads used by the server
    int thr = 2;

    if (argc >= 2) {
        port = static_cast<uint16_t>(std::stol(argv[1]));

        if (argc == 3)
            thr = std::stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    std :: cout << "Cores = " << hardware_concurrency() << endl;
    std :: cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    SmartLightEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();

    int rc, id=12;

    mosquitto_lib_init();

    struct mosquitto *mosq;

    mosq = mosquitto_new("subscribe-test", true, &id);
    mosquitto_connect_callback_set(mosq, onConnect);
    mosquitto_message_callback_set(mosq, onMessage);

    rc = mosquitto_connect(mosq, "localhost", 1883, 10);
    if(rc) {
        std :: cout << "[warn] Could not connect to Broker with return code " << rc << "\n";
        return -1;
    }

    mosquitto_loop_start(mosq);
    //std :: cout << "Type exit to cancel:"
    getchar();
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std :: cout << "[info] received signal " << signal << std::endl;
    }
    else
    {
        std :: cerr << "[info] sigwait returns " << status << std::endl;
    }

  stats.stop();
}
