// build and run command (in cmd): 
// g++ smartlight.cpp -o smartlight -lpistache -lcrypto -lssl -lpthread -std=c++17 && ./smartlight

#include <algorithm>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>

#include <nlohmann/json.hpp>

using namespace std;
using namespace Pistache;
using json = nlohmann::json;

auto const null = nlohmann::detail::value_t::null;

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
        
        Routes::Post(router, "/settings", Routes::bind(&SmartLightEndpoint::setSettingsJSON, this));
    }

    /** Setup a SmartLight
     * @param id The id of the SmartLight to be initiated
     **/
    void initSmartLight(const Rest::Request& request, Http::ResponseWriter response){
        
        try {
            int id = std::stoi(request.param(":id").as<std::string>());

            if (id < 0 || id >= MaxSmartLights) { // test Id
                response.send(Http::Code::Bad_Request, "The Id is unavailable\n");
                return;
            }
            
            if (smartLights[id].IsInit()) { // prevent multiple init
                response.send(Http::Code::Bad_Request, "This smart light was already init\n");
                return;
            }
            // else not init

            smartLights[id].Init();
            response.send(Http::Code::Ok, "The Smart Light setup has completed!\n");
        }
        catch (...) {
            response.send(Http::Code::Internal_Server_Error, "Something unexpected happened\n");
        }
    }

    /** Change the color of a SmartLight
     * @param id The id of the SmartLight to be changed
     * @param red Value between 0 and 255
     * @param green Value between 0 and 255
     * @param blue Value between 0 and 255
     **/
    void setRGB(const Rest::Request& request, Http::ResponseWriter response){
        try {
            int id = std::stoi(request.param(":id").as<std::string>());
            int R = std::stoi(request.param(":red").as<std::string>());
            int G = std::stoi(request.param(":green").as<std::string>());
            int B = std::stoi(request.param(":blue").as<std::string>());

            // This is a guard that prevents editing the same value by two concurent threads. 
            Guard guard(smartLightLock);

            if (id < 0 || id >= MaxSmartLights) { // test Id
                response.send(Http::Code::Bad_Request, "The Id is unavailable\n");
                return;
            }
            
            if (! smartLights[id].IsInit()) { // don't use if not init
                response.send(Http::Code::Bad_Request, "This smart light was not init\n");
                return;
            }

            bool setResponse = smartLights[id].setColor(R, G, B);

            if (setResponse) {
                response.send(Http::Code::Ok, "The color of the Smart Light number " + std::to_string(id) + " was set to " +
                                            std::to_string(R) + ", "+ std::to_string(G) + ", " + std::to_string(B) + ".");
            }
            else {
                response.send(Http::Code::Bad_Request, "Wrong values!\n");
            }
        }
        catch (...) {
            response.send(Http::Code::Internal_Server_Error, "Something unexpected happened\n");
        }
    }

    /** Get the color of a SmartLight
     * @param id The id of the SmartLight
     **/
    void getRGB(const Rest::Request& request, Http::ResponseWriter response){
        try {
            int id = std::stoi(request.param(":id").as<std::string>());

            Guard guard(smartLightLock);

            if (id < 0 || id >= MaxSmartLights) { // test Id
                response.send(Http::Code::Bad_Request, "The Id is unavailable\n");
                return;
            }
            
            if (! smartLights[id].IsInit()) { // don't use if not init
                response.send(Http::Code::Bad_Request, "This smart light was not init\n");
                return;
            }

            string valueSetting = smartLights[id].getColor();

            if (valueSetting != "") {
                response.send(Http::Code::Ok, "The color is " + valueSetting + ".\n");
            }
            else {
                response.send(Http::Code::Not_Found, "The color was not found...\n");
            }
        }
        catch (...) {
            response.send(Http::Code::Internal_Server_Error, "Something unexpected happened\n");
        }
    }

    void setSettingsJSON(const Rest::Request& request, Http::ResponseWriter response){

        static const int nrSettings = 6;
        string settings[nrSettings] = {"powered", "luminosity", "temperature", "R", "G", "B"};

        try {
            Guard guard(smartLightLock);

            auto j = json::parse(request.body())["input_buffers"];

            json jSettings = j["settings"];
            int id = std::stoi((string) jSettings["id"]);
            
            if (id < 0 || id >= MaxSmartLights) { // test Id
                response.send(Http::Code::Bad_Request, "The Id is unavailable\n");
                return;
            }
            
            if (! smartLights[id].IsInit()) { // don't use if not init
                response.send(Http::Code::Bad_Request, "This smart light was not init\n");
                return;
            }

            SmartLight sl_copy = SmartLight(smartLights[id]);
            json jsonSettings;
            sl_copy.ExportToJson(jsonSettings);
            string rsp = "";

            for (json::iterator iter = jSettings["buffer-tokens"].begin(); iter != jSettings["buffer-tokens"].end(); ++iter) {
                bool isSetting = false;
                json jCurr = iter.value();
                string jName = jCurr["name"];

                for (int i = 0; i < nrSettings; i++) {
                    if (jName == settings[i]) {
                        isSetting = true;
                        break;
                    }
                }

                if (!isSetting) {
                    rsp += jName + " is not a setting\n";
                }
                else {
                    bool validRsp = true;
                    string jValue = jCurr["value"];
                    try {
                        jsonSettings[jName] = std::stoi(jValue);
                    } catch (...) {
                        response.send(Http::Code::Bad_Request, "The value for '" + jName + "' is not a number\n");
                        return;
                    }
                    
                    if (validRsp)
                        rsp += jName + " was set to " + jValue + "\n";
                    else
                        rsp += jName +  " was not found and or '" + jValue + "' was not a valid value\n";
                }
            }

            sl_copy.ImportFromJson(jsonSettings);
            cout << sl_copy.Repr() << endl;

            // // read json output
            // std::ifstream input_json("window_output.json");
            // json content_json;
            // input_json >> content_json;
            // auto& output = content_json["output_buffers"];
            // output[0]["value"] = actualized_state_dict["windows_open"];
            // output[1]["value"] = actualized_state_dict["windows_semi_open"];
            // output[2]["value"] = actualized_state_dict["windows_closed"];
            // output[3]["value"] = actualized_state_dict["curtains_open"];
            // output[4]["value"] = actualized_state_dict["alerted"];
            // input_json.close();

            // // write json output
            // std::ofstream output_json ("window_output.json");
            // output_json << std::setw(4) << content_json << std::endl;
            // output_json.close();


            // our_window.setActualizedStateDictWindow(actualized_state_dict);
            if (sl_copy.HasValidConfig()) {
                smartLights[id].UpdateFromSL(sl_copy)
                // TODO Update values in file (save state)
                response.send(Http::Code::Ok, rsp);
            } else {
                // invalid configuration -> the previous value remain unchanged
                response.send(Http::Code::Bad_Request, "Invalid new setting configuration\n");                
            }
        }
        catch (...) {
            response.send(Http::Code::Internal_Server_Error, "Something unexpected happened\n");
        }
    }

    // The class of the SmartLight
    class SmartLight {
    private:
        bool init = false, powered = false;
        int R, G, B, luminosity, temperature;

    public:
        explicit SmartLight() {
            this->R = 222;
            this->G = 111;
            this->B = 000;
            this->luminosity = 100;
            this->temperature = 0;
        } 

        SmartLight (const SmartLight &original) {
            this->UpdateFromSL(original);
        }

        void UpdateFromSL (const SmartLight &original) {
            this->init        = original.init;
            this->powered     = original.powered;
            this->R           = original.R;
            this->G           = original.G;
            this->B           = original.B;
            this->luminosity  = original.luminosity;
            this->temperature = original.temperature;
        }

        void ExportToJson (json &j) {
            j["init"] = this->init;
            j["powered"] = this->powered;
            j["R"] = this->R;
            j["G"] = this->G;
            j["B"] = this->B;
            j["luminosity"] = this->luminosity;
            j["temperature"] = this->temperature;
        }

        void ImportFromJson (const json &j) {
            if (j["init"] != null)
                this->init = j["init"];
            if (j["powered"] != null)
                this->powered = j["powered"];
            if (j["R"] != null)
                this->R = j["R"];
            if (j["G"] != null)
                this->G = j["G"];
            if (j["B"] != null)
                this->B = j["B"];
            if (j["luminosity"] != null)
              this->luminosity = j["luminosity"];
            if (j["temperature"] != null)
                this->temperature = j["temperature"];
        }

        string Repr (int indentation = 4) {
            json j;
            this->ExportToJson(j);
            return j.dump(indentation);
        }

        void Init() {
            this->init = true;
        }

        bool IsInit() {
            return this->init;
        }

        void SetPower(bool powered)
        {
            this->powered = powered;
        }

        bool IsPowered()
        {
            return this->powered;
        }

        bool SetR (const int R) {
            if (0 > R || R > 255)
                return false;
            this->R = R;
            return true;
        }

        bool SetG (const int G) {
            if (0 > G || G > 255)
                return false;
            this->G = G;
            return true;
        }

        bool SetB (const int B) {
            if (0 > B || B > 255)
                return false;
            this->B = B;
            return true;
        }

        bool SetLuminosity (const int luminosityB) {
            if (0 > luminosity || luminosity > 100)
                return false;
            this->luminosity = luminosity;
            return true;
        }

        bool SetTemperature (const int temperature) {
            if (0 > temperature || temperature > 100)
                return false;
            this->temperature = temperature;
            return true;
        }

        bool setColor(const int R, const int G, const int B) {
            
            if (0 <= R && R <= 255 &&
                0 <= G && G <= 255 &&
                0 <= B && B <= 255)
            {
                this->R = R;
                this->G = G;
                this->B = B;
                return true;
            }
            else return false;
        }

        string getColor() {
            return std::to_string(this->R) + ", " + std::to_string(this->G) + ", " + std::to_string(this->B);
        }

        void SetByName (const string name, const int value) {
            // no validation; (for copies and validated after)
            if (name == "powered") {
                this->powered = value;
                return;
            }   
            if (name == "R") {
                this->R = value;
                return;
            } 
            if (name == "G") {
                this->G = value;
                return;
            } 
            if (name == "B") {
                this->B = value;
                return;
            } 
            if (name == "luminosity") {
                this->luminosity = value;
                return;
            } 
            if (name == "temperature") {
                this->temperature = value;
                return;
            }   
            
        }

        bool HasValidConfig() {
            if (! this->init)
                return false;
            if (0 > R || R > 255)
                return false;
            if (0 > G || G > 255)
                return false;
            if (0 > B || B > 255)
                return false;
            if (0 > luminosity || luminosity > 100)
                return false;
            if (0 > temperature || temperature > 100)
                return false;

            return true;
        }
    };

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock smartLightLock;

    static const int MaxSmartLights = 10;

    // Collection of Smart Lights
    SmartLight smartLights[MaxSmartLights];

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