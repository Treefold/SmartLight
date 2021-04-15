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

using namespace std;
using namespace Pistache;

// This is just a helper function to preety-print the Cookies that one of the enpoints shall receive.
void printCookies(const Http::Request& req) {
    auto cookies = req.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto& c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
}

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.
namespace Generic {

    void handleReady(const Rest::Request&, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, "Service is Ready!\n");
    }

}

// Definition of the SmartLightEnpoint class 
class SmartLightEndpoint {
public:
    explicit SmartLightEndpoint(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    { }

    // Initialization of the server. Additional options can be provided here
    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options()
            .threads(static_cast<int>(thr));
        httpEndpoint->init(opts);
        // Server routes are loaded up
        setupRoutes();
    }

    // Server is started threaded
    void start() {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
    }

    // When signaled server shuts down
    void stop(){
        httpEndpoint->shutdown();
    }

private:
    void setupRoutes() {
        using namespace Rest;
        // Defining various endpoints
        // Generally say that when http://localhost:9080/ready is called, the handleReady function should be called
        // All the arguments are given as strings. Convert them to the desired data type afterwards (std::stoi for string to int)
        Routes::Get(router, "/ready", Routes::bind(&Generic::handleReady));
        Routes::Post(router, "/init/:id", Routes::bind(&SmartLightEndpoint::initSmartLight, this));
        Routes::Post(router, "/rgb/:id/:red/:green/:blue", Routes::bind(&SmartLightEndpoint::setRGB, this));
        Routes::Get(router, "/rgb/:id", Routes::bind(&SmartLightEndpoint::getRGB, this));
    }

    /** Setup a SmartLight
     * @param id The id of the SmartLight to be initiated
     **/
    void initSmartLight(const Rest::Request& request, Http::ResponseWriter response){
        int id = std::stoi(request.param(":id").as<std::string>());

        // Does nothing...

        response.send(Http::Code::Ok, "The Smart Light setup has completed!");

    }

    /** Change the color of a SmartLight
     * @param id The id of the SmartLight to be changed
     * @param red Value between 0 and 255
     * @param green Value between 0 and 255
     * @param blue Value between 0 and 255
     **/
    void setRGB(const Rest::Request& request, Http::ResponseWriter response){
        int id = std::stoi(request.param(":id").as<std::string>());
        int R = std::stoi(request.param(":red").as<std::string>());
        int G = std::stoi(request.param(":green").as<std::string>());
        int B = std::stoi(request.param(":blue").as<std::string>());

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(smartLightLock);

        bool setResponse = false;
        if (0 <= R && R <= 255 &&
            0 <= G && G <= 255 &&
            0 <= B && B <= 255)
            setResponse = smartLights[id].setColor(R, G, B);

        if (setResponse) {
            response.send(Http::Code::Ok, "The color of the Smart Light number " + std::to_string(id) + " was set to " +
                                           std::to_string(R) + ", "+ std::to_string(G) + ", " + std::to_string(B) + ".");
        }
        else {
            response.send(Http::Code::Bad_Request, "Wrong values!");
        }

    }

    /** Get the color of a SmartLight
     * @param id The id of the SmartLight
     **/
    void getRGB(const Rest::Request& request, Http::ResponseWriter response){
        int id = std::stoi(request.param(":id").as<std::string>());

        Guard guard(smartLightLock);

        string valueSetting = smartLights[id].getColor();

        if (valueSetting != "") {
            response.send(Http::Code::Ok, "The color is " + valueSetting + ".");
        }
        else {
            response.send(Http::Code::Not_Found, "The color was not found...");
        }
    }

    // The class of the SmartLight
    class SmartLight {
    public:
        explicit SmartLight() {
            this->R = 222;
            this->G = 111;
            this->B = 000;
        }

        bool setColor(int R, int G, int B) {
            this->R = R;
            this->G = G;
            this->B = B;
            return true;
        }

        string getColor() {
            return std::to_string(this->R) + ", " + std::to_string(this->G) + ", " + std::to_string(this->B);
        }

    private:
        int R, G, B;
    };

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock smartLightLock;

    // Collection of Smart Lights
    SmartLight smartLights[10];

    // Defining the httpEndpoint and a router.
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
};

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

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    SmartLightEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();


    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std::cout << "received signal " << signal << std::endl;
    }
    else
    {
        std::cerr << "sigwait returns " << status << std::endl;
    }

    stats.stop();
}