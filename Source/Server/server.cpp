#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include <print>
#include <thread>

#include "attacker_api.hpp"
#include "victim_api.hpp"

int main() {
    //TODO:
    // Load server config from Database
    const int16_t attacker_port{ 1337 } ;
    const int16_t victim_port{ 3000 };

    std::println("Staring Attacker API on Port: {}", attacker_port);
    std::jthread attacker_api(start_attacker_api, attacker_port);

    std::println("Staring Victim API on Port: {}", attacker_port);
    std::jthread(start_victim_api, victim_port);
}